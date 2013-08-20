/**
 * @file
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
 * TODO/Note: i2c_reset, i2c_read_mem and i2c_write_mem shall not be
 * called from parallel processes. This may result in deadlocks or
 * data corruption.
 * */

#include <librorc/sysmon.hh>

namespace librorc
{


    sysmon::sysmon
    (
         bar *parent_bar
    )
    {
        m_bar = NULL;
        if( !parent_bar )
        {
            throw LIBRORC_SYSMON_ERROR_CONSTRUCTOR_FAILED;
        }

        m_bar = parent_bar;
    }



    sysmon::~sysmon()
    {
        m_bar = NULL;
    }



    /** Firmware ******************************************************/



    uint32_t
    sysmon::FwRevision()
    {
        uint32_t firmware_revision
            = m_bar->get32(RORC_REG_FIRMWARE_REVISION);

        if(firmware_revision == 0xffffffff)
        {
            throw LIBRORC_SYSMON_ERROR_PCI_PROBLEM;
        }

        return firmware_revision;
    }



    uint32_t
    sysmon::FwBuildDate()
    {
        uint32_t date
            = m_bar->get32(RORC_REG_FIRMWARE_DATE);

        if(date == 0xffffffff)
        {
            throw LIBRORC_SYSMON_ERROR_PCI_PROBLEM;
        }

        return date;
    }



    /** PCI ***********************************************************/



    uint32_t
    sysmon::pcieNumberOfLanes()
    {
        uint32_t status = m_bar->get32(RORC_REG_PCIE_CTRL);
        return(1<<(status>>3 & 0x3));
    }



    uint32_t
    sysmon::pcieGeneration()
    {
        uint32_t status = m_bar->get32(RORC_REG_PCIE_CTRL);
        return(1<<(status>>5 & 0x01));
    }



    /** SYSTEM Monitoring *********************************************/



    double
    sysmon::FPGATemperature()
    {
        uint32_t value = m_bar->get32(RORC_REG_FPGA_TEMPERATURE);
        return (double)(value*503.975/1024.0 - 273.15);
    }



    double
    sysmon::VCCINT()
    {
        uint32_t value = m_bar->get32(RORC_REG_FPGA_VCCINT);
        return (double)(value/1024.0 * 3.0);
    }



    double
    sysmon::VCCAUX()
    {
        uint32_t value = m_bar->get32(RORC_REG_FPGA_VCCAUX);
        return (double)(value/1024.0 * 3.0);
    }



    //void librorc_sysmon::setIcapDin ( uint32_t dword )
    //{
    //	bar->set(RORC_REG_ICAP_DIN, dword);
    //}



    //void librorc_sysmon::setIcapDinReorder ( uint32_t )
    //{
    //	bar->set(RORC_REG_ICAP_DIN_REORDER, dword);
    //}



    bool
    sysmon::systemClockIsRunning()
    {
        /** check firmware type */
        if( (m_bar->get32(RORC_REG_TYPE_CHANNELS)>>16) != RORC_CFG_PROJECT_hwtest )
        {
            /* TODO: the register below is only available in 'hwtest'
             * -> need to find a firmware independent detection here.
             *  */
            return true;
        }
        else
        {
            uint32_t ddrctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
            if( ((ddrctrl>>3)&1) == 1 )
            { return true; }
            else
            { return false; }
        }
    }



    bool
    sysmon::systemFanIsEnabled()
    {
        uint32_t fanctrl = m_bar->get32(RORC_REG_FAN_CTRL);
        if ( !(fanctrl & (1<<31)) )
        { return false; }
        else
        { return true; }
    }



    bool
    sysmon::systemFanIsRunning()
    {
        uint32_t fanctrl = m_bar->get32(RORC_REG_FAN_CTRL);
        if( !(fanctrl & (1<<29)) )
        { return false; }
        else
        { return true; }
    }



    double
    sysmon::systemFanSpeed()
    {
        uint32_t fanctrl = m_bar->get32(RORC_REG_FAN_CTRL);
        return 15/((fanctrl & 0x1fffffff)*0.000000004);
    }



    /** QSFP Monitoring ***********************************************/



    bool sysmon::qsfpIsPresent
    (
        uint8_t index
    )
    {
        uint32_t qsfp_ctrl = m_bar->get32(RORC_REG_QSFP_CTRL);

        if( ((~qsfp_ctrl)>>(8*index+2) & 0x01) == 1 )
        {
            return true;
        }

        return false;
    }



    bool sysmon::qsfpLEDIsOn
    (
        uint8_t qsfp_index,
        uint8_t LED_index
    )
    {
        uint32_t qsfp_ctrl = m_bar->get32(RORC_REG_QSFP_CTRL);

        if( ((~qsfp_ctrl)>>(8*qsfp_index+LED_index) & 0x01) == 1 )
        {
            return true;
        }

        return false;
    }



    string*
    sysmon::qsfpVendorName
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        return( qsfp_i2c_string_readout(index, 148, 163) );
    }



    string*
    sysmon::qsfpPartNumber
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        return( qsfp_i2c_string_readout(index, 168, 183) );
    }



    float
    sysmon::qsfpTemperature
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);

        uint8_t data_r;
        data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, 23);

        uint32_t temp = data_r;
        data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, 22);

        temp += ((uint32_t)data_r<<8);

        return ((float)temp/256);
    }



    uint8_t
    sysmon::i2c_read_mem
    (
        uint8_t chain,
        uint8_t slvaddr,
        uint8_t memaddr
    )
    {
        uint32_t opdata = memaddr;
        m_bar->set32(RORC_REG_I2C_OPERATION, opdata);

        i2c_module_start(chain, 
                slvaddr,
                I2C_READ,
                0, /** mode */
                1); /** byte_enable */

        i2c_wait_for_cmpl();

        return ((m_bar->get32(RORC_REG_I2C_OPERATION)>>8) & 0xff);
    }



    void
    sysmon::i2c_write_mem
    (
        uint8_t chain,
        uint8_t slvaddr,
        uint8_t memaddr,
        uint8_t data
    )
    {
        uint32_t opdata = (data<<8) | (memaddr);
        m_bar->set32(RORC_REG_I2C_OPERATION, opdata);

        i2c_module_start(chain, 
                slvaddr,
                I2C_WRITE,
                0, /** mode */
                1); /** byte_enable */

        i2c_wait_for_cmpl();
    }



    void
    sysmon::i2c_write_mem_dual
    (
        uint8_t chain,
        uint8_t slvaddr,
        uint8_t memaddr0,
        uint8_t data0,
        uint8_t memaddr1,
        uint8_t data1
    )
    {
        uint32_t opdata = (data1<<24) | (memaddr1<<16) | 
            (data0<<8) | (memaddr0);
        m_bar->set32(RORC_REG_I2C_OPERATION, opdata);

        i2c_module_start(chain, 
                slvaddr,
                I2C_WRITE,
                0, /** mode */
                3); /** byte_enable */

        i2c_wait_for_cmpl();
    }



    /** Protected ***** ***********************************************/


    uint32_t
    sysmon::i2c_wait_for_cmpl()
    {
        uint32_t status = m_bar->get32(RORC_REG_I2C_CONFIG);
        while( (status & 0x1)==0 )
        {
            usleep(100);
            status = m_bar->get32(RORC_REG_I2C_CONFIG);
        }

        if ( status & (0xc0) ) /** error flags: bits 6,7 */
        {
            throw LIBRORC_SYSMON_ERROR_RXACK;
        }
        return status;
    }


    void
    sysmon::i2c_set_config( uint32_t config )
    {
        m_bar->set32(RORC_REG_I2C_CONFIG, config);
    }



    string*
    sysmon::qsfp_i2c_string_readout
    (
        uint8_t index,
        uint8_t start,
        uint8_t end
    )
    {
        string *readout = new string();
        char data_r = 0;
        for(uint8_t i=start; i<=end; i++)
        {
            data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, i);
            readout->append(&data_r);
        }
        return readout;
    }



    void
    sysmon::qsfp_select_page0
    (
        uint8_t index
    )
    {
        uint8_t data_r;

        /** get current page **/
        data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, 127);

        if( data_r!=0 )
        {
            /** set to page 0 **/
            i2c_write_mem(index, QSFP_I2C_SLVADDR, 127, 0);
        }

    }

    void
    sysmon::i2c_module_start
    (
         uint8_t chain,
         uint8_t slvaddr,
         uint8_t cmd,
         uint8_t mode,
         uint8_t byte_enable
    )
    {

        /** Chain mapping:
         * 0: QSFP0
         * 1: QSFP1
         * 2: QSFP2
         * 3: DDR3
         * 4: RefClkGen
         * 5: FMC
         *
         * bytes_enable:
         * 0x1: lower byte
         * 0x2: upper byte
         * 0x3: both bytes
         *
         * mode:
         * 0: 100 kHz
         * 1: 400 kHz
         * */

        if ( (chain > 5) || (byte_enable > 3) || 
                (cmd!=I2C_READ && cmd!=I2C_WRITE) || (mode>1) )
        {
            throw LIBRORC_SYSMON_ERROR_I2C_INVALID_PARAM;
        }

        uint8_t chain_select = (1<<chain);

        uint32_t cfgcmd = (chain_select<<24) |
            (slvaddr<<8) |
            (mode<<5) |
            (byte_enable<<3) |
            cmd;

        m_bar->set32(RORC_REG_I2C_CONFIG, cfgcmd);
    }

}
