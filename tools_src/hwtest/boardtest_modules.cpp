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
printHeader
(
    const char *title
)
{
    cout << endl << "--===== " << title << " =====--" << endl;
}


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
            cout << "INFO: Found DDR3 SO-DIMM in Socket "
                << module_id << endl;
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
    int module_id,
    int verbose
)
{
    /** check read/write leveling status */
    uint32_t ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);
    bool mod_reset = ((ddrctrl>>(16*module_id))&1);
    bool phy_init_done = ((ddrctrl>>(16*module_id+2))&1);
    bool rd_lvl_done = (((ddrctrl>>(16*module_id+6))&3)==3);
    bool rd_lvl_failed = (((ddrctrl>>(16*module_id+8))&3) != 0);
    bool wr_lvl_done = ((ddrctrl>>(16*module_id+11))&1);
    bool wr_lvl_failed = (((ddrctrl>>(16*module_id+12))&1) != 0);

    if (mod_reset) 
    { 
        cout << "WARNING: DDR3 module "
             << module_id << " in reset!" << endl;
    }
    if ( !phy_init_done )
    {
        cout << "ERROR: DDR3 module "
             << module_id << " Phy initialization not done!" << endl;
    }
    if ( !rd_lvl_done )
    {
        cout << "ERROR: DDR3 module "
             << module_id << " read leveling not done!" << endl;
    }
    if ( rd_lvl_failed )
    {
        cout << "ERROR: DDR3 module "
             << module_id << " read leveling failed!" << endl;
    }
    if ( !wr_lvl_done )
    {
        cout << "ERROR: DDR3 module "
             << module_id << " write leveling not done!" << endl;
    }
    if ( wr_lvl_failed )
    {
        cout << "ERROR: DDR3 module "
             << module_id << " write leveling failed!" << endl;
    }

    if ( verbose )
    {
        if ( !mod_reset && phy_init_done && rd_lvl_done && wr_lvl_done )
        {
            cout << "INFO: DDR3 module "
             << module_id << " Calibration successful!" << endl;
        }
    }
}


void
checkDdr3ModuleTg
(
    librorc::bar *bar,
    int module_id,
    int verbose
)
{
    uint32_t ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);
    bool tg_error = ( ((ddrctrl>>(16*module_id+13))&3) != 0 );
    bool tg_running = ((ddrctrl>>(16*module_id+2))&1)==1; // phy_init_done

    if ( tg_error and tg_running )
    {
        cout << "ERROR: DDR3 module "
             << module_id << " Traffic Generator Error!" << endl;
    } else if (!tg_running)
    {
        cout << "ERROR: DDR3 module "
             << module_id << " Traffic Generator not running!" << endl;
    }
    else if (verbose)
    {
        cout << "INFO: DDR3 module "
             << module_id << " Traffic Generator OK." << endl;
    }
}



void
checkMicrocontroller
(
    librorc::bar *bar,
    int firstrun,
    int verbose
)
{
    int uc_avail = 1;

    /** get DIP switch setting */
    uint32_t dipswitch = bar->get32(RORC_REG_UC_CTRL)>>24;
    if (dipswitch==0 || dipswitch>0x7f)
    {
        cout << "ERROR: invalid dip switch setting "
             << HEXSTR(dipswitch, 1)
             << " - SMBus access is likely to fail!"
             << endl;
    }
    else
    {
        cout << "INFO: uC using slave address "
             << HEXSTR(dipswitch, 1) << endl;
    }

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
        uc->configure_spi();

        try
        {
            uc->set_reset(1);
            uc->enter_prog_mode();

            uint32_t sig = uc->read_device_signature();
            if ( sig != UC_DEVICE_SIGNATURE )
            {
                cout << "ERROR: unexpected microcontroller signature "
                     << HEXSTR(sig, 6) << " - expected "
                     << HEXSTR(UC_DEVICE_SIGNATURE, 6) << endl;
            } else 
            {
                if ( verbose )
                {
                    cout << "INFO: Microcontroller signature: "
                        << HEXSTR(sig, 6) << endl;
                }
            }
            uc->set_reset(0);

            if ( firstrun )
            {

                char *fwpath = getenv("LIBRORC_FIRMWARE_PATH");
                if (!fwpath)
                {
                    cout << "WARNING: Environment variable "
                        << "LIBRORC_FIRMWARE_PATH is not set - "
                        << "will not program microcontroller!" << endl;
                } else {

                    char *fwfile = (char *)malloc(strlen(fwpath) +
                            strlen("/crorc_ucfw_latest.bin") + 1);
                    sprintf(fwfile, "%s/crorc_ucfw_latest.bin", fwpath);

                    if ( verbose )
                    {
                        cout << "INFO: trying to program uC with " << fwfile
                            << endl;
                    }
                    uc->programFromFile(fwfile);
                }
            }

        }
        catch (int e)
        {
            switch(e)
            {
                case LIBRORC_UC_FILE_ERROR:
                    cout << "ERROR: loading file failed";
                    break;
                case LIBRORC_UC_SPI_NOT_IMPLEMENTED:
                    cout << "ERROR: uC SPI not implemented";
                    break;
                case LIBRORC_UC_SEND_CMD_TIMEOUT:
                    cout << "ERROR: uC Timeout sending command";
                    break;
                case LIBRORC_UC_PROG_MODE_FAILED:
                    cout << "ERROR: Failed to enter uC programming mode";
                    break;
                default:
                    cout << "ERROR: Unknown uC error " << e;
                    break;
            }
            cout << endl;
        }

        uc->unconfigure_spi();
        
        delete uc;
    }
}


void
checkLvdsTester
(
    librorc::bar *bar,
    int verbose
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
        else if ( verbose )
        {
            cout << "INFO: LVDS Link0 up and running!" << endl;
        }

        if ( !(lvdsctrl & (1<<2)) )
        {
            cout << "ERROR: LVDS via RJ45 Link1 DOWN" << endl;
        }
        else if ( verbose )
        {
            cout << "INFO: LVDS Link1 up and running!" << endl;
        }
    }
}


bool
checkFlashManufacturerCode
(
    librorc::flash *flash,
    int verbose
)
{
    uint16_t mcode = flash->getManufacturerCode();
    if ( mcode != 0x0049 )
    {
        cout << "ERROR: Flash " << flash->getChipSelect()
             << "invalid Manufacturer Code "
             << HEXSTR(mcode, 4) << endl;
    } else if ( verbose )
    {
        cout << "INFO: Flash " << flash->getChipSelect() << " Status: "
            << HEXSTR(mcode, 4) << endl;
    }
    return (mcode==0x0049);
}




uint64_t
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
        cout << "FATAL: failed to initialize BAR0" << endl;
        abort();
    }

    librorc::flash *flash = NULL;

    try
    {
        flash = new librorc::flash(bar, chip_select, LIBRORC_VERBOSE_OFF);
    }
    catch(...)
    {
        cout << "FATAL: Flash " << chip_select << " init failed" << endl;
        abort();
    }

    /** set asynchronous read mode */
    flash->setConfigReg(0xbddf);

    uint16_t status = flash->resetChip();
    if ( status )
    {
        cout << "ERROR Flash " << chip_select
             << "resetChip failed: " << HEXSTR(status, 4) << endl;
    }

    uint64_t devnr = 0;
    if ( checkFlashManufacturerCode(flash, verbose) )
    {
        devnr = flash->getUniqueDeviceNumber();
    }

    delete flash;
    delete bar;

    return devnr;
}


bool
flashDeviceNumbersAreValid
(
    uint64_t devnr0,
    uint64_t devnr1
)
{
    bool valid = false;
    if ( devnr0 == 0 || devnr0 == 0xfffffffff )
    {
        cout << "ERROR: invalid unique device number for Flash0: "
             << HEXSTR(devnr0, 16) << endl;
    }
    else if ( devnr1 == 0 || devnr1 == 0xfffffffff )
    {
        cout << "ERROR: invalid unique device number for Flash1: "
             << HEXSTR(devnr1, 16) << endl;
    }
    else if ( devnr0==devnr1 )
    {
        cout << "ERROR: both flashed reported the same "
             << "'unique' device number: " << devnr0
             << endl;
    }
    else
    {
        valid = true;
    }

    cout << "INFO: Flash 0 unique device number: "
         << HEXSTR(devnr0, 16) << endl;
    cout << "INFO: Flash 1 unique device number: "
         << HEXSTR(devnr1, 16) << endl;

    return valid;
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
        cout << "FATAL: Refclk init failed!" << endl;
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
            cout << "INFO: Reference Clock Frequency: "
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


bool
checkQsfp
(
    librorc::sysmon *sm,
    int module_id,
    int verbose
)
{
    bool qsfp_ready = false;
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
        qsfp_ready = true;
    }
    return qsfp_ready;
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
        cout << "ERROR: QSFP" << module_id
            << " Temperature out of bounds: "
            << qsfptemp << endl;
    } else if ( verbose )
    {
        cout << "INFO: QSFP" << module_id
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
        cout << "ERROR: QSFP" << module_id
            << " VCC out of bounds: " << qsfpvcc << endl;
    } else if ( verbose )
    {
        cout << "INFO QSFP" << module_id
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
            cout << "WARNING: QSFP" << module_id << " Channel " << j
                << " (Link " << setw(2) << 4*module_id+j
                << ") RX Power out of bounds: "
                << rxpower << " mW" << endl;
        } else if ( verbose )
        {
            cout << "INFO: QSFP" << module_id  << " Channel " << j
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
            cout << "WARNING: QSFP" << module_id << " Channel " << j
                << " (Link " << setw(2) << 4*module_id+j
                << ") TX Bias out of bounds: "
                << txbias << " mA" << endl;
        } else if ( verbose )
        {
            cout << "INFO: QSFP" << module_id  << " Channel " << j
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
        cout << "INFO: FPGA Temp: " << fpgatemp << " degC" << endl;
    }

    /** check FPGA VCCINT*/
    float vccint = sm->VCCINT();
    if ( vccint < FPGA_VCCINT_MIN || vccint > FPGA_VCCINT_MAX )
    {
        cout << "ERROR: FPGA VCCINT out of bounds: "
            << vccint << endl;
    } else if ( verbose )
    {
        cout << "INFO: FPGA VCCINT: " << vccint << "V" << endl;
    }

    /** check FPGA VCCAUX*/
    float vccaux = sm->VCCAUX();
    if ( vccaux < FPGA_VCCAUX_MIN || vccaux > FPGA_VCCAUX_MAX )
    {
        cout << "ERROR: FPGA VCCINT out of bounds: "
            << vccaux << endl;
    } else if ( verbose )
    {
        cout << "INFO: FPGA VCCAUX: " << vccaux << "V" << endl;
    }
}


void
checkPcieState
(
    librorc::sysmon *sm,
    int verbose
)
{
    uint32_t pcie_gen = sm->pcieGeneration();
    if ( pcie_gen != 2 )
    {
        cout << "WARNING: Detected as PCIe Gen" << pcie_gen
             << ", expexted Gen2" << endl;
    } else if ( verbose )
    {
        cout << "INFO: Detected as PCIe Gen2" << endl;
    }

    uint32_t pcie_nlanes = sm->pcieNumberOfLanes();
    if ( pcie_nlanes != 8 )
    {
        cout << "WARNING: Expected 8 lanes PCIe, got "
             << pcie_nlanes << endl;
    } else if ( verbose )
    {
        cout << "INFO: 8 PCIe lanes detected " << endl;
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
            cout << "INFO: Fan: " << fanspeed << " RPM" << endl;
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
        uint32_t gtxctrl = link->GTX(RORC_REG_GTX_CTRL);
        if ( !(gtxctrl&1) )
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
checkDmaTestResults
(
    librorcChannelStatus *chstats,
    timeval               start_time,
    timeval               end_time,
    int                   verbose
)
{
    if ( chstats->bytes_received == 0 )
    {
        cout << "ERROR: DMA test failed, no data received!" << endl;
    } 
    else if  ( chstats->n_events == 0 )
    {
        cout << "ERROR: DMA test failed, no events received!" << endl;
    }
    else
    {
        float timediff = gettimeofdayDiff(start_time, end_time);
        float mbytes = (float)chstats->bytes_received / (float)(1<<20);
        float dma_rate = mbytes / timediff;
        if ( dma_rate < DMA_RATE_MIN || dma_rate > DMA_RATE_MAX )
        {
            cout << "WARNING: unexpected DMA rate: " << dma_rate
                 << " MB/s." << endl;
        }
        cout << "INFO: DMA Test Result: " << mbytes << " MBytes in "
             << timediff << " sec. -> " << dma_rate << " MB/s."
             << endl;
        if (verbose)
        {
            cout << "INFO: EPI - max: " << chstats->max_epi
                 << ", min: " << chstats->min_epi
                 << ", avg: " << chstats->n_events/chstats->set_offset_count
                 << ", setOffsetCount: " << chstats->set_offset_count
                 << endl;
        }
    }
}



void
testDmaChannel
(
    librorc::device *dev,
    librorc::bar *bar,
    uint32_t channel_id,
    int timeout,
    int verbose
)
{
    DMAOptions opts;
    opts.deviceId  = dev->getDeviceId();
    opts.channelId = channel_id;
    opts.eventSize = 0x1000;
    opts.useRefFile = false;
    opts.esType = LIBRORC_ES_IN_HWPG;

    char logdirectory[] = "/tmp";
    
    librorc_event_callback event_callback = eventCallBack;
    librorc::link *link = new librorc::link(bar, opts.channelId);

    librorc::event_stream *eventStream;
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
    timeval current_time = start_time;
    
    while( gettimeofdayDiff(start_time, current_time) < timeout )
    {
        eventStream->handleChannelData( (void*)&(checker) );
        eventStream->m_bar1->gettime(&current_time, 0);
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    checkDmaTestResults( eventStream->m_channel_status, start_time, end_time, verbose );

    /** Cleanup */
    delete eventStream;
}


struct pci_dev
*initLibPciDev
(
    librorc::device *dev
)
{
    struct pci_access *pacc = NULL;
    struct pci_dev *pdev = NULL;

    // get PCI device serial number from libpci
    pacc = pci_alloc();		/* Get the pci_access structure */
    pci_init(pacc);		/* Initialize the PCI library */
    pci_scan_bus(pacc);		/* We want to get the list of devices */
    pdev = pci_get_dev(pacc, 0, dev->getBus(), 
            dev->getSlot(), dev->getFunc());
    return pdev;
}





uint64_t
getPcieDSN
(
    struct pci_dev *pdev
)
{
    uint64_t device_serial = 0;
    uint8_t *devserptr = (uint8_t *)&device_serial;
    pci_read_block(pdev, 0x104, devserptr, 8);

    return device_serial;
}

bool
checkForValidBarConfig
(
    struct pci_dev *pdev
)
{
    uint32_t bar0 = 0;
    uint32_t bar1 = 0;
    uint8_t *ptr = (uint8_t *)&bar0;
    pci_read_block(pdev, 0x10, ptr, 4);
    ptr = (uint8_t *)&bar1;
    pci_read_block(pdev, 0x14, ptr, 4);

    return (bar0!=0 and bar1!=0);
}



void
printPcieInfos
(
    librorc::device *dev,
    librorc::sysmon *sm
)
{
    cout << "INFO: Device " << (unsigned int)dev->getDeviceId() << " at PCI ";
    cout << setfill('0')
        << hex << setw(4) << (unsigned int)dev->getDomain() << ":"
         << hex << setw(2) << (unsigned int)dev->getBus() << ":"
         << hex << setw(2) << (unsigned int)dev->getSlot() << "."
         << hex << setw(1) << (unsigned int)dev->getFunc();

    cout << " [FW date: " << hex << setw(8) << sm->FwBuildDate()
         << ", FW revision: "      << hex << setw(8) << sm->FwRevision()
         << "]" << endl;
}
