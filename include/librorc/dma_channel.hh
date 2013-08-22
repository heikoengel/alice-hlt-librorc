/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2011-08-17
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

#ifndef LIBRORC_DMA_CHANNEL_H
#define LIBRORC_DMA_CHANNEL_H

#include "librorc/include_ext.hh"
#include "librorc/include_int.hh"

#define LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED              1
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_FAILED                   2
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_PATTERN_GENERATOR_FAILED 3
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_GTX_FAILED               4
#define LIBRORC_DMA_CHANNEL_ERROR_CLOSE_GTX_FAILED                5

/** default maximum payload size in bytes. Check the capabilities
 *  of the chipset and the FPGA PCIe core before modifying this value
 *  Common values are 128 or 256 bytes.*/

#define LIBRORC_MAX_DMA_CHANNELS  12

// TODO get this from PDA
#define MAX_PAYLOAD 256 //was 128 before, but it seemed wrong
#define MAX_READ_REQ 256

#define PAGE_MASK ~(sysconf(_SC_PAGESIZE) - 1)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

/**
 * @class dma_channel
 * @brief DMA channel management class
 *
 * Initialize DMA channel with init() before using any other member
 * function. Initialization sets the parent BAR and the channel offset
 * within the BAR (via dma_base). Use prepareEB() and prepareRB() to
 * fill the buffer descriptor memories, then configure Buffer-Enable
 * and -Continuous flags (setRBDMEnable(), setEBDMEnable(),
 * setRBDMContinuous(), setEBDMContinuous() ).
 * No DMA transfer will start unless setDMAEnable() has been called
 * with enable=1.
 **/
namespace librorc
{

    class dma_channel
    {
        public:
             dma_channel
             (
                uint32_t  channel_number,
                device   *dev,
                bar      *bar
             );

             dma_channel
             (
                uint32_t  channel_number,
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
                buffer   *eventBuffer,
                buffer   *reportBuffer
             );

            ~dma_channel();

            void enable();
            void waitForGTXDomain();
            void configurePatternGenerator(uint32_t eventSize);
            void configureGTX();
            void closeGTX();
            void waitForCommandTransmissionStatusWord();

            /**
             * set Enable Bit of EBDM
             * @param enable nonzero param will enable, zero will disable
             **/
            void setEnableEB(int32_t enable);

            /**
             * set Enable Bit of RBDM
             * @param enable nonzero param will enable, zero will disable
             **/
            void setEnableRB(int32_t enable);

            /**
             * get Enable Bit of EBDM
             * @return enable bit
             **/
            uint32_t getEnableEB();

            /**
             * get Enable Bit of RBDM
             * @return enable bit
             **/
            uint32_t getEnableRB();

            /**
             * setDMAConfig set the DMA Controller operation mode
             * @param config Bit mapping:
             * TODO
             **/
            void
            setDMAConfig(uint32_t config);

            /**
             * getDMAConfig
             * @return DMA Packetizer COnfiguration and Status
             **/
            uint32_t getDMAConfig();

            /**
             * set maximum PCIe packet size. This is MAX_PAYLOAD for
             * hlt_in and MAX_READ_REQ for hlt_out channels.
             * @param size maximum packet size in bytes
             **/
            void setPciePacketSize(uint32_t packet_size);

            /**
             * get last value set as PCIe packet size
             * @return maximum payload size in bytes
             **/
            uint32_t getPciePacketSize();

            /**
             * get number of Scatter Gather entries for the Event buffer
             * @return number of entries
             **/
            uint32_t getEBDMnSGEntries();

            /**
             * get number of Scatter Gather entries for the Report buffer
             * @return number of entries
             **/
            uint32_t getRBDMnSGEntries();

            /**
             * get DMA Packetizer 'Busy' flag
             * @return 1 if busy, 0 if idle
             **/
            uint32_t getDMABusy();

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
             * set Event Buffer File Offset
             * the DMA engine only writes to the Event buffer as long as
             * its internal offset is at least (MaxPayload)-bytes smaller
             * than the Event Buffer Offset. In order to start a transfer,
             * set EBOffset to (BufferSize-MaxPayload).
             * IMPORTANT: offset has always to be a multiple of MaxPayload!
             **/
            void setEBOffset(uint64_t offset);

            /**
             * get current Event Buffer File Offset
             * @return offset
             **/
            uint64_t getEBOffset();

            /**
             * set Report Buffer File Offset
             **/
            void setRBOffset(uint64_t offset);

            /**
             * get Report Buffer File Offset
             * @return offset
             **/
            uint64_t getRBOffset();

            /**
             * setOffsets
             * @param eboffset byte offset in event buffer
             * @param rboffset byte offset in report buffer
             **/
            void
            setOffsets
            (
                uint64_t eboffset,
                uint64_t rboffset
            );

            /**
             * get buffer offset that is currently used as
             * DMA destination
             * @return 64bit offset in report buffer file
             **/
            uint64_t getRBDMAOffset();

            /**
             * get buffer offset that is currently used as
             * DMA destination
             * @return 64bit offset in event buffer file
             **/
            uint64_t getEBDMAOffset();

            /**
             * get last event buffer read offset written to channel
             * @return 64bit offset in event buffer file
             **/
            uint64_t getLastEBOffset();


            /**
             * get last report buffer read offset written to channel
             * @return 64bit offset in report buffer file
             **/
            uint64_t getLastRBOffset();

            /**
             * set DW in EBDM
             * @param addr address in EBDM component
             * @param data data to be writtem
             **/
            void
            setEBDM
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from EBDM
             * @param addr address in EBDM component
             * @return data read from EBDM
             **/
            uint32_t getEBDM(uint32_t addr);

            /**
             * set DW in RBDM
             * @param addr address in RBDM component
             * @param data data to be writtem
             **/
            void
            setRBDM
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from RBDM
             * @param addr address in RBDM component
             * @return data read from RBDM
             **/
            uint32_t getRBDM(uint32_t addr);

            /**
             * set DW in EBDRAM
             * @param addr address in EBDRAM component
             * @param data data to be writtem
             **/
            void
            setEBDRAM
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * memcpy into EBDRAM
             * @param startaddr address in EBDRAM component
             * @param dma_desc pointer to dma descriptor
             * */
            void
            memcpyEBDRAMentry
            (
                uint32_t                startaddr,
                struct rorcfs_dma_desc *dma_desc
            );

            /**
             * get DW from EBDRAM
             * @param addr address in EBDRAM component
             * @return data read from EBDRAM
             **/
            uint32_t getEBDRAM(uint32_t addr);

            /**
             * set DW in RBDRAM
             * @param addr address in RBDRAM component
             * @param data data to be writtem
             **/
            void
            setRBDRAM
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * memcpy into RBDRAM
             * @param startaddr address in EBDRAM component
             * @param dma_desc pointer to dma descriptor
             * */
            void
            memcpyRBDRAMentry
            (
                uint32_t                startaddr,
                struct rorcfs_dma_desc *dma_desc
            );

            /**
             * get DW from RBDRAM
             * @param addr address in RBDRAM component
             * @return data read from RBDRAM
             **/
            uint32_t getRBDRAM(uint32_t addr);

            /**
             * set DW in Packtizer
             * @param addr address in PKT component
             * @param data data to be writtem
             **/
            void
            setPKT
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from  Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t getPKT(uint32_t addr);

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
            unsigned int getGTX(uint32_t addr);

        protected:

            uint32_t  m_base;
            uint32_t  m_channel;
            uint32_t  m_pcie_packet_size;
            uint64_t  m_last_ebdm_offset;
            uint64_t  m_last_rbdm_offset;

            bool      m_is_pattern_generator;
            bool      m_is_gtx;

            bar      *m_bar;
            device   *m_dev;
            buffer   *m_eventBuffer;
            buffer   *m_reportBuffer;

            /**
             * Prepare EventBuffer: copy scatterlist from
             * librorc::buffer into the EventBufferDescriptorManager
             * in the RORC
             * @param buf librorc::buffer instance to be used as event destination buffer
             * @return 0 on sucess, -1 on errors, -EFBIG if more than 2048 sg-entries
             **/
            int32_t prepareEB(buffer *buf);

            /**
             * prepare ReportBuffer: copy scatterlist from
             * librorc::buffer into the ReportBufferDescriptorManager
             * in the RORC
             * @param buf librorc::buffer instance to be used as report destination buffer
             * @return 0 on sucess, -1 on errors
             **/
            int32_t prepareRB(buffer *buf);

            /**
             * This method is the generic version of the
             * methods to program the sglists into the
             * CRORC bars.
             **/
            int32_t
            prepare
            (
                buffer   *buf,
                uint32_t  flag
            );

            /**
             * configure DMA engine for current set of buffers
             * @param pcie_packet_size packet size to be used for PCIe writes (hlt_in) or
             * PCIe read requests (hlt_out)
             * @return 0 on sucess, <0 on error
             **/
            int32_t configureChannel(uint32_t pcie_packet_size);
    };

}

#endif /** LIBRORC_DMA_CHANNEL_H */
