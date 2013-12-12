/**
 * @file boardtest_modules.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-12-09
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
#include "boardtest_modules.hh"
#include "../dma/dma_handling.hh"


void
checkDdr3ModuleSpd
(
    librorc::sysmon *sm,
    int module_id,
    int verbose
)
{
    try
    {
        uint8_t module_type = sm->i2c_read_mem(3, 0x50+module_id, 0x03);
        if ( module_type != 0x03 )
        {
            cout << "ERROR: Unexpected DDR3 module type in socket"
                << module_id << ": " << HEXSTR((int)module_type, 1) << endl;
        } else if ( verbose )
        {
            cout << "DDR3 module type in socket"
                << module_id << ": " << HEXSTR((int)module_type, 1) << endl;
        }
    }
    catch(...)
    {
        cout << "ERROR: Failed to read from DDR3 Module "
            << module_id << " SPD - Is DDR3 module present?" << endl;
    }
}


void
checkDdr3ModuleCalib
(
    librorc::bar *bar,
    int module_id
)
{
    /** check read/write levelling status */
    uint32_t ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);
    if ((ddrctrl>>(16*module_id))&1) 
    { 
        cout << "WARNING: DDR3 module "
             << module_id << " in reset!" << endl;
    }
    if ( !((ddrctrl>>(16*module_id+2))&1) )
    {
        cout << "ERROR: DDR3 module "
             << module_id << "Phy initialization not done!" << endl;
    }
    if ( ((ddrctrl>>(16*module_id+6))&3) != 3 )
    {
        cout << "WARNING: DDR3 module "
             << module_id << "read levelling not done!" << endl;
    }
    if ( ((ddrctrl>>(16*module_id+8))&3) != 0 )
    {
        cout << "ERROR: DDR3 module "
             << module_id << "read levelling failed!" << endl;
    }
    if ( ((ddrctrl>>(16*module_id+11))&1) != 1 )
    {
        cout << "ERROR: DDR3 module "
             << module_id << "write levelling not done!" << endl;
    }
    if ( ((ddrctrl>>(16*module_id+12))&1) != 0 )
    {
        cout << "ERROR: DDR3 module "
             << module_id << "write levelling failed!" << endl;
    }
}


void
checkDdr3ModuleTg
(
    librorc::bar *bar,
    int module_id
)
{
    uint32_t ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);
    if ( ((ddrctrl>>(16*module_id+13))&3) != 0 )
    {
        cout << "ERROR: DDR3 module "
             << module_id << "Traffic Generator Error!" << endl;
    }
}



void
checkMicrocontroller
(
    librorc::bar *bar,
    int verbose
)
{
    int uc_avail = 1;

    /** read uc signature */
    librorc::microcontroller *uc;
    try
    {
        uc = new librorc::microcontroller(bar);
    }
    catch(...)
    {
        cout << "ERROR: failed to init microcontroller" << endl;
        uc_avail = 0;
    }

    if ( uc_avail )
    {
        try
        {
            uc->configure_spi();
            uc->set_reset(1);
            uc->enter_prog_mode();

            uint32_t sig = uc->read_device_signature();
            if ( sig != UC_DEVICE_SIGNATURE )
            {
                cout << "ERROR: unexpected microcontroller signature "
                     << HEXSTR(sig, 6) << " - expected "
                     << HEXSTR(UC_DEVICE_SIGNATURE, 6) << endl;
            } else if ( verbose )
            {
                cout << "Microcontroller signature: "
                     << HEXSTR(sig, 6) << endl;
            }

            uc->set_reset(0);
        }
        catch (int e)
        {
            cout << "ERROR: failed to access microcontroller via SPI: ";
            switch(e)
            {
                case LIBRORC_UC_SPI_NOT_IMPLEMENTED:
                    cout << "SPI not implemented";
                    break;
                case LIBRORC_UC_SEND_CMD_TIMEOUT:
                    cout << "Timeout sending command";
                    break;
                case LIBRORC_UC_PROG_MODE_FAILED:
                    cout << "Failed to enter programming mode";
                    break;
                default:
                    cout << "Unknown error " << e;
                    break;
            }
            cout << endl;
        }
        delete uc;
    }
}


void
checkLvdsTester
(
    librorc::bar *bar
)
{
    uint32_t lvdsctrl = bar->get32(RORC_REG_LVDS_CTRL);
    if ( lvdsctrl&(1<<31) || !(lvdsctrl&1) )
    {
        cout << "WARNING: LVDS Tester disabled or in reset state" << endl;
    } else
    {
        if ( !(lvdsctrl & (1<<1)) )
        {
            cout << "ERROR: LVDS via RJ45 Link0 DOWN" << endl;
        }
        if ( !(lvdsctrl & (1<<2)) )
        {
            cout << "ERROR: LVDS via RJ45 Link1 DOWN" << endl;
        }
    }
}


void
checkFlash
(
    librorc::device *dev,
    int chip_select,
    int verbose
)
{
    librorc::bar *bar = NULL;
    try
    {
#ifdef SIM
        bar = new librorc::sim_bar(dev, 0);
#else
        bar = new librorc::rorc_bar(dev, 0);
#endif
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR0" << endl;
        abort();
    }

    librorc::flash *flash = NULL;

    try
    {
        flash = new librorc::flash(bar, chip_select, LIBRORC_VERBOSE_OFF);
    }
    catch(...)
    {
        cout << "Flash " << chip_select << " init failed" << endl;
    }

    /** set asynchronous read mode */
    flash->setConfigReg(0xbddf);

    uint16_t status = flash->resetChip();
    if ( status )
    {
        cout << "resetChip failed: " << HEXSTR(status, 4) << endl;
        //optional: decode status value to specific error
    }

    uint16_t mcode = flash->getManufacturerCode();
    if ( mcode != 0x0049 )
    {
        cout << "ERROR: invalid Manufacturer Code "
            << HEXSTR(mcode, 4) << " for flash " << chip_select << endl;
    } else if ( verbose )
    {
        cout << "Flash " << chip_select << " Status: "
            << HEXSTR(mcode, 4) << endl;
    }

    delete flash;
    delete bar;
}


void
checkRefClkGen
(
    librorc::sysmon *sm,
    int verbose
)
{
    /** check current Si570 refclk setings */
    librorc::refclk *rc;
    try
    {
        rc = new librorc::refclk(sm);
    }
    catch(...)
    {
        cout << "Refclk init failed!" << endl;
        delete sm;
        abort();
    }

    try
    {
        /** get current configuration */
        refclkopts opts = rc->getCurrentOpts( LIBRORC_REFCLK_DEFAULT_FOUT );

        double cur_freq = opts.fxtal * opts.rfreq_float /
            (opts.hs_div * opts.n1);
        if ( cur_freq < REFCLK_FREQ_MIN || cur_freq > REFCLK_FREQ_MAX )
        {
            cout << "ERROR: Si570 Reference Clock out of bounds: "
                << cur_freq << " MHz" << endl;
        } else if ( verbose )
        {
            cout << "Reference Clock Frequency: "
                 << cur_freq << " MHz" << endl;
        }
    }
    catch(...)
    {
        cout << "ERROR: failed to read Si570 Reference Clock configuration"
             << endl;
    }

    delete rc;
}


void
checkQsfp
(
    librorc::sysmon *sm,
    int module_id,
    int verbose
)
{
    if ( !sm->qsfpIsPresent(module_id) )
    {
        cout << "WARNING: No QSFP detected in slot "
            << module_id << " - skipping some tests..." << endl;
    }
    else if ( sm->qsfpGetReset(module_id) )
    {
        cout << "WARNING: QSFP " << module_id
            << " seems to be in RESET state - "
            << " skipping some tests" << endl;
    }
    else
    {
        checkQsfpTemperature( sm, module_id, verbose );
        checkQsfpVcc( sm, module_id, verbose );
        checkQsfpOpticalLevels( sm, module_id, verbose );
    }
}



void
checkQsfpTemperature
(
    librorc::sysmon *sm,
    int module_id,
    int verbose
)
{
    float qsfptemp = sm->qsfpTemperature(module_id);
    if ( qsfptemp < QSFP_TEMP_MIN || qsfptemp > QSFP_TEMP_MAX )
    {
        cout << "ERROR: QSFP " << module_id
            << " Temperature out of bounds: "
            << qsfptemp << endl;
    } else if ( verbose )
    {
        cout << "QSFP " << module_id
            << " Temperature: " << qsfptemp << " degC" << endl;
    }
}


void
checkQsfpVcc
(
    librorc::sysmon *sm,
    int module_id,
    int verbose
)
{
    float qsfpvcc = sm->qsfpVoltage(module_id);
    if ( qsfpvcc < QSFP_VCC_MIN || qsfpvcc > QSFP_VCC_MAX )
    {
        cout << "ERROR: QSFP " << module_id
            << " VCC out of bounds: " << qsfpvcc << endl;
    } else if ( verbose )
    {
        cout << "QSFP " << module_id
            << " VCC: " << qsfpvcc << " V" << endl;
    }
}


void
checkQsfpOpticalLevels
(
    librorc::sysmon *sm,
    int module_id,
    int verbose
)
{
    /** QSFP RX Power */
    for (int j=0; j<4; j++)
    {
        float rxpower = sm->qsfpRxPower(module_id, j);
        if ( rxpower < QSFP_RXPOWER_MIN || rxpower > QSFP_RXPOWER_MAX )
        {
            cout << "WARNING: QSFP " << module_id << " Channel " << j
                << " (Link " << setw(2) << 4*module_id+j
                << ") RX Power out of bounds: "
                << rxpower << " mW" << endl;
        } else if ( verbose )
        {
            cout << "QSFP " << module_id  << " Channel " << j
                << " (Link " << setw(2) << 4*module_id+j << ") RX Power: "
                << rxpower << " mW" << endl;
        }
    }

    /** QSFP TX Bias */
    for (int j=0; j<4; j++)
    {
        float txbias = sm->qsfpTxBias(module_id, j);
        if ( txbias < QSFP_TXBIAS_MIN || txbias > QSFP_TXBIAS_MAX )
        {
            cout << "WARNING: QSFP " << module_id << " Channel " << j
                << " (Link " << setw(2) << 4*module_id+j
                << ") TX Bias out of bounds: "
                << txbias << " mA" << endl;
        } else if ( verbose )
        {
            cout << "QSFP " << module_id  << " Channel " << j
                << " (Link " << setw(2) << 4*module_id+j << ") TX bias: "
                << txbias << " mA" << endl;
        }
    }
}


void
checkFpgaSystemMonitor
(
    librorc::sysmon *sm,
    int verbose
)
{
    /** check FPGA Temp */
    float fpgatemp = sm->FPGATemperature();
    if ( fpgatemp < FPGA_TEMP_MIN || fpgatemp > FPGA_TEMP_MAX )
    {
        cout << "ERROR: FPGA Temperature out of bounds: "
            << fpgatemp << endl;
    } else if ( verbose )
    {
        cout << "FPGA Temp: " << fpgatemp << " degC" << endl;
    }

    /** check FPGA VCCINT*/
    float vccint = sm->VCCINT();
    if ( vccint < FPGA_VCCINT_MIN || vccint > FPGA_VCCINT_MAX )
    {
        cout << "ERROR: FPGA VCCINT out of bounds: "
            << vccint << endl;
    } else if ( verbose )
    {
        cout << "FPGA VCCINT: " << vccint << "V" << endl;
    }

    /** check FPGA VCCAUX*/
    float vccaux = sm->VCCAUX();
    if ( vccaux < FPGA_VCCAUX_MIN || vccaux > FPGA_VCCAUX_MAX )
    {
        cout << "ERROR: FPGA VCCINT out of bounds: "
            << vccaux << endl;
    } else if ( verbose )
    {
        cout << "FPGA VCCAUX: " << vccaux << "V" << endl;
    }
}


void
checkPcieState
(
    librorc::sysmon *sm
)
{
    uint32_t pcie_gen = sm->pcieGeneration();
    if ( pcie_gen != 2 )
    {
        cout << "WARNING: Detected as PCIe Gen" << pcie_gen
             << ", expexted Gen2" << endl;
    }

    uint32_t pcie_nlanes = sm->pcieNumberOfLanes();
    if ( pcie_nlanes != 8 )
    {
        cout << "WARNING: Expected 8 lanes PCIe, got "
             << pcie_nlanes << endl;
    }
}


void
checkFpgaFan
(
    librorc::sysmon *sm,
    int verbose
)
{
    if ( !sm->systemFanIsRunning() )
    {
        cout << "ERROR: FPGA fan seems to be stopped " << endl;
    }
    else
    {
        double fanspeed = sm->systemFanSpeed();
        if ( fanspeed < FPGA_FANSPEED_MIN || fanspeed > FPGA_FANSPEED_MAX )
        {
            cout << "ERROR: FPGA fan speed out of bounds: "
                << fanspeed << " RPM" << endl;
        } else if ( verbose )
        {
            cout << "Fan: " << fanspeed << " RPM" << endl;
        }
    }
}


int
checkSysClkAvailable
(
    librorc::sysmon *sm
)
{
    int clk_avail = 1;
    if ( !sm->systemClockIsRunning() )
    {
        cout << "ERROR: 200 MHz system clock not detected "
             << "- have to skip some test now..." << endl;
        clk_avail = 0;
    }
    return clk_avail;
}


void
    checkCount
(
 uint32_t channel_id,
 uint32_t count,
 const char *name
 )
{
    if ( count != 0 )
    {
        cout << "ERROR: Link channel " << channel_id
             << " " << name << " count was "
             << HEXSTR(count, 8) << endl;
    }
}



void
checkLinkState
( 
    librorc::link *link,
    uint32_t channel_id
)
{
    if (link->isGtxDomainReady())
    {
        uint32_t ddlctrl = link->GTX(RORC_REG_DDL_CTRL);
        cout << HEXSTR(ddlctrl, 8) << endl;
        /** TODO: change to GTX_ASYNC_STS::READY */
        if ( !((ddlctrl>>5)&1) )
        {
            cout << "ERROR: Link channel " << channel_id
                 << " is down!" << endl;
        }
        else
        {
            checkCount(channel_id, link->GTX(RORC_REG_GTX_DISPERR_CNT),
                    "Disperity Error");
            checkCount(channel_id, link->GTX(RORC_REG_GTX_RXNIT_CNT),
                    "RX-Not-In-Table Error");
            checkCount(channel_id, link->GTX(RORC_REG_GTX_RXLOS_CNT),
                    "RX-Not-Loss-Of-Signal");
            checkCount(channel_id, link->GTX(RORC_REG_GTX_RXBYTEREALIGN_CNT),
                    "RX-Byte-Realign");
            checkCount(channel_id, link->GTX(RORC_REG_GTX_ERROR_CNT),
                    "LinkTester Error");
        }
    }
    else
    {
        cout << "ERROR: No clock on channel " << channel_id << "!" << endl;
    }
}



uint64_t
eventCallBack
(
    void                     *userdata,
    uint64_t                  event_id,
    librorc_event_descriptor  report,
    const uint32_t           *event,
    librorcChannelStatus     *channel_status
)
{
    librorc::event_sanity_checker *checker = (librorc::event_sanity_checker*)userdata;

    try{ checker->check(report, channel_status, event_id); }
    catch(...){ abort(); }
    return 0;
}

void
testDmaChannel
(
    librorc::device *dev,
    librorc::bar *bar,
    int timeout
)
{
    DMAOptions opts;
    opts.deviceId  = dev->getDeviceId();
    opts.channelId = 0;
    opts.eventSize = 0x1000;
    opts.useRefFile = false;
    opts.esType = LIBRORC_ES_IN_HWPG;

    char logdirectory[] = "/tmp";
    
    librorc_event_callback event_callback = eventCallBack;
    librorc::link *link = new librorc::link(bar, opts.channelId);

    librorc::event_stream *eventStream;
    cout << "Prepare ES" << endl;
    if( !(eventStream = prepareEventStream(dev, bar, opts)) )
    { exit(-1); }
    eventStream->setEventCallback(event_callback);

    /** enable EBDM + RBDM + PKT */
    link->setPacketizer(RORC_REG_DMA_CTRL, 
            (link->packetizer(RORC_REG_DMA_CTRL) | 0x0d) );

    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE|CHK_PATTERN|CHK_ID;

    librorc::event_sanity_checker checker = 
        librorc::event_sanity_checker
        (
         eventStream->m_eventBuffer,
         opts.channelId,
         PG_PATTERN_INC,
         sanity_check_mask,
         logdirectory
        );
    
    /** Capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;
    
    while( gettimeofdayDiff(last_time, current_time) < timeout )
    {
        eventStream->handleChannelData( (void*)&(checker) );
        eventStream->m_bar1->gettime(&current_time, 0);
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(
            eventStream->m_channel_status,
            start_time,
            end_time);

    /** Cleanup */
    delete eventStream;
}

