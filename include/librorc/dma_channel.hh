/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
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

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>
#include <librorc/link.hh>

#define LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED              1
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_FAILED                   2
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_PATTERN_GENERATOR_FAILED 3
#define LIBRORC_DMA_CHANNEL_ERROR_CLOSE_PATTERN_GENERATOR_FAILED  4
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_GTX_FAILED               5
#define LIBRORC_DMA_CHANNEL_ERROR_CLOSE_GTX_FAILED                6

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
namespace LIBRARY_NAME
{
class bar;
class buffer;
class device;
class link;
class dma_channel_configurator;

    class dma_channel
    {
        friend class dma_channel_configurator;

        public:
             dma_channel
             (
                uint32_t  channel_number,
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
                buffer   *eventBuffer,
                buffer   *reportBuffer
             );

            virtual ~dma_channel();

            void enable();
            void disable();


            void
            setBufferOffsetsOnDevice
            (
                uint64_t eboffset,
                uint64_t rboffset
            );

            /**
             * get DMA Packetizer 'Busy' flag
             * @return 1 if busy, 0 if idle
             **/
            uint32_t getDMABusy();

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

            //TODO: make this protected and mark using classes as friend
            link* getLink();


/** TODO: This is stuff which is slated to be protected soon, but is used by several apps */

            /**
             * Set maximum PCIe packet size. This is MAX_PAYLOAD for
             * hlt_in and MAX_READ_REQ for hlt_out channels.
             * @param packet_size maximum packet size in bytes
             **/
            void setPciePacketSize(uint32_t packet_size);

            /**
             * Get last value set as PCIe packet size
             * @return maximum payload size in bytes
             **/
            uint32_t pciePacketSize();

            /**
             * Set Enable Bit of EBDM
             **/
            void enableEventBuffer();
            /**
             * Unset Enable Bit of EBDM
             **/
            void disableEventBuffer();

            /**
             * Set Enable Bit of RBDM
             **/
            void enableReportBuffer();

            /**
             * Unset Enable Bit of RBDM
             **/
            void disableReportBuffer();

            /**
             * setDMAConfig set the DMA Controller operation mode
             * @param config Bit mapping:
             **/
            void setDMAConfig(uint32_t config);

            /**
             * Printout the state of the DMA engine to the console
             * */
            void printDMAState();

        protected:
            uint64_t  m_last_ebdm_offset;
            uint64_t  m_last_rbdm_offset;

            device   *m_dev;
            buffer   *m_eventBuffer;
            buffer   *m_reportBuffer;
            link     *m_link;
            bar      *m_bar;
            uint32_t  m_channel_number;

            dma_channel_configurator *m_channelConfigurator;

            void
            initMembers
            (
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
                uint32_t  channel_number,
                buffer   *eventBuffer,
                buffer   *reportBuffer
            );

            void prepareBuffers();

            /**
             * Copy scatterlist from librorc::buffer into the EventBufferDescriptorManager
             * of the CRORC
             * @param buf librorc::buffer instance to be used as event destination buffer
             * @return 0 on sucess, -1 on errors, -EFBIG if more than 2048 sg-entries
             **/
            int32_t programSglistForEventBuffer(buffer *buf);

            /**
             * Copy scatterlist from librorc::buffer into the ReportBufferDescriptorManager
             * of the CRORC
             * @param buf librorc::buffer instance to be used as report destination buffer
             * @return 0 on sucess, -1 on errors
             **/
            int32_t programSglistForReportBuffer(buffer *buf);

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

            /****** ATOMICS **************************************************************/

            /**
             * get Enable Bit of the Event Buffer Descriptor Manager
             * @return enable bit
             **/
            uint32_t isEventBufferEnabled();

            /**
             * get Enable Bit of Report Buffer Descriptor Manager
             * @return enable bit
             **/
            uint32_t isReportBufferEnabled();

            /**
             * Read out the current DMA configuration
             * @return DMA Packetizer Configuration and Status
             **/
            uint32_t DMAConfig();

    };

}

#endif /** LIBRORC_DMA_CHANNEL_H */
