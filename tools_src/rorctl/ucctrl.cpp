/**
 * @file ucctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-26
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
        -d [0...255]    Target device \n\
        -r              Reset uC \n\
        -f [file]       File to be written into uC \n\
        -p              Program uC \n\
"

#define SPI_PRESCALER 500

#define SPI_PROG_ENABLE 0xac530000
#define SPI_READ_SIG    0x30000000

void
uc_configure_spi
(
    librorc::bar *bar
)
{
    /** set prescaler */
    bar->set(RORC_REG_UC_SPI_CTRL, SPI_PRESCALER);

    uint32_t ucctrl = bar->get(RORC_REG_UC_CTRL);
    /** drive MOSI low from GPIO */
    ucctrl &= ~(1<<5);
    /** drive SCK low from GPIO */
    ucctrl &= ~(1<<7);
    /** drive RESET_N high */
    ucctrl |= (1<<16);

    /** set SCK as output */
    ucctrl &= ~(1<<15);
    /** set MOSI as output */
    ucctrl &= ~(1<<13);
    /** set RESET_N as output */
    ucctrl &= ~(1<<20);

    bar->set(RORC_REG_UC_CTRL, ucctrl);
}

void
uc_unconfigure_spi
(
    librorc::bar *bar
)
{
    uint32_t ucctrl = bar->get(RORC_REG_UC_CTRL);
    /** set all CTRL lines to input */
    ucctrl |= (0x07<<20);
    /** set all DAT lines to input */
    ucctrl |= (0xff<<8);

    bar->set(RORC_REG_UC_CTRL, ucctrl);
}

void
uc_set_reset
(
    librorc::bar *bar,
    uint32_t rstval
    )
{
    uint32_t ucctrl = bar->get(RORC_REG_UC_CTRL);
    if ( rstval )
    {
        /** drive low, assert reset */
        ucctrl &= ~(1<<16);
    }
    else
    {
        /** drive high, deassert reset */
        ucctrl |= (1<<16);
    }

    bar->set(RORC_REG_UC_CTRL, ucctrl);
}


uint32_t
spi_send_command
(
    librorc::bar *bar,
    uint32_t cmd
    )
{
    /** send command */
    bar->set(RORC_REG_UC_SPI_DATA, cmd);

    /** wait for SPI_CE to deassert */
    while ( bar->get(RORC_REG_UC_SPI_CTRL) & (1<<31) )
    {
        usleep(100);
        // TODO: add timeout
    }

    return bar->get(RORC_REG_UC_SPI_DATA);
}

uint32_t
spi_read_device_signature
(
    librorc::bar *bar
)
{
    uint32_t sig = 0;
    for ( int i=0; i<4; i++ )
    {
        uint32_t result = spi_send_command(bar, SPI_READ_SIG | (i<<8));
        sig += ( (result & 0xff) << (8*i));
    }
    return sig;
}


int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = 0;
    int do_reset = 0;
    int do_program = 0;
    char *filename = NULL;
    int arg;
    while ( (arg = getopt(argc, argv, "hd:rf:p")) != -1 )
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

            case 'd':
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
        cout << "Invalid device selected: " << device_number << endl;
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
#ifdef SIM
    librorc::bar *bar = new librorc::sim_bar(dev, 1);
#else
    librorc::bar *bar = new librorc::rorc_bar(dev, 1);
#endif

    if ( bar->init() == -1 )
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    uc_configure_spi(bar);
    uc_set_reset(bar, 1);

    cout << "Device Signature: 0x" << setw(8) << setfill('0')
         << spi_read_device_signature(bar) << setfill(' ') << endl;

    uc_set_reset(bar, 0);
    uc_unconfigure_spi(bar);


    /**
     * TODO:
     * * reset uC
     * * read uC config status
     * * config uC
     * */

    delete bar;
    delete dev;

    if (filename)
    {
        delete filename;
    }

    return 0;
}
