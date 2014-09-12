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
#include <librorc/registers.h>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>

namespace LIBRARY_NAME
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

        /** default to 100 kHz I2C speed */
        m_i2c_hsmode = 0;
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
        { throw LIBRORC_SYSMON_ERROR_PCI_PROBLEM; }

        return firmware_revision;
    }



    uint32_t
    sysmon::FwBuildDate()
    {
        uint32_t date
            = m_bar->get32(RORC_REG_FIRMWARE_DATE);

        if(date == 0xffffffff)
        { throw LIBRORC_SYSMON_ERROR_PCI_PROBLEM; }

        return date;
    }

    uint16_t
    sysmon::firmwareType()
    {
        return((m_bar->get32(RORC_REG_TYPE_CHANNELS)>>16) & 0xffff);
    }

    bool
    sysmon::firmwareIsHltIn()
    {
        return(firmwareType()==RORC_CFG_PROJECT_hlt_in);
    }

    bool
    sysmon::firmwareIsHltOut()
    {
        return(firmwareType()==RORC_CFG_PROJECT_hlt_out);
    }

    bool
    sysmon::firmwareIsHltInFcf()
    {
        return(firmwareType()==RORC_CFG_PROJECT_hlt_in_fcf);
    }

    bool
    sysmon::firmwareIsHltHardwareTest()
    {
        return(firmwareType()==RORC_CFG_PROJECT_hwtest);
    }

    const char*
    sysmon::firmwareDescription()
    {
        switch( firmwareType() )
        {
            case RORC_CFG_PROJECT_hlt_in:
                return "HLT_IN";
            case RORC_CFG_PROJECT_hlt_out:
                return "HLT_OUT";
            case RORC_CFG_PROJECT_hlt_in_fcf:
                return "HLT_IN_FCF";
            case RORC_CFG_PROJECT_hwtest:
                return "HWTEST";
            case RORC_CFG_PROJECT_ibert:
                return "IBERT";
            default:
                return "UNKNOWN";
        }
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


    uint32_t
    sysmon::numberOfChannels()
    {
        return(m_bar->get32(RORC_REG_TYPE_CHANNELS) & 0xffff);
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
        double clk_period = 0.000000004; // 250 MHz, 4 ns
        uint32_t pulses_per_rotation = 3; // SEPA Fan - Radian Fan has 4
        return 60/((fanctrl & 0x1fffffff)*clk_period*pulses_per_rotation);
    }



    void
    sysmon::systemFanSetEnable
    (
        uint32_t ovrd,
        uint32_t enable
    )
    {
        /** get current settings */
        uint32_t fanctrl = m_bar->get32(RORC_REG_FAN_CTRL);
        if( ovrd )
        {
            // drive PWM_EN_T low
            fanctrl &= ~(1<<30);
            // clear previous 'enable' value
            fanctrl &= ~(1<<31);
            // set new 'enable' value
            fanctrl |= ((enable&1)<<31);
        }
        else
        {
            // drive PWM_EN_T high, enable auto controls
            fanctrl |= (1<<30);
        }

        /** write back new settings */
        m_bar->set32(RORC_REG_FAN_CTRL, fanctrl);
    }

    bool
    sysmon::systemFanIsAutoMode()
    {
        uint32_t fanctrl = m_bar->get32(RORC_REG_FAN_CTRL);
        return ((fanctrl & (1<<30))!=0);
    }



    uint64_t
    sysmon::uptimeSeconds()
    {
        /** uptime is counter value times 134ms (Gen2) or 268ms (Gen1) */
        return (uint64_t)(m_bar->get32(RORC_REG_UPTIME) *
                0.268435456 / pcieGeneration());
    }

    uint32_t
    sysmon::dipswitch()
    {
        return ((m_bar->get32(RORC_REG_UC_SPI_CTRL)>>16) & 0xff);
    }



    /** QSFP Monitoring ***********************************************/



    bool sysmon::qsfpIsPresent
    (
        uint8_t index
    )
    {
        uint32_t qsfp_ctrl = m_bar->get32(RORC_REG_QSFP_LED_CTRL);

        if( ((~qsfp_ctrl)>>(8*index+2) & 0x01) == 1 )
        {
            return true;
        }

        return false;
    }



    bool sysmon::qsfpGetReset
    (
        uint8_t index
    )
    {
        uint32_t qsfp_ctrl = m_bar->get32(RORC_REG_QSFP_LED_CTRL);

        if( ((~qsfp_ctrl)>>(8*index+3) & 0x01) == 1 )
        {
            return true;
        }

        return false;
    }



    void sysmon::qsfpSetReset
    (
        uint8_t index,
        uint8_t reset
    )
    {
        uint32_t qsfp_ctrl = m_bar->get32(RORC_REG_QSFP_LED_CTRL);

        /** QSFP Reset Bit is active LOW */
        if ( reset==0 )
        {
            /** set MOD_RST_N=1 */
            qsfp_ctrl |= (1<<(8*index+3));
        }
        else
        {
            /** set MOD_RST_N=0 */
            qsfp_ctrl &= ~(1<<(8*index+3));
        }

        /** write new value back */
        m_bar->set32(RORC_REG_QSFP_LED_CTRL, qsfp_ctrl);
    }



    bool sysmon::qsfpLEDIsOn
    (
        uint8_t qsfp_index,
        uint8_t LED_index
    )
    {
        uint32_t qsfp_ctrl = m_bar->get32(RORC_REG_QSFP_LED_CTRL);

        if( ((~qsfp_ctrl)>>(8*qsfp_index+LED_index) & 0x01) == 1 )
        {
            return true;
        }

        return false;
    }



    std::string*
    sysmon::qsfpVendorName
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        return( qsfp_i2c_string_readout(index, 148, 163) );
    }



    std::string*
    sysmon::qsfpPartNumber
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        return( qsfp_i2c_string_readout(index, 168, 183) );
    }



    std::string*
    sysmon::qsfpSerialNumber
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        return( qsfp_i2c_string_readout(index, 196, 211) );
    }



    std::string*
    sysmon::qsfpRevisionNumber
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        return( qsfp_i2c_string_readout(index, 184, 185) );
    }



    float
    sysmon::qsfpTemperature
    (
        uint8_t index
    )
    {
        uint8_t data_r;
        data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, 23);

        uint32_t temp = data_r;
        data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, 22);

        temp += ((uint32_t)data_r<<8);

        return ((float)temp/256);
    }



    float
    sysmon::qsfpVoltage
    (
        uint8_t index
    )
    {
        /** voltage is 16bit unsigned integer (0 to 65535) with
         * LSB equal to 100 uVolt. This gives a range of 0-6.55 Volts. */
        uint16_t voltage = (i2c_read_mem(index, QSFP_I2C_SLVADDR, 26)<<8) |
            i2c_read_mem(index, QSFP_I2C_SLVADDR, 27);
        return voltage/10000.0;
    }



    float
    sysmon::qsfpTxBias
    (
        uint8_t index,
        uint8_t channel
    )
    {
        /** TX bias current is a 16b uint16_t
         * with LSB equal to 2 uA */
        uint16_t ubias = (i2c_read_mem(index, QSFP_I2C_SLVADDR, 42+2*channel)<<8) |
            i2c_read_mem(index, QSFP_I2C_SLVADDR, 43+2*channel);
        /** return in mA */
        return ubias*0.002;
    }



    float
    sysmon::qsfpRxPower
    (
        uint8_t index,
        uint8_t channel
    )
    {
        /** RX received optical power is a 16b uint16_t
         * with LSB equal to 0.1 uWatt */
        uint16_t upower = (i2c_read_mem(index, QSFP_I2C_SLVADDR, 34+2*channel)<<8) |
            i2c_read_mem(index, QSFP_I2C_SLVADDR, 35+2*channel);
        /** return in mWatt */
        return upower/10000.0;
    }



    float
    sysmon::qsfpWavelength
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);

        /** Wavelengthis provided as uint16_t
         * with LSB equal to 0.05 nm */
        uint16_t uwl = (i2c_read_mem(index, QSFP_I2C_SLVADDR, 186)<<8) |
            i2c_read_mem(index, QSFP_I2C_SLVADDR, 187);
        /** return in nm */
        return uwl * 0.05;
    }



    uint8_t
    sysmon::qsfpTxFaultMap
    (
        uint8_t index
    )
    {
        /**
         * Addr 4, Bits [0:3]:
         * [3]: TX4 Fault
         * [2]: TX3 Fault
         * [1]: TX2 Fault
         * [0]: TX1 Fault
         * */
        return i2c_read_mem(index, QSFP_I2C_SLVADDR, 4) & 0x0f;
    }


    uint8_t
    sysmon::qsfpGetRateSelectionSupport
    (
        uint8_t index
    )
    {
        qsfp_select_page0(index);
        /**
         * rate selection support:
         * page 0, byte 221, bits [3:2] and
         * page 0, byte 195, bit 5
         * */
        uint8_t options = i2c_read_mem(index, QSFP_I2C_SLVADDR, 195);
        uint8_t enh_options = i2c_read_mem(index, QSFP_I2C_SLVADDR, 221);
        uint8_t ext_rate_sel = i2c_read_mem(index, QSFP_I2C_SLVADDR, 141);

        /** enh_options[3:2]==0 and options[5]==0 */
        if ( (enh_options & 0x0c)==0x00 && (options & (0x20))==0x00 )
        {
            return LIBRORC_SYSMON_QSFP_NO_RATE_SELECTION;
        }
        /** enh_options[3:2]="10" */
        else if ( (enh_options & 0x0c)==0x08 && (ext_rate_sel & 0x01)==0x01 )
        {
            return LIBRORC_SYSMON_QSFP_EXT_RATE_SELECTION;
        }
        /** enh_options[3:2]="01" */
        else if ( (enh_options & 0x0c)==0x40 )
        {
            return LIBRORC_SYSMON_QSFP_APT_RATE_SELECTION;
        }
        else
        {
            return LIBRORC_SYSMON_QSFP_NO_RATE_SELECTION;
        }
    }


    uint8_t
    sysmon::qsfpPowerClass
    (
        uint8_t index
    )
    {
        /** Extended Identifier Values, Addr=129, Bits 7-6 */
        return (i2c_read_mem(index, QSFP_I2C_SLVADDR, 129)>>6) + 1;
    }



    bool
    sysmon::qsfpHasTXCDR
    (
        uint8_t index
    )
    {
        return ((i2c_read_mem(index, QSFP_I2C_SLVADDR, 129) & (1<<3)) != 0);
    }



    bool
    sysmon::qsfpHasRXCDR
    (
        uint8_t index
    )
    {
        return ((i2c_read_mem(index, QSFP_I2C_SLVADDR, 129) & (1<<2)) != 0);
    }



    /** Error Counters ***************************************************/

    void
    sysmon::clearAllErrorCounters()
    {
        clearPcieRequestCanceledCounter();
        clearPcieTxTimeoutCounter();
        clearPcieIllegalRequestCounter();
        clearPcieMultiDwReadCounter();
        clearPcieTxDestinationBusyCounter();
        clearPcieTransmissionErrorCounter();
    }

    uint32_t
    sysmon::pcieRequestCanceledCounter()
    { return m_bar->get32(RORC_REG_SC_REQ_CANCELED); }

    void
    sysmon::clearPcieRequestCanceledCounter()
    { m_bar->set32(RORC_REG_SC_REQ_CANCELED, 0); }

    uint32_t
    sysmon::pcieTxTimeoutCounter()
    { return m_bar->get32(RORC_REG_DMA_TX_TIMEOUT); }

    void
    sysmon::clearPcieTxTimeoutCounter()
    { m_bar->set32(RORC_REG_DMA_TX_TIMEOUT, 0); }

    uint32_t
    sysmon::pcieIllegalRequestCounter()
    { return m_bar->get32(RORC_REG_ILLEGAL_REQ); }

    void
    sysmon::clearPcieIllegalRequestCounter()
    { m_bar->set32(RORC_REG_ILLEGAL_REQ, 0); }

    uint32_t
    sysmon::pcieMultiDwReadCounter()
    { return m_bar->get32(RORC_REG_MULTIDWREAD); }

    void
    sysmon::clearPcieMultiDwReadCounter()
    { m_bar->set32(RORC_REG_MULTIDWREAD, 0); }

    uint32_t
    sysmon::pcieTxDestinationBusyCounter()
    { return m_bar->get32(RORC_REG_PCIE_DST_BUSY); }

    void
    sysmon::clearPcieTxDestinationBusyCounter()
    { m_bar->set32(RORC_REG_PCIE_DST_BUSY, 0); }

    uint32_t
    sysmon::pcieTransmissionErrorCounter()
    { return m_bar->get32(RORC_REG_PCIE_TERR_DROP); }

    void
    sysmon::clearPcieTransmissionErrorCounter()
    { m_bar->set32(RORC_REG_PCIE_TERR_DROP, 0); }




    /** I2C Low Level Access ***********************************************/


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
                m_i2c_hsmode, /** mode */
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
                m_i2c_hsmode, /** mode */
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
                m_i2c_hsmode, /** mode */
                3); /** byte_enable */

        i2c_wait_for_cmpl();
    }


    void
    sysmon::i2c_set_mode
    (
        uint8_t mode
    )
    {
        if ( mode & (~0x1) )
        {
            throw LIBRORC_SYSMON_ERROR_I2C_INVALID_PARAM;
        }
        m_i2c_hsmode = mode;
    }


    uint8_t
    sysmon::i2c_get_mode()
    {
        return m_i2c_hsmode;
    }



    uint32_t
    sysmon::ddr3DataReplayEventToRam
    (
         uint32_t *event_data,
         uint32_t num_dws,
         uint32_t ddr3_start_addr,
         uint8_t channel,
         bool last_event
    )
    {
        uint32_t *dataptr = event_data;
        uint32_t addr = ddr3_start_addr;
        uint16_t mask = 0x7fff;
        uint32_t flags = 0;

        while ( num_dws>0 )
        {
            if ( num_dws<=15 )
            {
                mask = (1<<num_dws)-1;
                flags |= DATA_REPLAY_EOE;
                if (last_event)
                {
                    flags |= DATA_REPLAY_END;
                }
            }

            ddr3DataReplayBlockToRam( addr, dataptr, mask, channel, flags);
            addr += 8;
            dataptr += 15;
            num_dws = (num_dws<15) ? 0 : (num_dws-15);
        }
        return addr;
    }

    uint32_t
    sysmon::ddr3Bitrate
    (
        uint32_t controller
    )
    {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        /** C0: bits [10:9], C1: bits [26:25] */
        switch( (ddr3ctrl>>(16*(controller&1)+9)) & 3 )
        {
            case 1:
                return 606;
            case 2:
                return 800;
            case 3:
                return 1066;
            default: // controller not implemented
                return 0;
        }
    }

    uint32_t
    sysmon::ddr3ControllerMaxModuleSize
    (
        uint32_t controller
    )
    {
        uint32_t i = (controller&1);
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);

        /**
         * get controller address width encoded as (32-regval)
         * C0: bits [13:11], C1: bits [29:27]
         **/
        uint32_t addr_width = (32-((ddr3ctrl>>(16*i+11)) & 0x7));

        /**
         * number of ranks:
         * C0: bit[14], C1: bit[30]
         * 0 => single ranked, 1 => dual ranked
         **/
        uint32_t nranks = (((ddr3ctrl>>(14+16*i))&1)+1);

        /**
         * highest address bit cannot be counted on single
         * ranked modules
         **/
        if( nranks==1 )
        { addr_width -= 1; }

        return 8*(1<<addr_width);
    }



    bool
    sysmon::ddr3ModuleInitReady
    (
        uint32_t controller
    )
    {
        /** C0 status in bits [15:0], C1 status in bits [31:16] */
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        uint16_t controller_status =
            ( (ddr3ctrl>>(16*(controller&1))) & 0xffff );

        if( (controller_status & (3<<9)) == 0)
        {
            /** bitrate==0 => controller not implemented in firmware */
            return false;
        }

        uint16_t checkmask = (1<<0) | // module reset
            (1<<1) | // PHY init done
            (1<<2) | // PLL lock
            (1<<3) | // read levelling started
            (1<<4) | // read levelling done
            (1<<5) | // read levelling error
            (1<<7) | // write levelling done
            (1<<8); // write levelling error
        uint16_t expected_value = (0<<0) | // not in reset
            (1<<1) | // PHY init done
            (1<<2) | // PLL locked
            (1<<3) | // read levelling started
            (1<<4) | // read levelling done
            (0<<5) | // no read levelling error
            (1<<7) | // write levelling done
            (0<<8); // no write levelling error

        return ( (controller_status & checkmask) == expected_value );
    }


    uint8_t
    sysmon::ddr3SpdRead
    (
        uint8_t module,
        uint8_t address
    )
    {
        uint8_t rdval = i2c_read_mem(
                3, // DDR3 is chain 3
                DDR3_SPD_SLVADDR + (module&1),
                address);
        return rdval;
    }

    std::string
    sysmon::ddr3SpdReadString
    (
        uint8_t module,
        uint8_t start_address,
        uint8_t end_address
    )
    {
        std::string readout = "";
        char data_r = 0;
        for( uint8_t addr=start_address; addr<=end_address; addr++ )
        {
            data_r = ddr3SpdRead(module, addr);
            readout.append(1, data_r);
        }
        return readout;
    }




    /** Protected ***** ***********************************************/

    void
    sysmon::ddr3DataReplayBlockToRam
    (
        uint32_t start_addr,
        uint32_t *data,
        uint16_t mask,
        uint8_t channel,
        uint32_t flags
    )
    {
        if ( channel>11 )
        {
            throw LIBRORC_SYSMON_ERROR_DATA_REPLAY_INVALID;
        }

        uint32_t controller_select = (channel>6) ? 1 : 0;
        uint8_t channel_select = controller_select ?
            (1<<(channel-6)) : (1<<channel);

        /**
         * a replay block consists of 16 DWs:
         * - DW0 is a header
         * - DW1-DW15 are data words
         * fill block header, then copy 15 datawords to block buffer
         **/
        uint32_t block_buffer[16];
        block_buffer[0] = ((uint32_t)mask<<16) | flags | channel_select;
        for ( int i=0; i<15; i++)
        {
            if ( mask & (1<<i) )
            {
                block_buffer[i+1]=data[i];
            }
        }

        /** copy block_buffer to onboard registers */
        m_bar->memcopy(RORC_REG_DATA_REPLAY_PAYLOAD_BASE,
                block_buffer, 16*sizeof(uint32_t));

        /**
         * make the RORC write the onboard buffer to the selected
         * start address in DDR3 RAM
         **/
        uint32_t drctrl = (start_addr & 0x7ffffffc);
        drctrl |= (1<<controller_select); // set destination controller
        m_bar->set32(RORC_REG_DATA_REPLAY_CTRL, drctrl);

        /** wait for write_done */
        uint32_t timeout = LIBRORC_SYSMON_DR_TIMEOUT;
        while( !(m_bar->get32(RORC_REG_DATA_REPLAY_CTRL) &
                LIBRORC_DATA_REPLAY_WRITE_DONE) )
        {
            if (timeout==0)
            { throw LIBRORC_SYSMON_ERROR_DATA_REPLAY_TIMEOUT; }
            timeout--;
            usleep(100);
        }
    }



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



    std::string*
    sysmon::qsfp_i2c_string_readout
    (
        uint8_t index,
        uint8_t start,
        uint8_t end
    )
    {
        std::string *readout = new std::string();
        char data_r = 0;
        for(uint8_t i=start; i<=end; i++)
        {
            data_r = i2c_read_mem(index, QSFP_I2C_SLVADDR, i);
            readout->append(1, data_r);
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
