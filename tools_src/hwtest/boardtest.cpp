/**
 * @file boardtest.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-26
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
#include <iostream>
#include <iomanip>

#include "librorc.h"

using namespace std;

#define HELP_TEXT "ucctrl usage: \n\
    ucctrl [parameters] \n\
Parameters: \n\
-h              Print this help \n\
-n [0...255]    Target device \n\
-v              Verbose mode \n\
"

/** define value ranges */
#define FPGA_TEMP_MIN 35.0
#define FPGA_TEMP_MAX 65.0
#define FPGA_VCCINT_MIN 0.9
#define FPGA_VCCINT_MAX 1.1
#define FPGA_VCCAUX_MIN 2.4
#define FPGA_VCCAUX_MAX 2.6
#define FPGA_FANSPEED_MIN 8000.0
#define FPGA_FANSPEED_MAX 10000.0

#define QSFP_TEMP_MIN 20.0
#define QSFP_TEMP_MAX 50.0
#define QSFP_VCC_MIN 3.2
#define QSFP_VCC_MAX 3.4
#define QSFP_RXPOWER_MIN 0.1
#define QSFP_RXPOWER_MAX 1.0
#define QSFP_TXBIAS_MIN 1.0
#define QSFP_TXBIAS_MAX 10.0

#define REFCLK_FREQ_MIN 212.4
#define REFCLK_FREQ_MAX 212.6

#define UC_DEVICE_SIGNATURE 0x15951e


/**
 * MAIN
 * */
int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int sysclk_avail = 1;
    int uc_avail = 1;
    int verbose = 0;

    /** parse command line arguments */
    int arg;
    while ( (arg = getopt(argc, argv, "hn:v")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                {
                    cout << HELP_TEXT;
                    return 0;
                }
                break;

            case 'n':
                {
                    device_number = strtol(optarg, NULL, 0);
                }
                break;

            case 'v':
                {
                    verbose = 1;
                }
                break;

            default:
                {
                    cout << "Unknown parameter (" << arg << ")!" << endl;
                    cout << HELP_TEXT;
                    return -1;
                }
                break;
        } //switch
    } //while

    if ( device_number < 0 || device_number > 255 )
    {
        cout << "No or invalid device selected: " << device_number << endl;
        cout << HELP_TEXT;
        abort();
    }

    /** Instantiate device **/
    librorc::device *dev = NULL;
    try{
        dev = new librorc::device(device_number);
    }
    catch(...)
    {
        cout << "Failed to intialize device " << device_number
            << endl;
        return -1;
    }

    /** Instantiate a new bar */
    librorc::bar *bar = NULL;
    try
    {
#ifdef SIM
        bar = new librorc::sim_bar(dev, 1);
#else
        bar = new librorc::rorc_bar(dev, 1);
#endif
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    /** print FW rev and date */
    librorc::sysmon *sm;
    try
    { sm = new librorc::sysmon(bar); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar;
        delete dev;
        abort();
    }

    cout << "Device [" <<  device_number << "] ";

    cout << setfill('0');
    cout << hex << setw(2) << (unsigned int)dev->getBus() << ":"
         << hex << setw(2) << (unsigned int)dev->getSlot()   << "."
         << hex << setw(1) << (unsigned int)dev->getFunc() ;

    cout << " - Firmware date: " << hex << setw(8) << sm->FwBuildDate()
         << ", Revision: "      << hex << setw(8) << sm->FwRevision()
         << dec << endl;

    /** check FW type */
    uint32_t fwtype = bar->get32(RORC_REG_TYPE_CHANNELS);
    if ( (fwtype>>16) != RORC_CFG_PROJECT_hwtest )
    {
        cout << "ERROR: No suitable FW detected - aborting." << endl;
        delete bar;
        delete dev;
        abort();
    }


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

    /** check sysclk is running */
    if ( !sm->systemClockIsRunning() )
    {
        cout << "ERROR: 200 MHz system clock not detected " 
             << "- have to skipp some test now..." << endl;
        sysclk_avail = 0;
    }

    /** check fan is running */
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

    if ( sysclk_avail )
    {
        /** check FPGA Temp */
        float fpgatemp = sm->FPGATemperature();
        if ( fpgatemp < FPGA_TEMP_MIN || fpgatemp > FPGA_TEMP_MAX )
        {
            cout << "ERROR: FPGA Temperature out of bounds: "
                << fpgatemp << endl;
        } else if ( verbose )
        {
            cout << "FPGA Temp: " << fpgatemp << " °C" << endl;
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

    /** check QSFP present and i2c status */ 
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        if ( !sm->qsfpIsPresent(i) )
        {
            cout << "WARNING: No QSFP detected in slot "
                 << i << " - skipping some tests..." << endl;
        }
        else if ( sm->qsfpGetReset(i) )
        {
            cout << "WARNING: QSFP " << i 
                 << " seems to be in RESET state - " 
                 << " skipping some tests" << endl;
        }
        else
        {
            /** QSFP temperature */
            float qsfptemp = sm->qsfpTemperature(i);
            if ( qsfptemp < QSFP_TEMP_MIN || qsfptemp > QSFP_TEMP_MAX )
            {
                cout << "ERROR: QSFP " << i 
                     << " Temperature out of bounds: "
                     << qsfptemp << endl;
            } else if ( verbose )
            {
                cout << "QSFP " << i 
                     << " Temperature: " << qsfptemp << " °C" << endl;
            }

            /** QSFP supply voltage */
            float qsfpvcc = sm->qsfpVoltage(i);
            if ( qsfpvcc < QSFP_VCC_MIN || qsfpvcc > QSFP_VCC_MAX )
            {
                cout << "ERROR: QSFP " << i
                     << " VCC out of bounds: " << qsfpvcc << endl;
            } else if ( verbose )
            {
                cout << "QSFP " << i 
                     << " VCC: " << qsfpvcc << " V" << endl;
            }

            /** QSFP RX Power */
            for (int j=0; j<4; j++)
            {
                float rxpower = sm->qsfpRxPower(i, j);
                if ( rxpower < QSFP_RXPOWER_MIN || rxpower > QSFP_RXPOWER_MAX )
                {
                    cout << "WARNING: QSFP " << i << " Channel " << j
                        << " (Link " << setw(2) << 4*i+j 
                        << ") RX Power out of bounds: "
                        << rxpower << " mW" << endl;
                } else if ( verbose )
                {
                    cout << "QSFP " << i  << " Channel " << j
                        << " (Link " << setw(2) << 4*i+j << ") RX Power: " 
                        << rxpower << " mW" << endl;
                }
            }

            /** QSFP TX Bias */
            for (int j=0; j<4; j++)
            {
                float txbias = sm->qsfpTxBias(i, j);
                if ( txbias < QSFP_TXBIAS_MIN || txbias > QSFP_TXBIAS_MAX )
                {
                    cout << "WARNING: QSFP " << i << " Channel " << j
                        << " (Link " << setw(2) << 4*i+j 
                        << ") TX Bias out of bounds: "
                        << txbias << " mA" << endl;
                } else if ( verbose )
                {
                    cout << "QSFP " << i  << " Channel " << j
                        << " (Link " << setw(2) << 4*i+j << ") TX bias: " 
                        << txbias << " mA" << endl;
                }
            }
        }
    }

    
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
        delete bar;
        delete dev;
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


    //TODO
    /** check GTX clk is running */



    /** check DDR3 module types in C0/C1 */
    for ( uint32_t i=0; i<2; i++)
    {
        try
        {
            uint8_t module_type = sm->i2c_read_mem(3, 0x50+i, 0x03);
            if ( module_type != 0x03 )
            {
                cout << "ERROR: Unexpected DDR3 module type in socket"
                     << i << ": 0x" << hex << (int)module_type << dec << endl;
            } else if ( verbose )
            {
                cout << "DDR3 module type in socket"
                     << i << ": 0x" << hex << (int)module_type << dec << endl;
            }
        }
        catch(...)
        {
            cout << "ERROR: Failed to read from DDR3 Module " << i << " SPD"
                 << " - Is DDR3 module present?" << endl;
        }
    }



    /** read uc signature */
    librorc::microcontroller *uc;
    try
    {
        uc = new librorc::microcontroller(bar);
    }
    catch(...)
    {
        cout << "failed to init microcontroller" << endl;
        uc_avail = 0;
    }

    if ( uc_avail )
    {
        try
        {
            /** configure FPGA IOs for SPI */
            uc->configure_spi();

            /** set device in reset */
            uc->set_reset(1);

            /** enter programming mode */
            uc->enter_prog_mode();

            uint32_t sig = uc->read_device_signature();
            if ( sig != UC_DEVICE_SIGNATURE )
            {
                cout << "ERROR: unexpected microcontroller signature 0x"
                     << hex << setw(6) << setfill('0')
                     << sig << " - expected 0x" << UC_DEVICE_SIGNATURE
                     << dec << setfill(' ') << endl;
            } else if ( verbose )
            {
                cout << "Microcontroller signature: 0x"
                     << hex << setw(6) << setfill('0')
                     << sig << dec << setfill(' ') << endl;
            }

            /** release reset */
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


    /* check LVDS tester status */
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


    /** free BAR1 */
    delete bar;


    /** check flashes */

    /** initialize BAR0 */
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
        delete dev;
        abort();
    }

    /** iterate over flash chips */
    for ( uint32_t i=0; i<2; i++ )
    {
        librorc::flash *flash = NULL;

        try
        {
            flash = new librorc::flash(bar, i, LIBRORC_VERBOSE_OFF);
        }
        catch(...)
        {
            cout << "Flash " << i << " init failed" << endl;
        }

        /** set asynchronous read mode */
        flash->setConfigReg(0xbddf);

        uint16_t status = flash->resetChip();
        if ( status )
        {
            cout << "resetChip failed: " << hex << status << endl;
            //optional: decode status value to specific error
        }

        uint16_t mcode = flash->getManufacturerCode();
        if ( mcode != 0x0049 )
        {
            cout << "ERROR: invalid Manufacturer Code 0x" << hex
                 << mcode << dec << " for flash " << i << endl;
        } else if ( verbose )
        {
            cout << "Flash " << i << " Status: " << hex
                 << mcode << dec << endl;
        }

        delete flash;
    }

    delete dev;

    return 0;
}
