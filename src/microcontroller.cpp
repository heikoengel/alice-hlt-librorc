/**
 * @file microcontroller.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-27
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
#include <librorc/microcontroller.hh>
#include <librorc/bar.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME
{
    microcontroller::microcontroller
    (
        bar *parent_bar
    )
    {
        m_bar = NULL;
        if ( !parent_bar)
        {
            throw LIBRORC_UC_ERROR_CONSTRUCTOR_FAILED;
        }
        m_bar = parent_bar;
    }

    microcontroller::~microcontroller()
    {
        m_bar = NULL;
    }


    void
    microcontroller::configure_spi()
    {
        if ( m_bar->get32(RORC_REG_UC_SPI_CTRL)==0xa5a5a5a5 )
        {
            throw LIBRORC_UC_SPI_NOT_IMPLEMENTED;
        }

        // set prescaler 
        m_bar->set32(RORC_REG_UC_SPI_CTRL, UC_SPI_PRESCALER);

        uint32_t ucctrl = m_bar->get32(RORC_REG_UC_CTRL);
        // drive MOSI low from GPIO 
        ucctrl &= ~(1<<5);
        // drive SCK low from GPIO 
        ucctrl &= ~(1<<7);
        // drive RESET_N high 
        ucctrl |= (1<<16);

        // set SCK as output 
        ucctrl &= ~(1<<15);
        // set MOSI as output 
        ucctrl &= ~(1<<13);
        // set RESET_N as output 
        ucctrl &= ~(1<<20);

        m_bar->set32(RORC_REG_UC_CTRL, ucctrl);
    }


    void
    microcontroller::unconfigure_spi()
    {
        uint32_t ucctrl = m_bar->get32(RORC_REG_UC_CTRL);
        // set all CTRL lines to input 
        ucctrl |= (0x07<<20);
        // set all DAT lines to input 
        ucctrl |= (0xff<<8);

        m_bar->set32(RORC_REG_UC_CTRL, ucctrl);
    }


    void
    microcontroller::set_reset
    (
        uint32_t rstval
    )
    {
        uint32_t ucctrl = m_bar->get32(RORC_REG_UC_CTRL);
        if ( rstval )
        {
            // drive low, assert reset 
            ucctrl &= ~(1<<16);
        }
        else
        {
            // drive high, deassert reset 
            ucctrl |= (1<<16);
        }

        m_bar->set32(RORC_REG_UC_CTRL, ucctrl);

        // wait for at least 20 ms 
        usleep(20000);
    }


    uint32_t
    microcontroller::send_command
    (
        uint32_t cmd
    )
    {
        // set timeout to 1 sec. 
        uint32_t timeout = 10000;

        // send command 
        m_bar->set32(RORC_REG_UC_SPI_DATA, cmd);

        // wait for SPI_CE to deassert 
        while ( m_bar->get32(RORC_REG_UC_SPI_CTRL) & (1<<31) )
        {
            if ( timeout == 0 )
            {
                throw LIBRORC_UC_SEND_CMD_TIMEOUT;
            }

            usleep(100);
            timeout --;
        }

        return m_bar->get32(RORC_REG_UC_SPI_DATA);
    }


    void
    microcontroller::enter_prog_mode()
    {
        uint32_t result = send_command(UC_SPI_PROG_ENABLE);
        if ( (result & 0xff00) != 0x5300)
        {
            throw LIBRORC_UC_PROG_MODE_FAILED;
        }
    }


    uint32_t
    microcontroller::read_device_signature()
    {
        uint32_t sig = 0;
        for ( int i=0; i<3; i++ )
        {
            uint32_t result = send_command(UC_SPI_READ_SIG | (i<<8));
            sig += ( (result & 0xff) << (8*i));
        }
        return sig;
    }


    void
    microcontroller::earse_chip()
    {
        // set device in reset 
        set_reset(1);

        // enter programming mode 
        enter_prog_mode();

        // send chip ease command 
        send_command(UC_SPI_ERASE_CHIP);

        // wait at least 9 ms 
        usleep(9000);

        // release reset to end the erase 
        set_reset(0);
    }


    void
    microcontroller::load_extended_addr
    (
        uint32_t addr
    )
    {
        addr &= 0xff;
        send_command(UC_SPI_EXT_ADDR | (addr<<8) );
    }


    void
    microcontroller::load_mem_page
    (
        uint32_t addr,
        uint16_t data
    )
    {
        send_command(UC_SPI_PROG_LOW | (addr<<8) | (data & 0xff));
        send_command(UC_SPI_PROG_HIGH | (addr<<8) | ((data>>8) & 0xff));
    }


    uint16_t
    microcontroller::read_mem_page
    (
        uint32_t addr
    )
    {
        uint32_t low  = send_command(UC_SPI_READ_LOW | (addr<<8));
        uint32_t high = send_command(UC_SPI_READ_HIGH | (addr<<8));
        return (low & 0xff) | ((high&0xff)<<8);
    }


    void
    microcontroller::prog_mem_page
    (
        uint32_t addr
    )
    {
        // send write page command 
        send_command(UC_SPI_WRITE_PAGE | (addr<<8));

        // wait at least 4.5 ms 
        usleep(4500);
    }


    void
    microcontroller::programFromFile
    (
        char *filename
    )
    {
        // open file 
        int fd = open(filename, O_RDONLY);
        if ( fd < 0 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Failed to read file %s\n",
                    filename);
            throw LIBRORC_UC_FILE_ERROR;
        }

        struct stat stats;
        if ( fstat(fd, &stats) == -1 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Failed to stat file %s\n",
                    filename);
            throw LIBRORC_UC_FILE_ERROR;
        }

        if ( stats.st_size & 1 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Invalid file size: %d\n",
                    stats.st_size);
            throw LIBRORC_UC_FILE_ERROR;
        }

        // configure FPGA IOs for SPI 
        configure_spi();

        // erase flash 
        earse_chip();

        // enable reset 
        set_reset(1);

        // enter programming mode 
        enter_prog_mode();

        // set extended address for page 0 
        load_extended_addr(0);

        uint32_t addr = 0;
        uint32_t nbytes = 0;
        uint16_t page[UC_SPI_PAGESIZE];

        // read page from file 
        while ( (nbytes = read(fd, &page, UC_SPI_PAGESIZE<<1)) >0 )
        {
            for ( uint32_t i=0; i<(nbytes>>1); i++ )
            {
                load_mem_page(i, page[i]);
            }
            prog_mem_page(addr);
            addr += UC_SPI_PAGESIZE;
        }

        // release reset 
        set_reset(0);

        close(fd);

        unconfigure_spi();
    }

}
