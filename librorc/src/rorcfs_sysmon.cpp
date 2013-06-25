/**
 * @file rorcfs_sysmon.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2011-11-16
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "rorcfs_bar.hh"
#include "rorcfs_sysmon.hh"



rorcfs_sysmon::rorcfs_sysmon
(
    librorc_bar *parent_bar
)
{
	m_bar = NULL;
    if( !parent_bar )
	{
		throw LIBRORC_SYSMON_ERROR_CONSTRUCTOR_FAILED;
	}

    m_bar = parent_bar;
}



rorcfs_sysmon::~rorcfs_sysmon()
{
	m_bar = NULL;
}



/** Firmware ******************************************************/



uint32_t
rorcfs_sysmon::FwRevision()
{
	uint32_t firmware_revision
        = m_bar->get(RORC_REG_FIRMWARE_REVISION);

	if(firmware_revision == 0xffffffff)
    {
        throw LIBRORC_SYSMON_ERROR_PCI_PROBLEM;
    }

	return firmware_revision;
}



uint32_t
rorcfs_sysmon::FwBuildDate()
{
    uint32_t date
        = m_bar->get(RORC_REG_FIRMWARE_DATE);

	if(date == 0xffffffff)
    {
        throw LIBRORC_SYSMON_ERROR_PCI_PROBLEM;
    }

	return date;
}



/** PCI ***********************************************************/



uint32_t
rorcfs_sysmon::pcieNumberOfLanes()
{
    uint32_t status = m_bar->get(RORC_REG_PCIE_CTRL);
    return(1<<(status>>3 & 0x3));
}



uint32_t
rorcfs_sysmon::pcieGeneration()
{
    uint32_t status = m_bar->get(RORC_REG_PCIE_CTRL);
    return(1<<(status>>5 & 0x01));
}



/** SYSTEM Monitoring *********************************************/



double
rorcfs_sysmon::FPGATemperature()
{
	uint32_t value = m_bar->get(RORC_REG_FPGA_TEMPERATURE);
	return (double)(value*503.975/1024.0 - 273.15);
}



double
rorcfs_sysmon::VCCINT()
{
	uint32_t value = m_bar->get(RORC_REG_FPGA_VCCINT);
	return (double)(value/1024.0 * 3.0);
}



double
rorcfs_sysmon::VCCAUX()
{
	uint32_t value = m_bar->get(RORC_REG_FPGA_VCCAUX);
	return (double)(value/1024.0 * 3.0);
}



//void rorcfs_sysmon::setIcapDin ( uint32_t dword )
//{
//	bar->set(RORC_REG_ICAP_DIN, dword);
//}



//void rorcfs_sysmon::setIcapDinReorder ( uint32_t )
//{
//	bar->set(RORC_REG_ICAP_DIN_REORDER, dword);
//}



bool
rorcfs_sysmon::systemClockIsRunning()
{
    uint32_t ddrctrl = m_bar->get(RORC_REG_DDR3_CTRL);
    if( ((ddrctrl>>3)&1) == 1 )
    { return true; }
    else
    { return false; }
}



bool
rorcfs_sysmon::systemFanIsEnabled()
{
    uint32_t fanctrl = m_bar->get(RORC_REG_FAN_CTRL);
    if ( !(fanctrl & (1<<31)) )
    { return false; }
    else
    { return true; }
}



bool
rorcfs_sysmon::systemFanIsRunning()
{
    uint32_t fanctrl = m_bar->get(RORC_REG_FAN_CTRL);
    if( !(fanctrl & (1<<29)) )
    { return false; }
    else
    { return true; }
}



double
rorcfs_sysmon::systemFanSpeed()
{
    uint32_t fanctrl = m_bar->get(RORC_REG_FAN_CTRL);
    return 15/((fanctrl & 0x1fffffff)*0.000000004);
}



/** QSFP Monitoring ***********************************************/



bool
rorcfs_sysmon::qsfpIsPresent
(
    uint32_t index
)
{
    uint32_t qsfp_ctrl = m_bar->get(RORC_REG_QSFP_CTRL);

    if( ((~qsfp_ctrl)>>(8*index+2) & 0x01) == 1 )
    {
        return true;
    }

    return false;
}



bool
rorcfs_sysmon::qsfpLEDIsOn
(
    uint32_t qsfp_index,
    uint32_t LED_index
)
{
    uint32_t qsfp_ctrl = m_bar->get(RORC_REG_QSFP_CTRL);

    if( ((~qsfp_ctrl)>>(8*qsfp_index+LED_index) & 0x01) == 1 )
    {
        return true;
    }

    return false;
}



string*
rorcfs_sysmon::qsfpVendorName
(
    uint32_t index
)
{
    qsfp_set_page0_and_config(index);
    return( qsfp_i2c_string_readout(148, 163) );
}



string*
rorcfs_sysmon::qsfpPartNumber
(
    uint32_t index
)
{
    qsfp_set_page0_and_config(index);
    return( qsfp_i2c_string_readout(168, 183) );
}



float
rorcfs_sysmon::qsfpTemperature
(
    uint32_t index
)
{
    qsfp_set_page0_and_config(index);

    uint8_t data_r;
    data_r = i2c_read_mem(SLVADDR, 23);

    uint32_t temp = data_r;
    data_r = i2c_read_mem(SLVADDR, 22);

    temp += ((uint32_t)data_r<<8);

    return ((float)temp/256);
}



//_________________________________________________________________________________



    void
    rorcfs_sysmon::i2c_reset()
    {
        m_bar->set(RORC_REG_I2C_OPERATION, 0x00040000);

        uint32_t status
            = wait_for_tip_to_negate();

        /** read back not only zeros */
        if ( !(status & 0x00ff0000) )
        {
            throw LIBRORC_SYSMON_ERROR_I2C_RESET_FAILED;
        }
    }



    uint8_t
    rorcfs_sysmon::i2c_read_mem
    (
        uint8_t slvaddr,
        uint8_t memaddr
    )
    {
        /** slave address shifted by one, write bit set */
        uint8_t addr_wr = (slvaddr<<1);
        uint8_t addr_rd = (slvaddr<<1) | 0x01;

        /** write addr + write bit to TX register, set STA, set WR */
        m_bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_wr<<8)) );
        check_rxack_is_zero( wait_for_tip_to_negate() );

        /** set mem addr, set WR bit */
        m_bar->set(RORC_REG_I2C_OPERATION, (0x00100000 | (memaddr<<8)) );
        check_rxack_is_zero( wait_for_tip_to_negate() );

        /** set slave addr + read bit, set STA, set WR */
        m_bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_rd<<8)) );
        uint32_t status = wait_for_tip_to_negate();

        /** set RD, set ACK=1 (NACK), set STO */
        m_bar->set(RORC_REG_I2C_OPERATION, 0x00680000);
        status = wait_for_tip_to_negate();

        return(status & 0xff);
    }



    void
    rorcfs_sysmon::i2c_write_mem
    (
        uint8_t slvaddr,
        uint8_t memaddr,
        uint8_t data
    )
    {
        /** slave address shifted by one, write bit set */
        uint8_t addr_wr = (slvaddr<<1);

        /** write addr + write bit to TX register, set STA, set WR */
        m_bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_wr<<8)) );
        check_rxack_is_zero( wait_for_tip_to_negate() );

        /** set mem addr, set WR bit */
        m_bar->set(RORC_REG_I2C_OPERATION, (0x00100000 | (memaddr<<8)) );
        check_rxack_is_zero( wait_for_tip_to_negate() );

        /** set WR, set ACK=0 (ACK), set STO, set data */
        m_bar->set(RORC_REG_I2C_OPERATION, (0x00500000|(unsigned int)(data<<8)));
        wait_for_tip_to_negate();
    }



    uint32_t
    rorcfs_sysmon::wait_for_tip_to_negate()
    {
        uint32_t status = m_bar->get(RORC_REG_I2C_OPERATION);
        while( status & 0x02000000 )
        {
            usleep(100);
            status = m_bar->get(RORC_REG_I2C_OPERATION);
        }

        return status;
    }



    void
    rorcfs_sysmon::check_rxack_is_zero( uint32_t status )
    {
        if( status & 0x80000000 )
        {
            throw LIBRORC_SYSMON_ERROR_RXACK;
        }
    }



    void rorcfs_sysmon::i2c_set_config( uint32_t config )
    {
        m_bar->set(RORC_REG_I2C_CONFIG, config);
    }



    string*
    rorcfs_sysmon::qsfp_i2c_string_readout
    (
        uint8_t start,
        uint8_t end
    )
    {
        string *readout = new string();
        uint8_t data_r = 0;
        for(uint8_t i=start; i<=end; i++)
        {
            data_r = i2c_read_mem(SLVADDR, i);
            readout += (char)data_r;
        }
        return readout;
    }



    void
    rorcfs_sysmon::qsfp_set_page0_and_config
    (
        uint32_t index
    )
    {
        uint8_t data_r;

        data_r = i2c_read_mem(SLVADDR, 127);

        if( data_r!=0 )
        {
            i2c_write_mem(SLVADDR, 127, 0);
        }

        i2c_set_config( 0x01f30081 | ((1<<index)<<8) );
    }
