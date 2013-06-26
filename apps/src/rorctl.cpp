/**
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2012-04-12
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

#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "librorc.h"

using namespace std;

#define HELP_TEXT "rorctl usage :                            \n\
rorctl [value parameter(s)] [instruction parameter]          \n\
Value parameters :                                           \n\
  -n [0...255]    Target device ID (get a list with -l).     \n\
  -f <filename>   Filename of the flash file.                \n\
  -v              Be verbose                                 \n\
  -c              Select Flash Chip [0 or 1]                 \n\
Instruction parameters :                                     \n\
  -l              List available devices.                    \n\
  -d              Dump device firmware to file.              \n\
                  Requires value parameters -n and f.        \n\
  -p              Program device flash.                      \n\
                  Requires value parameters -n and f.        \n\
  -e              Erase device flash (caution JTAG programmer\n\
                  needed afterwards)                         \n\
                  Requires value parameters -n               \n\
  -s              Show flash status                          \n\
  -r              Reset Flash                                \n\
Examples :                                                   \n\
Show status of Device 0 Flash 0:                             \n\
  rorctl -n 0 -c 0 -s                                        \n\
Dump firmware of device 0 to file firmware.bin :             \n\
  rorctl -n 0 -f firmware.bin -d                             \n\
Program firmware into device 0 from file firmware.bin :      \n\
  rorctl -n 0 -f firmware.bin -p                             \n\
Erase firmware from device 0:                                \n\
  rorctl -n 0 -e                                             \n\
List all available devices :                                 \n\
  rorctl -l                                                  \n\
"

#ifndef NOT_SET
    #define NOT_SET 0xFFFFFFFFFFFFFFFF
#endif

/** Parameter container */
typedef struct
{
    uint64_t                device_number;
    uint64_t                chip_select;
    char                   *filename;
    librorc_verbosity_enum  verbose;
    rorcfs_device          *dev;
}confopts;

/** Function signatures */
void print_devices();

inline
librorc_flash*
init_flash
(
    confopts options
);

int
show_device_monitor
(
    rorcfs_device *dev
);

void
flash_status
(
    librorc_flash *flash
);

void
dump_flash_status
(
    uint16_t       status,
    librorc_flash *flash
);



int main
(
    int argc,
    char* const *argv
)
{
    if(argc<2)
    {
        cout << HELP_TEXT;
        return -1;
    }

    confopts options =
    {
        NOT_SET,
        NOT_SET,
        NULL,
        LIBRORC_VERBOSE_OFF,
        NULL
    };

    librorc_flash *flash = NULL;
    {
        opterr = 0;
        int c;
        while((c = getopt(argc, argv, "hvlmdepn:f:c:sr")) != -1)
        {
            switch(c)
            {
                case 'h':
                {
                    cout << HELP_TEXT;
                    goto ret_main;
                }
                break;

                case 'v':
                {
                    options.verbose = LIBRORC_VERBOSE_ON;
                }
                break;

                case 'l':
                {
                    print_devices();
                    return(0);
                }
                break;

                case 'm':
                {
                    cout << "Monitor Device " << options.device_number
                         << endl << endl;
                    return( show_device_monitor(options.dev) );
                }
                break;

                case 'd':
                {
                    cout << "Dumping device to file!" << endl;
                    flash = init_flash(options);
                    return( flash->dump(options.filename, options.verbose) );
                }
                break;

                case 'r':
                {
                    cout << "Resetting Flash!" << endl;
                    flash = init_flash(options);
                    return(0);
                }
                break;

                case 'e':
                {
                    cout << "Erasing device!" << endl;
                    flash = init_flash(options);
                    return( flash->erase(16<<20, options.verbose) );
                }
                break;

                case 'p':
                {
                    cout << "Flashing device!" << endl;
                    flash = init_flash(options);
                    return( flash->flash(options.filename, options.verbose) );
                }
                break;

                case 'n':
                {
                    options.device_number = atoi(optarg);
                    try
                    { options.dev = new rorcfs_device(0); }
                    catch(...)
                    {
                        cout << "failed to initialize device 0 -"
                             << " is the board detected with lspci?" << endl;
                        abort();
                    }
                }
                break;

                case 'f':
                {
                    options.filename = (char *)malloc(strlen(optarg)+2);
                    sprintf(options.filename, "%s", optarg);
                }
                break;

                case 'c':
                {
                    options.chip_select = atoi(optarg);
                }
                break;

                case 's':
                {
                    flash = init_flash(options);
                    flash_status(flash);
                    return 0;
                }
                break;

                default:
                {
                    cout << "Unknown parameter (" << c << ")!" << endl;
                    cout << HELP_TEXT;
                    goto ret_main;
                }
                break;
            }
        }
    }

ret_main:

    if(options.filename != NULL)
    {
        free(options.filename);
    }

    if(options.dev != NULL)
    {
        delete options.dev;
    }

    return -1;
}



void
print_devices()
{
    cout << "Available CRORC devices:" << endl;

    rorcfs_device *dev = NULL;
    for(uint8_t i=0; i>(-1); i++)
    {
        try{ dev = new rorcfs_device(i);}
        catch(...){ break; }

        dev->printDeviceDescription();
        cout << endl;

        delete dev;
    }
}



inline
librorc_flash *
init_flash
(
    confopts options
)
{
    if(options.device_number == NOT_SET)
    {
        cout << "Device ID was not given!" << endl;
        return NULL;
    }

    if(options.chip_select == NOT_SET)
    {
        cout << "Flash Chip Select was not given, "
             << "using default Flash0" << endl;
        options.chip_select = 0;
    }

    librorc_bar *bar;
    #ifdef SIM
        bar = new sim_bar(options.dev, 0);
    #else
        bar = new rorc_bar(options.dev, 0);
    #endif

    if(bar->init() == -1)
    {
        cout << "BAR0 init failed" << endl;
        return(NULL);
    }

    /** get flash object */
    librorc_flash *flash = NULL;
    try
    {
        flash = new librorc_flash(bar, options.chip_select, options.verbose);
    }
    catch (int e)
    {
        switch (e)
        {
            case 1:
            {
                cout << "BAR is no CRORC flash.";
            }
            break;

            case 2:
            {
                cout << "Illegal flash chip select";
            }
            break;

            default:
            {
                cout << "Unknown Exceptoin Nr. " << e << endl;
            }
            break;
        }
        return(NULL);
    }

    /** set asynchronous read mode */
    flash->setConfigReg(0xbddf);

    uint16_t status = flash->resetChip();
    if ( status )
    {
        cout << "resetChip failed: " << hex << status << endl;
    }

    return(flash);
}



int
show_device_monitor
(
    rorcfs_device *dev
)
{
    if(dev == NULL)
    {
        cout << "Device ID was not given!" << endl;
        return -1;
    }

    /** bind to BAR1 */
    librorc_bar *bar1 = NULL;
    #ifdef SIM
        bar1 = new sim_bar(dev, 1);
    #else
        bar1 = new rorc_bar(dev, 1);
    #endif

    if ( bar1->init() == -1 )
    {
        return -1;
    }

    /** instantiate a new sysmon */
    rorcfs_sysmon *sm;
    try
    { sm = new rorcfs_sysmon(bar1); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar1;
        abort();
    }

    /** Printout revision and date */
    try
    {
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setprecision(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setprecision(8) << sm->FwBuildDate() << dec << endl;
    }
    catch(...)
    {
        cout << "Reading Date returned wrong. This is likely a "
             << "PCIe access problem. Check lspci if the device is up"
             << "and the BARs are enabled" << endl;
        delete sm;
        delete bar1;
        abort();
    }

    /** Print Voltages and Temperature */
    cout << "Temperature   : " << sm->FPGATemperature() << " °C" << endl
         << "FPGA VCCINT   : " << sm->VCCINT() << " V"  << endl
         << "FPGA VCCAUX   : " << sm->VCCAUX() << " V"  << endl;

    /** Print and check reported PCIe link width/speed */
    cout << "Detected as   : PCIe Gen" << sm->pcieGeneration()
         << " x" << sm->pcieNumberOfLanes() << endl;

    if( (sm->pcieGeneration()!=2) || (sm->pcieNumberOfLanes()!=8) )
    { cout << " WARNING: FPGA reports unexpexted PCIe link parameters!" << endl; }

    /** Check if system clock is running */
    cout << "SysClk locked : " << sm->systemClockIsRunning() << endl;

    /** Check if fan is running */
    cout << "Fan speed     : " << sm->systemFanSpeed() << " rpm" << endl;
    if( sm->systemFanIsEnabled() == false)
    {
        cout << "WARNING: fan seems to be disabled!" << endl;
    }

    if( sm->systemFanIsRunning() == false)
    {
        cout << "WARNING: fan seems to be stopped!" << endl;
    }

    /** Show QSFP status */
    cout << endl << "QSFPs" << endl;
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        cout << endl << "-------------------------------------" << endl << endl;
        try
        {
            cout << "QSFP " << i << " present: " << sm->qsfpIsPresent(i)  << endl;
            cout << "QSFP " << i << " LED0 : "   << sm->qsfpLEDIsOn(i, 0)
                                 << " LED1 : "   << sm->qsfpLEDIsOn(i, 1) << endl;

            if( sm->qsfpIsPresent(i) )
            {
                cout << "Checking QSFP"  << i << " i2c access:"    << endl;
                cout << "Vendor Name : " << sm->qsfpVendorName(i)  << endl;
                cout << "Part Number : " << sm->qsfpPartNumber(i)  << endl;
                cout << "Temperature : " << sm->qsfpTemperature(i) << "°C" << endl;
            }
        }
        catch(...)
        {
            cout << "QSFP readout failed!" << endl;
        }
    }

    cout << endl;
    cout << "-------------------------------------" << endl;

    delete sm;
    delete bar1;

    return EXIT_SUCCESS;
}



void
flash_status
(
    librorc_flash *flash
)
{
    flash->clearStatusRegister(0);
    uint16_t flashstatus = flash->getStatusRegister(0);

    cout << "Status               : " << hex
         << setw(4) << flashstatus << endl;
    dump_flash_status(flashstatus, flash);

    cout << "Manufacturer Code    : "    << hex << setw(4)
         << flash->getManufacturerCode() << endl;
    dump_flash_status(flashstatus, flash);

    cout << "Device ID            : " << hex << setw(4)
         << flash->getDeviceID() << endl;
    dump_flash_status(flashstatus, flash);

    cout << "Read Config Register : " << hex << setw(4)
         << flash->getReadConfigurationRegister() << endl;
    dump_flash_status(flashstatus, flash);

    cout << "Unique Device Number : " << hex
         << flash->getUniqueDeviceNumber() << endl;
    dump_flash_status(flashstatus, flash);
}



void
dump_flash_status
(
    uint16_t       status,
    librorc_flash *flash
)
{

    if( flash->getStatusRegister(0) != 0x0080 )
    {
        cout << "Status : " << hex << setw(4) << status << endl;

        if( status & (1<<7) )
        { cout << "\tReady" << endl; }
        else
        { cout << "\tBusy" << endl; }

        if( status & (1<<6) )
        { cout << "\tErase suspended" << endl; }
        else
        { cout << "\tErase in progress or completed" << endl; }

        if( status & (1<<5) )
        { cout << "\tErase/blank check error" << endl; }
        else
        { cout << "\tErase/blank check sucess" << endl; }

        if( status & (1<<4) )
        { cout << "\tProgram Error" << endl; }
        else
        { cout << "\tProgram sucess" << endl; }

        if( status & (1<<3) )
        { cout << "\tVpp invalid, abort" << endl; }
        else
        { cout << "\tVpp OK" << endl; }

        if( status & (1<<2) )
        { cout << "\tProgram Suspended" << endl; }
        else
        { cout << "\tProgram in progress or completed" << endl; }

        if( status & (1<<1) )
        { cout << "\tProgram/erase on protected block, abort" << endl; }
        else
        { cout << "\tNo operation to protected block" << endl; }

        if( status & 1 )
        {
            if( status & (1<<7) )
            { cout << "\tNot Allowed" << endl; }
            else
            { cout << "\tProgram or erase operation in a bank other than the addressed bank" << endl; }
        }
        else
        {
            if( status & (1<<7) )
            { cout << "\tNo program or erase operation in the device" << endl; }
            else
            { cout << "\tProgram or erase operation in addressed bank" << endl; }
        }

        flash->clearStatusRegister(0);
    }
}
