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

#define LIBRORC_LINK_DDL_TIMEOUT 10000

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



namespace LIBRARY_NAME
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

            /** clear DMA stall count */
            void clearDmaStallCount();

            /** clear Event Count */
            void clearEventCount();

            bool isGtxDomainReady();

            void clearGtxDisparityErrorCount();
            void clearGtxRxNotInTableCount();
            void clearGtxRxLossOfSignalCount();
            void clearGtxRxByteRealignCount();
            void clearAllGtxErrorCounter();

            uint32_t dmaStallCount();

            uint32_t dmaNumberOfEventsProcessed();

            //TODO Phase Locked Loop
            /**
             * Read from GTX DRP port
             * DRP = Dynamic Reconfiguratin Port (transciever)
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
            drpGetPllConfig();

            /**
             * set new PLL configuration
             * @param ch pointer to dma_channel instance
             * @param pll struct gtxpll_settings with new values
             * */
            void
            drpSetPllConfig(gtxpll_settings pll);

            /**
             * get DW from Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t packetizer(uint32_t addr);

            void waitForCommandTransmissionStatusWord();

            /**
             * wait for GTX domain to be ready read asynchronous GTX status
             * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
             * & !gtx_in_rst
            **/
            void waitForGTXDomain();

            /**
             * Printout the DIU-state to the console
             * */
            void printDiuState();



            /**
             * get buffer size set in EBDM. This returns the size of the
             * DMA buffer set in the DMA enginge and has to be the physical
             * size of the associated DMA buffer.
             * @return buffer size in bytes
             **/
            uint64_t getEBSize();

            /**
             * get buffer size set in RBDM. As the RB is not overmapped this size
             * should be equal to the sysfs file size and buf->getRBSize()
             * @return buffer size in bytes
             **/
            uint64_t getRBSize();

        protected:
            bar      *m_bar;
            uint32_t  m_base;
            uint32_t  m_link_number;

            /**
             * get base
             * @return channel base address
             **/
            uint64_t getBase()
            {
                return m_base;
            }

            uint32_t waitForDrpDenToDeassert();

    };

}

#endif /** LIBRORC_LINK_H */
