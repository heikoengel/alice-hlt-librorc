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
}confopts;

/** Function signatures */
void print_devices();

inline
librorc_flash *
init_flash
(
    confopts options
);

void dump_flash_status ( uint16_t status )
{
    if ( status & (1<<7) )
    {
        cout << "\tReady" << endl;
    } else
    {
        cout << "\tBusy" << endl;
    }

    if ( status & (1<<6) ) cout << "\tErase suspended" << endl;
    else cout << "\tErase in progress or completed" << endl;

    if ( status & (1<<5) ) cout << "\tErase/blank check error" << endl;
    else cout << "\tErase/blank check sucess" << endl;

    if ( status & (1<<4) ) cout << "\tProgram Error" << endl;
    else cout << "\tProgram sucess" << endl;

    if ( status & (1<<3) ) cout << "\tVpp invalid, abort" << endl;
    else cout << "\tVpp OK" << endl;

    if ( status & (1<<2) ) cout << "\tProgram Suspended" << endl;
    else cout << "\tProgram in progress or completed" << endl;

    if ( status & (1<<1) ) cout << "\tProgram/erase on protected block, abort" << endl;
    else cout << "\tNo operation to protected block" << endl;

    if ( status & 1 )
    {
        if (status & (1<<7)) cout << "\tNot Allowed" << endl;
        else cout << "\tProgram or erase operation in a bank other than the addressed bank" << endl;
    }
    else
    {
        if (status & (1<<7)) cout << "\tNo program or erase operation in the device" << endl;
        else cout << "\tProgram or erase operation in addressed bank" << endl;
    }
}



/*----------------------------------------------------------*/



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
        LIBRORC_VERBOSE_OFF
    };

    librorc_flash *flash = NULL;
    {
        opterr = 0;
        int c;
        while((c = getopt(argc, argv, "hvldepn:f:c:s")) != -1)
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

                case 'd':
                {
                    cout << "Dumping device to file!" << endl;
                    flash = init_flash(options);
                    return( flash->dump(options.filename, options.verbose) );
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
                    flash->clearStatusRegister(0);

                    // set asynchronous read mode
                    flash->setConfigReg(0xbddf);

                    uint16_t flashstatus = flash->getStatusRegister(0);
                    cout << "Status               : " << hex
                        << setw(4) << flashstatus << endl;
                    if (flashstatus!=0x0080)
                    {
                        dump_flash_status(flashstatus);
                        flash->clearStatusRegister(0);
                    }

                    cout << "Manufacturer Code    : " << hex << setw(4)
                        << flash->getManufacturerCode() << endl;
                    flashstatus = flash->getStatusRegister(0);
                    if (flashstatus!=0x0080)
                    {
                        cout << "Status : " << hex << setw(4) << flashstatus << endl;
                        dump_flash_status(flashstatus);
                        flash->clearStatusRegister(0);
                    }

                    cout << "Device ID            : " << hex << setw(4)
                        << flash->getDeviceID() << endl;
                    flashstatus = flash->getStatusRegister(0);
                    if (flashstatus!=0x0080)
                    {
                        cout << "Status : " << hex << setw(4) << flashstatus << endl;
                        dump_flash_status(flashstatus);
                        flash->clearStatusRegister(0);
                    }

                    cout << "Read Config Register : " << hex << setw(4)
                        << flash->getReadConfigurationRegister() << endl;
                    flashstatus = flash->getStatusRegister(0);
                    if (flashstatus!=0x0080)
                    {
                        cout << "Status : " << hex << setw(4) << flashstatus << endl;
                        dump_flash_status(flashstatus);
                        flash->clearStatusRegister(0);
                    }

                    cout << "Unique Device Number : " << hex
                        << flash->getUniqueDeviceNumber() << endl;
                    flashstatus = flash->getStatusRegister(0);
                    if (flashstatus!=0x0080)
                    {
                        cout << "Status : " << hex << setw(4) << flashstatus << endl;
                        dump_flash_status(flashstatus);
                        flash->clearStatusRegister(0);
                    }

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

    return -1;
}



void
print_devices()
{
    cout << "Available CRORC devices:" << endl;

    rorcfs_device *dev = NULL;
    for(uint8_t i=0; i>(-1); i++)
    {
        dev = new rorcfs_device();
        if(dev->init(i) == -1)
        {
            break;
        }

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

    rorcfs_device *dev
        = new rorcfs_device();
    if(dev->init(options.device_number) == -1)
    {
        cout << "Failed to initialize device "
             << options.device_number << endl;
        return(NULL);
    }

    librorc_bar *bar;
    #ifdef SIM
        bar = new sim_bar(dev, 0);
    #else
        bar = new rorc_bar(dev, 0);
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
            cout << "BAR is no CRORC flash.";
            break;

        case 2:
            cout << "Illegal flash chip select";
            break;

        default:
            cout << "Unknown Exceptoin Nr. " << e << endl;
            break;
        }
        return(NULL);
    }

    return(flash);
}
