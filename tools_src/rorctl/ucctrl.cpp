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
        -n [0...255]    Target device \n\
        -r              Reset uC \n\
        -e              Erase Chip \n\
        -d              Dump to file \n\
        -f [file]       Filename \n\
        -p              Program uC \n\
"

#define SPI_PRESCALER 1000
/** page size in words */
#define SPI_PAGESIZE 64

#define SPI_PROG_ENABLE 0xac530000
#define SPI_READ_SIG    0x30000000
#define SPI_ERASE_CHIP  0xac800000
#define SPI_EXT_ADDR    0x4d000000
#define SPI_PROG_HIGH   0x48000000
#define SPI_PROG_LOW    0x40000000
#define SPI_WRITE_PAGE  0x4c000000
#define SPI_READ_LOW    0x20000000
#define SPI_READ_HIGH   0x28000000

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

    /** wait for at least 20 ms */
    usleep(20000);
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
    /**
     * Device has to be in programming mode to allow signature readout.
     * There are three signature bytes, selected by address in command
     * bits [9:8]. Values expected for ATmega324A:
     * addr 0: 0x1e
     * addr 1: 0x94
     * addr 2: 0x15
     * */
    uint32_t sig = 0;
    for ( int i=0; i<3; i++ )
    {
        uint32_t result = spi_send_command(bar, SPI_READ_SIG | (i<<8));
        sig += ( (result & 0xff) << (8*i));
    }
    return sig;
}

void
spi_print_device_status
(
    librorc::bar *bar
)
{
    /** set device in reset */
    uc_set_reset(bar, 1);

    /** enter programming mode */
    spi_send_command(bar, SPI_PROG_ENABLE);

    uint32_t sig = spi_read_device_signature(bar);
    cout << "Device Signature: 0x" << hex << setw(6) << setfill('0')
        << sig << setfill(' ') << dec << endl;
    cout << "Flash size      : " << (1<< ((sig>>8)&0x0f)) << "kB" << endl;

    /** release reset */
    uc_set_reset(bar, 0);
}


void
spi_earse_chip
(
    librorc::bar *bar
)
{
    /** set device in reset */
    uc_set_reset(bar, 1);

    /** enter programming mode */
    spi_send_command(bar, SPI_PROG_ENABLE);
    // TODO: check return value

    /** send chip ease command */
    spi_send_command(bar, SPI_ERASE_CHIP);

    /** wait at least 9 ms */
    usleep(9000);

    /** release reset to end the erase */
    uc_set_reset(bar, 0);
}

void
spi_load_extended_addr
(
    librorc::bar *bar,
    uint32_t addr
)
{
    addr &= 0xff;
    spi_send_command(bar, SPI_EXT_ADDR | (addr<<8) );
}

void
spi_load_mem_page
(
    librorc::bar *bar,
    uint32_t addr,
    uint16_t data
)
{
    spi_send_command(bar, SPI_PROG_LOW | (addr<<8) | (data & 0xff));
    spi_send_command(bar, SPI_PROG_HIGH | (addr<<8) | ((data>>8) & 0xff));
}

uint16_t
spi_read_mem_page
(
    librorc::bar *bar,
    uint32_t addr
)
{
    uint32_t low  = spi_send_command(bar, SPI_READ_LOW | (addr<<8));
    uint32_t high = spi_send_command(bar, SPI_READ_HIGH | (addr<<8));
    return (low & 0xff) | ((high&0xff)<<8);
}

void
spi_prog_mem_page
(
    librorc::bar *bar,
    uint32_t addr
)
{
    /** send write page command */
    spi_send_command(bar, SPI_WRITE_PAGE | (addr<<8));

    /** wait at least 4.5 ms */
    usleep(4500);
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
    int do_erase = 0;
    int do_status = 0;
    int do_program = 0;
    int do_dump = 0;
    char *filename = NULL;
    int arg;
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

    /** perform chip erase */
    if ( do_erase )
    {
        cout << "Erasing Chip...";
        spi_earse_chip(bar);
        cout << " done." << endl;
    }

    /** toggle reset */
    if ( do_reset )
    {
        cout << "Resetting uC...";
        uc_set_reset(bar, 1);
        /** sleep additional 20 ms */
        usleep(20000);
        uc_set_reset(bar, 0);
        cout << " done." << endl;
    }
        
    /** print device status */
    if ( do_status )
    {
        spi_print_device_status(bar);
    }

    /** program from file */
    if ( do_program )
    {
        if (!filename)
        {
            cout << "No filename given!" << endl;
            uc_unconfigure_spi(bar);
            abort();
        }

        /** open file */
        int fd = open(filename, O_RDONLY);
        if ( fd < 0 )
        {
            cout << "Failed to read file." << endl;
            uc_unconfigure_spi(bar);
            abort();
        }
        struct stat stats;
        if ( fstat(fd, &stats) == -1 )
        {
            cout << "Failed to stat file." << endl;
            uc_unconfigure_spi(bar);
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
        uc_set_reset(bar, 1);

        /** enter programming mode */
        uint32_t status = spi_send_command(bar, SPI_PROG_ENABLE);
        if ( (status & 0xff00) != 0x5300)
        {
            cout << "Failed to enter programming mode: " << hex 
                 << status << endl;
            close(fd);
            uc_unconfigure_spi(bar);
            abort();
        }

        /** set extended address for page 0 */
        spi_load_extended_addr(bar, 0);

        uint32_t addr = 0;
        uint32_t nbytes = 0;
        uint16_t page[SPI_PAGESIZE];

        /** read page from file */
        while ( (nbytes = read(fd, &page, SPI_PAGESIZE<<1)) >0 )
        {
            for ( int i=0; i<(nbytes>>1); i++ )
            {
                //cout << hex << i << " " << page[i] << endl;
                spi_load_mem_page(bar, i, page[i]);
            }
            spi_prog_mem_page(bar, addr);
            //cout << "Wrote " << nbytes << " bytes to " << addr << endl;
            addr += SPI_PAGESIZE;
        }
        
        /** release reset */
        uc_set_reset(bar, 0);

        close(fd);
    }

    if ( do_dump )
    {        
        if (!filename)
        {
            cout << "No filename given!" << endl;
            uc_unconfigure_spi(bar);
            abort();
        }

        /** open file */
        int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
        if ( fd < 0 )
        {
            cout << "Failed to open file." << endl;
            uc_unconfigure_spi(bar);
            abort();
        }
        
        uint16_t page[256];

        uint32_t addr = 0;
        uint32_t nbytes = 0;
        
        /** enable reset */
        uc_set_reset(bar, 1);

        /** enter programming mode */
        uint32_t status = spi_send_command(bar, SPI_PROG_ENABLE);
        if ( (status & 0xff00) != 0x5300)
        {
            cout << "Failed to enter programming mode: " << hex 
                 << status << endl;
            close(fd);
            uc_unconfigure_spi(bar);
            abort();
        }

        /** set extended address for page 0 */
        spi_load_extended_addr(bar, 0);

        /** read page from device */
        for (addr=0;addr<0x4000;addr+=SPI_PAGESIZE)
        {
            for(int i=0; i<SPI_PAGESIZE; i++)
            {
                page[i] = spi_read_mem_page(bar, i+addr);
            }
            nbytes = write(fd, &page, SPI_PAGESIZE<<1);
        }
        
        /** release reset */
        uc_set_reset(bar, 0);

        close(fd);
    }

    uc_unconfigure_spi(bar);


    delete bar;
    delete dev;

    if (filename)
    {
        delete filename;
    }

    return 0;
}
