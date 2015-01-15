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

#include <librorc/defines.hh>
#include <librorc/buffer.hh>

#define LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED              1
#define LIBRORC_DMA_CHANNEL_ERROR_ENABLE_FAILED                   2


namespace LIBRARY_NAME
{
    class link;

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
        public:
             dma_channel( link *link );
            ~dma_channel();

            /**
             * enable DMA engine
             **/
            void enable();

            /**
             * disable DMA engine
             **/
            int disable();

            /**
             * get enable state of the DMA engine
             **/
            uint32_t getEnable();

            /**
             * configure DMA channel registers
             * This loads the scatter gather lists for ReportBuffer and
             * EventBuffer into the device and sets all configuration
             * registers required for DMA operation.
             * @param pointer to buffer instance to be used as EventBuffer
             * @param pointer to buffer instance to be used as ReportBuffer
             * @param esDir event stream data direction flag
             * @param pcie_packet_size maximum PCIe packet size to be used
             * @return 0 on success, -1 on error
             **/
            int
            configure
            (
                buffer *eventBuffer,
                buffer *reportBuffer,
                EventStreamDirection esDir,
                uint32_t pcie_packet_size
            );

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
             * get current number of Scatter Gather entries for the Event buffer
             * @return current number of entries
             **/
            uint32_t getEBDMNumberOfSgEntries();

            /**
             * get maximum number of Scatter Gather entries supported for the
             * Event buffer
             * @return maximum number of entries
             **/
            uint32_t getEBDMMaxSgEntries();

            /**
             * get current number of Scatter Gather entries for the Report buffer
             * @return current number of entries
             **/
            uint32_t getRBDMNumberOfSgEntries();

            /**
             * get maximum number of Scatter Gather entries supported for the
             * Report buffer
             * @return maximum number of entries
             **/
            uint32_t getRBDMMaxSgEntries();

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

            /**
             * read and clear DMA pointer stall flags. These flags get set if
             * a device-to-DMA-buffer write is delayed because the software read
             * pointer points to the next write location. This happens when the
             * software cannot handle the incoming data rate.
             * @return 2 bit stall flags: if bit 0 is set the EventBuffer pointer
             * stalled, if bit 1 is set the ReportBuffer pointer stalled.
             **/
            uint32_t readAndClearPtrStallFlags();


            /**
             * get DMA stall count. This counter increments if the Multiplexer from
             * DMA Engine to PCIe Interface is busy. This happens when the overall
             * data volume summed over all DMA channels is exceeding the PCIe link
             * capacity.
             * @return DMA stall count in number of clock cycles
             **/
            uint32_t stallCount();

            /**
             * clear the DMA stall counter
             **/
            void clearStallCount();

            /**
             * get current event count
             * @return number of events handled
             **/
            uint32_t eventCount();

            /**
             * clear event counter
             **/
            void clearEventCount();

            /**
             * set DMA readout event rate limit
             * @param rate upper limit on event readout rate in Hz. Set to 0
             * to disable the rate limit.
             * @param pcie_gen PCIe generation, allowed values: 1, 2(default)
             **/
            void setRateLimit( uint32_t rate, uint32_t pcie_gen = 2 );

            /**
             * get DMA readout event rate limit
             * @return event rate limit in Hz, 0 if no limit is set.
             **/
            uint32_t rateLimit( uint32_t pcie_gen = 2 );

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
                 std::vector<ScatterGatherEntry> sglist
            );

        protected:
            link     *m_link;
            uint64_t  m_last_ebdm_offset;
            uint64_t  m_last_rbdm_offset;
            uint32_t  m_pci_tag;
            uint32_t  m_outFifoDepth;

            /**
             * Copy scatterlist from librorc::buffer into the BufferDescriptorManager
             * of the CRORC, to be used either as ReportBuffer or as EventBuffer.
             * @param buf librorc::buffer instance to be used as destination buffer
             * @param target_ram 0 for EventBuffer, 1 for ReportBuffer
             * @return 0 on sucess, -1 on error
             **/
            int configureBufferDescriptorRam(buffer *buf, uint32_t target_ram);

            std::vector<ScatterGatherEntry>
            prepareSgList
            (
                 std::vector<ScatterGatherEntry> list
            );

            void
            configureDmaChannelRegisters
            (
                buffer *eventBuffer,
                buffer *reportBuffer,
                uint32_t pcie_packet_size
            );

            void
            pushSglistEntryToRAM
            (
                uint64_t sg_addr,
                uint32_t sg_len,
                uint32_t ctrl
            );

            /**
             * Read out the current DMA configuration
             * @return DMA Packetizer Configuration and Status
             **/
            uint32_t DMAConfig();

            /**
             * setDMAConfig set the DMA Controller operation mode
             * @param config Bit mapping:
             **/
            void setDMAConfig(uint32_t config);

    };

}

#endif /** LIBRORC_DMA_CHANNEL_H */
