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

#ifndef LIBRORC_MICROCONTROLLER_H
#define LIBRORC_MICROCONTROLLER_H

#include "include_ext.hh"
#include "defines.hh"

#define LIBRORC_UC_ERROR_CONSTRUCTOR_FAILED 1
#define LIBRORC_UC_SPI_NOT_IMPLEMENTED 2
#define LIBRORC_UC_SEND_CMD_TIMEOUT 4
#define LIBRORC_UC_PROG_MODE_FAILED 8
#define LIBRORC_UC_FILE_ERROR 16


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


namespace LIBRARY_NAME
{
    class bar;

    /**
     * @brief Interface class for onboard microcontroller
     *
     * This class provides the possibility to update the
     * microcontroller firmware.
     **/
    class microcontroller
    {
        public:
            microcontroller(bar *parent_bar);
            ~microcontroller();


            /**
             * Configure FPGA<->uC interface for SPI:
             * set SCK and MOSI as output, MISO as input
             * */
            void
            configure_spi();


            /**
             * Set SCK and MOSI IO configuration back to inputs
             * */
            void
            unconfigure_spi();


            /**
             * set microcontroller reset
             * @param rstval reset value, can be 0 or 1
             **/
            void
            set_reset
            (
                uint32_t rstval
            );


            /**
             * send SPI command to microcontroller. A command consists
             * of 4 byte. See ATmega324A Userguide for details.
             * @param cmd command
             * @return data returned from MISO. If successful,
             * result[15:8] is the same as command[23:16].
             **/
            uint32_t
            send_command
            (
                uint32_t cmd
            );
            

            /**
             * Enter programming mode
             **/
            void
            enter_prog_mode();


            /**
             * Read device signature. Device has to be in programming
             * mode to allow signature readout.
             * There are three signature bytes, selected by address 
             * in command bits [9:8]. Values expected for ATmega324A:
             * addr 0: 0x1e
             * addr 1: 0x95
             * addr 2: 0x15
             * @return signature
             **/
            uint32_t
            read_device_signature();
            

            /**
             * Erase Chip. This erases uC flash and EEPROM
             * */
            void
            earse_chip();


            /**
             * load extended address. This has to be done only for the
             * first page or when crossing a 64 KWord boundary. 
             * The latter never happens for ATmega324A...
             * @param addr address
             **/
            void
            load_extended_addr
            (
                uint32_t addr
            );


            /**
             * load flash data into page buffer
             * @param addr page address
             * @param data data to be written
             * */
            void
            load_mem_page
            (
                uint32_t addr,
                uint16_t data
            );


            /**
             * read flash memory address
             * @param addr page address
             * @return page content
             * */
            uint16_t
            read_mem_page
            (
                uint32_t addr
            );
  
            
            /**
             * write page buffer into flash. This completes the 
             * programming of a page.
             * @param addr page address
             * */
            void
            prog_mem_page
            (
                uint32_t addr
            );


            /**
             * wrapper call to flash a file to the microcontroller.
             * @param filename char* filename to be flashed
             **/
            void programFromFile
            (
                char *filename
            );

        private:
            bar *m_bar;

    };
}

#endif /**LIBRORC_MICROCONTROLLER_H */
