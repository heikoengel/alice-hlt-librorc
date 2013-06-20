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

//TODO: get rid of all the asserts here

rorcfs_sysmon::rorcfs_sysmon()
{
	bar = NULL;
}



rorcfs_sysmon::~rorcfs_sysmon()
{
	bar = NULL;
}



int32_t
rorcfs_sysmon::init( librorc_bar *parent_bar )
{
    if (parent_bar)
	{
		bar = parent_bar;
		return 0;
	}

    return -1;
}



uint32_t
rorcfs_sysmon::getFwRevision()
{
	assert(bar!=NULL);
	return bar->get(RORC_REG_FIRMWARE_REVISION);
}



uint32_t
rorcfs_sysmon::getFwBuildDate()
{
	assert(bar!=NULL);
	return bar->get(RORC_REG_FIRMWARE_DATE);
}



uint32_t
rorcfs_sysmon::getFanTachValue()
{
	assert(bar!=NULL);
//	unsigned int rpm_raw = bar->get(RORC_REG_FPGA_FAN_TACH_VALUE);
//	if ( rpm_raw==0 )
//		return 0;
//	else return (unsigned int)(750000000/rpm_raw);
	return 0;
}



double
rorcfs_sysmon::getFPGATemperature()
{
	assert(bar!=NULL);
	uint32_t value = bar->get(RORC_REG_FPGA_TEMPERATURE);
	return (double)(value*503.975/1024.0 - 273.15);
}



double
rorcfs_sysmon::getVCCINT()
{
	assert(bar!=NULL);
	uint32_t value = bar->get(RORC_REG_FPGA_VCCINT);
	return (double)(value/1024.0 * 3.0);
}



double
rorcfs_sysmon::getVCCAUX()
{
	assert(bar!=NULL);
	uint32_t value = bar->get(RORC_REG_FPGA_VCCAUX);
	return (double)(value/1024.0 * 3.0);
}



//void rorcfs_sysmon::setIcapDin ( unsigned int dword )
//{
//	assert(bar!=NULL);
//	bar->set(RORC_REG_ICAP_DIN, dword);
//}



//void rorcfs_sysmon::setIcapDinReorder ( unsigned int dword )
//{
//	assert(bar!=NULL);
//	bar->set(RORC_REG_ICAP_DIN_REORDER, dword);
//}



int32_t
rorcfs_sysmon::i2c_reset()
{
	assert(bar!=NULL);
	bar->set(RORC_REG_I2C_OPERATION, 0x00040000);

    uint32_t status
        = wait_for_tip_to_negate();

    /** read back not only zeros */
	if ( status & 0x00ff0000 )
	{ return 0; }
	else
	{ return -1; }
}



unsigned char
rorcfs_sysmon::i2c_read_mem
(
    unsigned char  slvaddr,
    unsigned char  memaddr
)
{
	assert(bar!=NULL);
	unsigned char addr_wr, addr_rd;

	/** slave address shifted by one, write bit set */
	addr_wr = (slvaddr<<1);
	addr_rd = (slvaddr<<1) | 0x01;

	/** write addr + write bit to TX register, set STA, set WR */
	bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_wr<<8)) );
	check_rxack_is_zero( wait_for_tip_to_negate() );

	/** set mem addr, set WR bit */
	bar->set(RORC_REG_I2C_OPERATION, (0x00100000 | (memaddr<<8)) );
	check_rxack_is_zero( wait_for_tip_to_negate() );

	/** set slave addr + read bit, set STA, set WR */
	bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_rd<<8)) );
	uint32_t status = wait_for_tip_to_negate();

	/** set RD, set ACK=1 (NACK), set STO */
	bar->set(RORC_REG_I2C_OPERATION, 0x00680000);
	status = wait_for_tip_to_negate();

	return(status & 0xff);
}



int32_t
rorcfs_sysmon::i2c_write_mem
(
    unsigned char slvaddr,
    unsigned char memaddr,
    unsigned char data
)
{
    assert(bar!=NULL);

	/** slave address shifted by one, write bit set */
	unsigned char addr_wr = (slvaddr<<1);

	/** write addr + write bit to TX register, set STA, set WR */
	bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_wr<<8)) );
	check_rxack_is_zero( wait_for_tip_to_negate() );

	/** set mem addr, set WR bit */
	bar->set(RORC_REG_I2C_OPERATION, (0x00100000 | (memaddr<<8)) );
	check_rxack_is_zero( wait_for_tip_to_negate() );


//	/** set slave addr + write bit, set STA, set WR */
//	bar->set(RORC_REG_I2C_OPERATION, (0x00900000 | (addr_wr<<8)) );
//	status = wait_for_tip_to_negate();

	/** set WR, set ACK=0 (ACK), set STO, set data */
	bar->set(RORC_REG_I2C_OPERATION, (0x00500000|(unsigned int)(data<<8)));
	wait_for_tip_to_negate();

	return 0;
}



    uint32_t
    rorcfs_sysmon::wait_for_tip_to_negate()
    {
        uint32_t status = bar->get(RORC_REG_I2C_OPERATION);
        while( status & 0x02000000 )
        {
            usleep(100);
            status = bar->get(RORC_REG_I2C_OPERATION);
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



void rorcfs_sysmon::i2c_set_config( unsigned int config )
{
	assert(bar!=NULL);
	bar->set(RORC_REG_I2C_CONFIG, config);
}
