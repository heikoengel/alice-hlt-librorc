/**
 * @file
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>, Heiko Engel <hengel@cern.ch>
 * @date 2011-09-18
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
 */

#ifndef LIBRORC_LINK_H
#define LIBRORC_LINK_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>


typedef struct
gtxpll_settings_struct
{
    uint8_t clk25_div;
    uint8_t n1;
    uint8_t n2;
    uint8_t d;
    uint8_t m;
    uint8_t tx_tdcc_cfg;
    float refclk;
}gtxpll_settings;



namespace librorc
{
class bar;
class device;

    class link
    {
        public:
            link
            (
                bar      *bar,
                uint32_t  link_number
            );

            virtual ~link(){};

            /**
             * get base
             * @return channel base address
             **/
            uint64_t getBase()
            {
                return m_base;
            }

            /**
             * get BAR
             * @return bound librorc::bar
             **/
            bar *getBar()
            {
                return m_bar;
            }

            /**
             * set DW in GTX Domain
             * @param addr address in GTX component
             * @param data data to be writtem
             **/
            void
            setGTX
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from GTX Domain
             * @param addr address in GTX component
             * @return data read from GTX
             **/
            uint32_t GTX(uint32_t addr);

            /**
             * set DW in Packtizer
             * @param addr address in PKT component
             * @param data data to be writtem
             **/
            void
            setPacketizer
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * Read from GTX DRP port
             * @param drp_addr DRP address to read from
             * @return DRP value
             * */
            uint16_t
            drpRead(uint8_t drp_addr);

            /**
             * Write to GTX DRP port
             * @param drp_addr DRP address to write to
             * @param drp_data data to be written
             * */
            void
            drpWrite
            (
                uint8_t  drp_addr,
                uint16_t drp_data
            );

            /**
             * get current PLL configuration
             * @param ch pointer to dma_channel instance
             * @return struct gtxpll_settings
             * */
            gtxpll_settings
            drp_get_pll_config();

            /**
             * set new PLL configuration
             * @param ch pointer to dma_channel instance
             * @param pll struct gtxpll_settings with new values
             * */
            void
            drp_set_pll_config(gtxpll_settings pll);

            /**
             * get DW from  Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t packetizer(uint32_t addr);

            /**
             * Printout the DIU-state to the console
             * */
            void printDiuState();

            /**
             * Printout the state of the DMA engine to the console
             * */
            void printDMAState();



        protected:
            bar      *m_bar;
            uint32_t  m_base;
            uint32_t  m_link_number;

            /**
             * read-modify-write:
             * replace rdval[bit+width-1:bit] with data[width-1:0]
             * */
            uint16_t
            rmw
            (
                uint16_t rdval,
                uint16_t data,
                uint8_t  bit,
                uint8_t  width
            );

    };

}

#endif /** LIBRORC_LINK_H */
