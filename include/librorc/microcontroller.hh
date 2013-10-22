/**
 * @file microcontroller.hh
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

#include "include_ext.hh"
#include "defines.hh"

#ifndef LIBRORC_MICROCONTROLLER_H
#define LIBRORC_MICROCONTROLLER_H

#define LIBRORC_UC_ERROR_CONSTRUCTOR_FAILED 1
#define LIBRORC_UC_SPI_NOT_IMPLEMENTED 2
#define LIBRORC_UC_SEND_CMD_TIMEOUT 4
#define LIBRORC_UC_PROG_MODE_FAILED 8


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
/** page size in words */
#define UC_SPI_PAGESIZE 64

#define UC_SPI_PROG_ENABLE 0xac530000
#define UC_SPI_READ_SIG    0x30000000
#define UC_SPI_ERASE_CHIP  0xac800000
#define UC_SPI_EXT_ADDR    0x4d000000
#define UC_SPI_PROG_HIGH   0x48000000
#define UC_SPI_PROG_LOW    0x40000000
#define UC_SPI_WRITE_PAGE  0x4c000000
#define UC_SPI_READ_LOW    0x20000000
#define UC_SPI_READ_HIGH   0x28000000

#define UCCTRL_PROG_MODE_FAILED 0
#define UCCTRL_UC_SPI_NOT_IMPLEMENTED 1
#define UCCTRL_SEND_CMD_TIMEOUT 2


namespace LIBRARY_NAME
{
class bar;

    class microcontroller
    {
        public:
            microcontroller(bar *parent_bar);
            ~microcontroller();

            void
            configure_spi();

            void
            unconfigure_spi();

            void
            set_reset
            (
                uint32_t rstval
            );

            uint32_t
            send_command
            (
                uint32_t cmd
            );
            
            void
            enter_prog_mode();

            uint32_t
            read_device_signature();
            
            void
            earse_chip();

            void
            load_extended_addr
            (
                uint32_t addr
            );

            void
            load_mem_page
            (
                uint32_t addr,
                uint16_t data
            );

            uint16_t
            read_mem_page
            (
                uint32_t addr
            );

            void
            prog_mem_page
            (
                uint32_t addr
            );

        private:
            bar *m_bar;

    };
}

#endif /**LIBRORC_MICROCONTROLLER_H */
