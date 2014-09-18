/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

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
        -r              Reset uC \n\
        -e              Erase Chip \n\
        -d              Dump to file \n\
        -f [file]       Filename \n\
        -p              Program uC \n\
"


/**
 * print signature and flash size
 * TODO: is there more usefull information available to print here?
 * */
void
spi_print_device_status
(
    librorc::microcontroller *uc
)
{
    /** set device in reset */
    uc->set_reset(1);

    /** enter programming mode */
    uc->enter_prog_mode();

    uint32_t sig = uc->read_device_signature();
    cout << "Device Signature: 0x" << hex << setw(6) << setfill('0')
        << sig << setfill(' ') << dec << endl;
    cout << "Flash size      : " << (1<< ((sig>>8)&0x0f)) << "kB" << endl;

    /** release reset */
    uc->set_reset(0);
}



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
    int do_reset = 0;
    int do_erase = 0;
    int do_status = 0;
    int do_program = 0;
    int do_dump = 0;
    char *filename = NULL;
    int arg;

    /** parse command line arguments */
    while ( (arg = getopt(argc, argv, "hn:rf:pesd")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                {
                    cout << HELP_TEXT;
                    return 0;
                }
                break;

            case 'r':
                {
                    do_reset = 1;
                }
                break;

            case 'e':
                {
                    do_erase = 1;
                }
                break;

            case 's':
                {
                    do_status = 1;
                }
                break;

            case 'd':
                {
                    do_dump = 1;
                }
                break;

            case 'n':
                {
                    device_number = atoi(optarg);
                }
                break;

            case 'f':
                {
                    filename = (char *)malloc(strlen(optarg)+2);
                    sprintf(filename, "%s", optarg);
                }
                break;

            case 'p':
                {
                    do_program = 1;
                    do_erase = 1;
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
    try
    {
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
    #ifdef MODELSIM
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

    librorc::microcontroller *uc;
    try
    {
        uc = new librorc::microcontroller(bar);
    }
    catch(...)
    {
        cout << "Failed to initialize microcontroller" << endl;
        delete bar;
        delete dev;
        abort();
    }

    try
    {
        /** configure FPGA IOs for SPI */
        uc->configure_spi();

        /** perform chip erase */
        if ( do_erase )
        {
            cout << "Erasing Chip...";
            uc->earse_chip();
            cout << " done." << endl;
        }

        /** toggle reset */
        if ( do_reset )
        {
            cout << "Resetting uC...";
            uc->set_reset(1);
            /** sleep additional 20 ms */
            usleep(20000);
            uc->set_reset(0);
            cout << " done." << endl;
        }

        /** print device status */
        if ( do_status )
        {
            spi_print_device_status(uc);
        }

        /** program from file */
        if ( do_program )
        {
            if (!filename)
            {
                cout << "No filename given!" << endl;
                uc->unconfigure_spi();
                abort();
            }

            /** open file */
            int fd = open(filename, O_RDONLY);
            if ( fd < 0 )
            {
                cout << "Failed to read file." << endl;
                uc->unconfigure_spi();
                abort();
            }
            struct stat stats;
            if ( fstat(fd, &stats) == -1 )
            {
                cout << "Failed to stat file." << endl;
                uc->unconfigure_spi();
                abort();
            }

            if ( stats.st_size & 1 )
            {
                cout << "WARNING: odd number of bytes - is this"
                    << " really a uC firmeware file?" << endl;
            }

            cout << "Filesize: " << stats.st_size << " bytes, "
                << (stats.st_size>>1) << " words" << endl;

            /** enable reset */
            uc->set_reset(1);

            /** enter programming mode */
            uc->enter_prog_mode();

            /** set extended address for page 0 */
            uc->load_extended_addr(0);

            uint32_t addr = 0;
            uint32_t nbytes = 0;
            uint16_t page[UC_SPI_PAGESIZE];

            /** read page from file */
            while ( (nbytes = read(fd, &page, UC_SPI_PAGESIZE<<1)) >0 )
            {
                for ( uint32_t i=0; i<(nbytes>>1); i++ )
                {
                    //cout << hex << i << " " << page[i] << endl;
                    uc->load_mem_page(i, page[i]);
                }
                uc->prog_mem_page(addr);
                //cout << "Wrote " << nbytes << " bytes to " << addr << endl;
                addr += UC_SPI_PAGESIZE;
            }

            /** release reset */
            uc->set_reset(0);

            close(fd);
        }


        /** dump configuration to file */
        if ( do_dump )
        {
            if (!filename)
            {
                cout << "No filename given!" << endl;
                uc->unconfigure_spi();
                abort();
            }

            /** open file */
            int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
            if ( fd < 0 )
            {
                cout << "Failed to open file." << endl;
                uc->unconfigure_spi();
                abort();
            }

            uint16_t page[UC_SPI_PAGESIZE];

            uint32_t addr = 0;
            uint32_t nbytes = 0;

            /** enable reset */
            uc->set_reset(1);

            /** enter programming mode */
            uc->enter_prog_mode();

            /** set extended address for page 0 */
            uc->load_extended_addr(0);

            /** read page from device */
            for (addr=0;addr<0x4000;addr+=UC_SPI_PAGESIZE)
            {
                for(int i=0; i<UC_SPI_PAGESIZE; i++)
                {
                    page[i] = uc->read_mem_page(i+addr);
                }
                nbytes = write(fd, &page, UC_SPI_PAGESIZE<<1);
                if ( nbytes != UC_SPI_PAGESIZE<<1 )
                {
                    perror("Failed to write to dump file");
                    break;
                }
            }

            /** release reset */
            uc->set_reset(0);

            close(fd);
        }

        uc->unconfigure_spi();
    }
    catch (...)
    {
        cout << "ucctrl failed!" << endl;
    }


    delete uc;
    delete bar;
    delete dev;

    if (filename)
    {
        delete filename;
    }

    return 0;
}
