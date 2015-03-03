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

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <librorc/microcontroller.hh>
#include <librorc/bar.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME
{
/**
 * prescaler defines SCK pulse width as number of FPGA clock cycles.
 * SCK pulse width has to be >2 uC CPU cycles. uC starts up with
 * 1 MHz CPU frequency -> min SCK width >2us.
 * Having a 250MHz FPGA clock this results in the following values:
 * | prescaler | SCK width | SPI bitrate |
 * |-----------|-----------|-------------|
 * |      1000 |      4 us |   ~125 kbps |
 * |       500 |      2 us |   ~250 kbps |
 * */
#define UC_SPI_PRESCALER 1000

#define UC_SPI_PROG_ENABLE 0xac530000
#define UC_SPI_READ_SIG    0x30000000
#define UC_SPI_ERASE_CHIP  0xac800000
#define UC_SPI_EXT_ADDR    0x4d000000
#define UC_SPI_PROG_HIGH   0x48000000
#define UC_SPI_PROG_LOW    0x40000000
#define UC_SPI_WRITE_PAGE  0x4c000000
#define UC_SPI_READ_LOW    0x20000000
#define UC_SPI_READ_HIGH   0x28000000

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

        /*uint32_t ucctrl = m_bar->get32(RORC_REG_UC_CTRL);
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

        m_bar->set32(RORC_REG_UC_CTRL, ucctrl);*/
    }


    void
    microcontroller::unconfigure_spi()
    {
        /*uint32_t ucctrl = m_bar->get32(RORC_REG_UC_CTRL);
        // set all CTRL lines to input 
        ucctrl |= (0x07<<20);
        // set all DAT lines to input 
        ucctrl |= (0xff<<8);

        m_bar->set32(RORC_REG_UC_CTRL, ucctrl);*/
    }


    void
    microcontroller::set_reset
    (
        uint32_t rstval
    )
    {
        uint32_t ucctrl = m_bar->get32(RORC_REG_UC_SPI_CTRL);
        if ( rstval )
        {
            // drive low, assert reset 
            ucctrl &= ~(1<<30);
        }
        else
        {
            // drive high, deassert reset 
            ucctrl |= (1<<30);
        }

        m_bar->set32(RORC_REG_UC_SPI_CTRL, ucctrl);

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
        { throw LIBRORC_UC_FILE_ERROR; }

        struct stat stats;
        if ( fstat(fd, &stats) == -1 )
        { throw LIBRORC_UC_FILE_ERROR; }

        if ( stats.st_size & 1 )
        { throw LIBRORC_UC_FILE_ERROR; }

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
