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
Instruction parameters :                                     \n\
  -l              List available devices.                    \n\
  -d              Dump device firmware to file.              \n\
                  Requires value parameters -n and f.        \n\
  -p              Program device flash.                      \n\
                  Requires value parameters -n and f.        \n\
  -e              Erase device flash (caution JTAG programmer\n\
                  needed afterwards)                         \n\
                  Requires value parameters -n               \n\
Examples :                                                   \n\
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

/* Parameter container */
typedef struct
{
    uint64_t  device_number;
    char     *filename;
    uint8_t   verbose;
}confopts;

void print_devices();

inline
rorcfs_flash_htg *
init_flash
(
    confopts options
);

int64_t
dump_device
(
    confopts          options,
    rorcfs_flash_htg *flash
);

int64_t
erase_device
(
    confopts          options,
    rorcfs_flash_htg *flash
);

int64_t
flash_device
(
    confopts          options,
    rorcfs_flash_htg *flash
);



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
        NULL,
        0
    };

    rorcfs_flash_htg *flash = NULL;

    {
        opterr = 0;
        int c;
        while((c = getopt(argc, argv, "hvldepn:f:")) != -1)
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
                    options.verbose = 1;
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
                    flash = init_flash(options);
                    return( flash->dump(options.filename, options.verbose) );
                }
                break;

                case 'e':
                {
                    flash = init_flash(options);
                    return( flash->erase() );
                }
                break;

                case 'p':
                {
                    flash = init_flash(options);
                    return( flash->flash(options.filename) );
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
rorcfs_flash_htg *
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

    rorcfs_device *dev
        = new rorcfs_device();
    if(dev->init(options.device_number) == -1)
    {
        cout << "Failed to initialize device "
             << options.device_number << endl;
        return(NULL);
    }

    rorcfs_bar *bar
        = new rorcfs_bar(dev, 0);
    if(bar->init() == -1)
    {
        cout << "BAR0 init failed" << endl;
        return(NULL);
    }

    rorcfs_flash_htg *flash
        = new rorcfs_flash_htg(bar);

    return(flash);
}
