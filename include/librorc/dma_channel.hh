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

#ifndef LIBRORC_DMA_CHANNEL_H
#define LIBRORC_DMA_CHANNEL_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>
#include <librorc/link.hh>
#include <librorc/sysmon.hh>
#include <librorc/buffer.hh>

#define LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED              1
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_FAILED                   2
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_PATTERN_GENERATOR_FAILED 3
#define LIBRORC_DMA_CHANNEL_ERROR_CLOSE_PATTERN_GENERATOR_FAILED  4
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_GTX_FAILED               5
#define LIBRORC_DMA_CHANNEL_ERROR_CLOSE_GTX_FAILED                6

/** currently used by dma_monitor.cpp and hwpgdma_es_sweep.cpp */
#define LIBRORC_MAX_DMA_CHANNELS  12

#define PAGE_MASK ~(sysconf(_SC_PAGESIZE) - 1)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)


namespace LIBRARY_NAME
{
    class bar;
    class buffer;
    class device;
    class link;
    class sysmon;
    class dma_channel_configurator;


    /**
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
    class dma_channel
    {
        friend class dma_channel_configurator;

#define SGCTRL_WRITE_ENABLE (1<<31)
#define SGCTRL_TARGET_EBDMRAM (0<<30)
#define SGCTRL_TARGET_RBDMRAM (1<<30)
#define SGCTRL_EOE_FLAG (1<<0)

        public:
             dma_channel
             (
                uint32_t  channel_number,
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
                buffer   *eventBuffer,
                buffer   *reportBuffer,
                LibrorcEsType esType
             );

            typedef struct
            __attribute__((__packed__))
            {
                uint32_t sg_addr_low;  /** lower part of sg address **/
                uint32_t sg_addr_high; /** higher part of sg address **/
                uint32_t sg_len;       /** total length of sg entry in bytes **/
            } librorc_sg_entry_config;

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
             * Set maximum PCIe packet size. This is the maximum payload size for
             * hlt_in and maximum read request size for hlt_out channels.
             * @param packet_size maximum packet size in bytes
             **/
            void setPciePacketSize(uint32_t packet_size);

            /**
             * Get last value set as PCIe packet size
             * @return maximum payload size in bytes
             **/
            uint32_t pciePacketSize();

            /**
             * Set Enable Bit of the EventBufferDescriptorManager
             **/
            void setEbdmEnable( uint32_t enable );

            /**
             * Set Enable Bit of the ReportBufferDescriptorManager
             **/
            void setRbdmEnable( uint32_t enable );

            /**
             * setDMAConfig set the DMA Controller operation mode
             * @param config Bit mapping:
             **/
            void setDMAConfig(uint32_t config);

            /**
             * Printout the state of the DMA engine to the console
             * */
            void printDMAState();

            /**
             * Fill state of the HLT_OUT event descriptor FIFO
             * @return number of entries in FIFO
             **/
            uint32_t outFifoFillState();

            /**
             * Maximum number of outstanding HLT_OUT event descriptor
             * FIFO entries (= FIFO depth). Make sure outFifoFillState()
             * never exceeds outFifoDepth().
             * @return maximum number of FIFO entries.
             **/
            uint32_t outFifoDepth();

            void
            pushSglistEntryToRAM
            (
                uint64_t sg_addr,
                uint32_t sg_len,
                uint32_t ctrl
            );

            /**
             * announce an event to be read by the HLT-OUT firmware and
             * pushed out via SIU.
             * @param sglist vector of librorc_sg_entry containing the
             * pysical start addresses and lengths of the event blocks.
             * Use buffer::composeSglistFromBufferSegment to get from
             * buffer offset and length to this scatter-gather list.
             **/
            void
            announceEvent
            (
                 std::vector<librorc_sg_entry> sglist
            );

        protected:
            uint64_t  m_last_ebdm_offset;
            uint64_t  m_last_rbdm_offset;

            device   *m_dev;
            buffer   *m_eventBuffer;
            buffer   *m_reportBuffer;
            link     *m_link;
            sysmon   *m_sm;
            bar      *m_bar;
            uint32_t  m_channel_number;
            uint32_t  m_outFifoDepth;
            LibrorcEsType m_esType;

            dma_channel_configurator *m_channelConfigurator;

            void
            initMembers
            (
                uint32_t  pcie_packet_size,
                device   *dev,
                bar      *bar,
                uint32_t  channel_number,
                buffer   *eventBuffer,
                buffer   *reportBuffer,
                LibrorcEsType esType
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
            uint32_t isEBDMEnabled();

            /**
             * get Enable Bit of Report Buffer Descriptor Manager
             * @return enable bit
             **/
            uint32_t isRBDMEnabled();

            /**
             * Read out the current DMA configuration
             * @return DMA Packetizer Configuration and Status
             **/
            uint32_t DMAConfig();


            /**
             * set DMA engine suspend
             * @param value suspend value to be set. set to 1 before
             * disabling DMA engine, set to 0 before starting DMA engine.
             * getDMABusy() can only be non-zero is suspend is set to 1.
             **/
            void setSuspend
            (
                 uint32_t value
            );


            /**
             * get current suspend value
             * @return suspend bit [0/1]
             **/
            uint32_t getSuspend();

    };

}

#endif /** LIBRORC_DMA_CHANNEL_H */
