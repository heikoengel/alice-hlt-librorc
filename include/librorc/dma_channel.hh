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

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>

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

//////////////////////////////////////
/** TODO : This might be obsolete */
/** Shared mem key offset **/
#define SHM_KEY_OFFSET 2048
/** Shared mem device offset **/
#define SHM_DEV_OFFSET 32

typedef struct
{
    uint64_t n_events;
    uint64_t bytes_received;
    uint64_t min_epi;
    uint64_t max_epi;
    uint64_t index;
    uint64_t set_offset_count;
    uint64_t error_count;
    int64_t  last_id;
    uint32_t channel;
}librorcChannelStatus;
//////////////////////////////////


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

class bar;
class buffer;
class device;
class dma_channel_configurator;

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
            void disable();
            void waitForGTXDomain();

            void
            setBufferOffsetsOnDevice
            (
                uint64_t eboffset,
                uint64_t rboffset
            );

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

            void
            setOffsets
            (
            	uint64_t eboffset,
            	uint64_t rboffset
            );

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
             * get DW from  Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t packetizer(uint32_t addr);


/** TODO: This is stuff which is slated to be protected soon, but is used by several apps */
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
            uint32_t getPciePacketSize();

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
             * TODO
             **/
            void setDMAConfig(uint32_t config);

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

            dma_channel_configurator *m_channelConfigurator;

            void
            initMembers
            (
                uint32_t  channel_number,
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
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

            /****** ATOMICS **************************************************************/

            void waitForCommandTransmissionStatusWord();

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
