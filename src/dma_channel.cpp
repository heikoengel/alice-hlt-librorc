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

#include <pda.h>
#include <librorc/dma_channel.hh>
#include <librorc/registers.h>
#include <librorc/buffer.hh>
#include <librorc/link.hh>


namespace LIBRARY_NAME
{
#define DMA_CHANNEL_TIMEOUT 10000

#define DMACTRL_ENABLE_BIT 0
#define DMACTRL_FIFO_RESET_BIT 1
#define DMACTRL_EBDM_ENABLE_BIT 2
#define DMACTRL_RBDM_ENABLE_BIT 3
#define DMACTRL_BUSY_BIT 7
#define DMACTRL_SUSPEND_BIT 10
#define DMACTRL_SYNC_SWRDPTRS_BIT 31

#define SGCTRL_WRITE_ENABLE (1<<31)
#define SGCTRL_TARGET_EBDMRAM (0<<30)
#define SGCTRL_TARGET_RBDMRAM (1<<30)
#define SGCTRL_EOE_FLAG (1<<0)


typedef struct
__attribute__((__packed__))
{
    volatile uint32_t ebdm_sw_read_pointer_low;  /** EBDM read pointer low **/
    volatile uint32_t ebdm_sw_read_pointer_high; /** EBDM read pointer high **/
    volatile uint32_t rbdm_sw_read_pointer_low;  /** RBDM read pointer low **/
    volatile uint32_t rbdm_sw_read_pointer_high; /** RBDM read pointer high **/
    volatile uint32_t dma_ctrl;                  /** DMA control register **/
} SwReadPointerRegisters;


    typedef struct
__attribute__((__packed__))
{
    volatile uint32_t ebdm_n_sg_config;      /** EBDM number of sg entries **/
    volatile uint32_t ebdm_buffer_size_low;  /** EBDM buffer size low (in bytes) **/
    volatile uint32_t ebdm_buffer_size_high; /** EBDM buffer size high (in bytes) **/
    volatile uint32_t rbdm_n_sg_config;      /** RBDM number of sg entries **/
    volatile uint32_t rbdm_buffer_size_low;  /** RBDM buffer size low (in bytes) **/
    volatile uint32_t rbdm_buffer_size_high; /** RBDM buffer size high (in bytes) **/
    volatile SwReadPointerRegisters swptrs;  /** struct for read pointers and control register **/
} DmaChannelConfigRegisters;


typedef struct
__attribute__((__packed__))
{
    uint32_t sg_addr_low;  /** lower part of sg address **/
    uint32_t sg_addr_high; /** higher part of sg address **/
    uint32_t sg_len;       /** total length of sg entry in bytes **/
} ScatterGatherEntryRegisterMapping;


dma_channel::dma_channel
(
    link *link
)
{
    m_link    = link;
    m_pci_tag = (m_link->linkNumber() << 16); // use channel ID as PCIe tag
}


dma_channel::~dma_channel()
{ m_link = NULL; }


void
dma_channel::enable()
{
    uint32_t dmacfg = DMAConfig();
    dmacfg |= (1<<DMACTRL_EBDM_ENABLE_BIT) |
        (1<<DMACTRL_RBDM_ENABLE_BIT) |
        (1<<DMACTRL_ENABLE_BIT);
    setDMAConfig( dmacfg );
}

uint32_t
dma_channel::getEnable()
{ return ((DMAConfig() >> DMACTRL_ENABLE_BIT) & 1); }

int
dma_channel::disable()
{
    uint32_t timeout = DMA_CHANNEL_TIMEOUT;
    // suspend DMA transfer only if active
    if( getEnable() )
    {
        setSuspend(1);

        /**
         * Timeout handling: simply continue after loop.
         * This may lead to wrong calculated/reported event sizes
         * but as we ran into the timeout there's something wrong anyway
         **/
        while(timeout!=0 && getDMABusy())
        {
            usleep(100);
            timeout--;
        }
    }

    /** Reset DFIFO, disable DMA engine, disable SUSPEND, disable EBDM/RBDM */
    uint32_t dmacfg = (1<<DMACTRL_FIFO_RESET_BIT);
    setDMAConfig(dmacfg);

    return (timeout!=0) ? 0 : -1;
}

void
dma_channel::setEbdmEnable
(
    uint32_t enable
)
{
    uint32_t dmacfg = DMAConfig();
    dmacfg &= ~(1<<DMACTRL_EBDM_ENABLE_BIT);
    dmacfg |= ((enable&1)<<DMACTRL_EBDM_ENABLE_BIT);
    setDMAConfig(dmacfg);
}

void
dma_channel::setRbdmEnable
(
    uint32_t enable
)
{
    uint32_t dmacfg = DMAConfig();
    dmacfg &= ~(1<<DMACTRL_RBDM_ENABLE_BIT);
    dmacfg |= ((enable&1)<<DMACTRL_RBDM_ENABLE_BIT);
    setDMAConfig(dmacfg);
}


void
dma_channel::setBufferOffsetsOnDevice
(
    uint64_t eboffset,
    uint64_t rboffset
)
{
    SwReadPointerRegisters offsets;
    offsets.ebdm_sw_read_pointer_low  = (uint32_t)(eboffset & 0xffffffff);
    offsets.ebdm_sw_read_pointer_high = (uint32_t)(eboffset>>32 & 0xffffffff);
    offsets.rbdm_sw_read_pointer_low  = (uint32_t)(rboffset & 0xffffffff);
    offsets.rbdm_sw_read_pointer_high = (uint32_t)(rboffset>>32 & 0xffffffff);

    offsets.dma_ctrl =
        (1<<DMACTRL_SYNC_SWRDPTRS_BIT) | // sync pointers)
        m_pci_tag | // set channel ID as tag
        (1<<DMACTRL_EBDM_ENABLE_BIT)  | // enable EB
        (1<<DMACTRL_RBDM_ENABLE_BIT)  | // enable RB
        (1<<DMACTRL_ENABLE_BIT);   // enable DMA engine

    m_link->memcopy( RORC_REG_EBDM_SW_READ_POINTER_L, &offsets, sizeof(offsets) );
    m_last_ebdm_offset = eboffset;
    m_last_rbdm_offset = rboffset;

    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
            "Setting ptrs: RBDM=%016lx EBDM=%016lx\n",
            rboffset, eboffset);
}


void
dma_channel::setPciePacketSize
(
    uint32_t packet_size
)
{
    /**
     * The packet size is located in RORC_REG_DMA_PKT_SIZE[9:0].
     * packet_size is in bytes, but the FW expects #DWs -> divide size by 4
     */
    uint32_t packet_size_words = ((packet_size >> 2) & 0x3ff);
    m_link->setPciReg(RORC_REG_DMA_PKT_SIZE, packet_size_words );
}


uint32_t
dma_channel::pciePacketSize()
{ return m_link->pciReg(RORC_REG_DMA_PKT_SIZE); }


void
dma_channel::setEBOffset
(
    uint64_t offset
)
{
    m_link->memcopy( RORC_REG_EBDM_SW_READ_POINTER_L, &offset, sizeof(offset) );
    setDMAConfig( DMAConfig() | (1<<DMACTRL_SYNC_SWRDPTRS_BIT) );
    m_last_ebdm_offset = offset;
}


uint64_t
dma_channel::getEBOffset()
{
    return ((uint64_t)m_link->pciReg(RORC_REG_EBDM_SW_READ_POINTER_H) << 32) +
           (uint64_t)m_link->pciReg(RORC_REG_EBDM_SW_READ_POINTER_L);
}


uint64_t
dma_channel::getLastEBOffset()
{ return m_last_ebdm_offset; }


uint64_t
dma_channel::getLastRBOffset()
{ return m_last_rbdm_offset; }


uint64_t
dma_channel::getEBDMAOffset()
{
    return ((uint64_t)m_link->pciReg(RORC_REG_EBDM_FPGA_WRITE_POINTER_H) << 32) +
           (uint64_t)m_link->pciReg(RORC_REG_EBDM_FPGA_WRITE_POINTER_L);
}


void
dma_channel::setRBOffset
(
    uint64_t offset
)
{
    m_link->memcopy( RORC_REG_RBDM_SW_READ_POINTER_L, &offset, sizeof(offset) );
    setDMAConfig( DMAConfig() | (1<<DMACTRL_SYNC_SWRDPTRS_BIT) );
    m_last_rbdm_offset = offset;
}


uint64_t
dma_channel::getRBOffset()
{
    return ((uint64_t)m_link->pciReg(RORC_REG_RBDM_SW_READ_POINTER_H) << 32) +
           (uint64_t)m_link->pciReg(RORC_REG_RBDM_SW_READ_POINTER_L);
}


uint64_t
dma_channel::getRBDMAOffset()
{
    return ((uint64_t)m_link->pciReg(RORC_REG_RBDM_FPGA_WRITE_POINTER_H) << 32) +
           (uint64_t)m_link->pciReg(RORC_REG_RBDM_FPGA_WRITE_POINTER_L);
}


uint32_t
dma_channel::getDMABusy()
{ return ((DMAConfig()>>DMACTRL_BUSY_BIT) & 1); }


uint32_t
dma_channel::outFifoFillState()
{
    uint32_t fill_state = m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG);
    return (fill_state & 0xffff);
}


uint32_t
dma_channel::outFifoDepth()
{
    uint32_t fill_state = m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG);
    return ((fill_state>>16) & 0xffff);
}


int
dma_channel::configure
(
    buffer *eventBuffer,
    buffer *reportBuffer,
    EventStreamDirection esDir,
    uint32_t pcie_packet_size
)
{
    int result;
    if( eventBuffer==NULL || reportBuffer==NULL )
    { return ENODEV; }

    if( esDir == kEventStreamToHost )
    {
        result = configureBufferDescriptorRam(eventBuffer, 0);
        if( result != 0 )
        { return result; }
    }
    else
    { m_outFifoDepth = outFifoDepth(); }

    result = configureBufferDescriptorRam(reportBuffer, 1);
    if( result != 0 )
    { return result; }

    configureDmaChannelRegisters(eventBuffer, reportBuffer, pcie_packet_size);

    return 0;
}


uint32_t
dma_channel::getEBDMNumberOfSgEntries()
{ return (m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG) & 0x0000ffff); }


uint32_t
dma_channel::getEBDMMaxSgEntries()
{ return (m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG) >> 16); }


uint32_t
dma_channel::getRBDMNumberOfSgEntries()
{ return (m_link->pciReg(RORC_REG_RBDM_N_SG_CONFIG) & 0x0000ffff); }


uint32_t
dma_channel::getRBDMMaxSgEntries()
{ return (m_link->pciReg(RORC_REG_RBDM_N_SG_CONFIG) >> 16); }


uint64_t
dma_channel::getEBSize()
{
    return ((uint64_t)m_link->pciReg(RORC_REG_EBDM_BUFFER_SIZE_H) << 32) +
           (uint64_t)m_link->pciReg(RORC_REG_EBDM_BUFFER_SIZE_L);
}


uint64_t
dma_channel::getRBSize()
{
    return ((uint64_t)m_link->pciReg(RORC_REG_RBDM_BUFFER_SIZE_H) << 32) +
           (uint64_t)m_link->pciReg(RORC_REG_RBDM_BUFFER_SIZE_L);
}


void
dma_channel::setSuspend
(
    uint32_t value
)
{
    uint32_t dmacfg = DMAConfig();
    // clear suspend bit and set new value
    dmacfg &= ~(1<<DMACTRL_SUSPEND_BIT);
    dmacfg |= ( (value&1)<<DMACTRL_SUSPEND_BIT);
    setDMAConfig(dmacfg);
}


uint32_t
dma_channel::getSuspend()
{ return ((DMAConfig()>>DMACTRL_SUSPEND_BIT) & 1); }


uint32_t
dma_channel::readAndClearPtrStallFlags()
{
    uint32_t dmacfg = DMAConfig();
    uint32_t stallflags = ( (dmacfg>>11) & 3 );
    if ( stallflags != 0 )
    {
        dmacfg |= (3<<11);
        setDMAConfig(dmacfg);
    }
    return stallflags;
}


uint32_t
dma_channel::stallCount()
{ return m_link->pciReg(RORC_REG_DMA_STALL_CNT); }


void
dma_channel::clearStallCount()
{ m_link->setPciReg(RORC_REG_DMA_STALL_CNT, 0); }


uint32_t
dma_channel::eventCount()
{ return m_link->pciReg(RORC_REG_DMA_N_EVENTS_PROCESSED); }


void
dma_channel::clearEventCount()
{ m_link->setPciReg(RORC_REG_DMA_N_EVENTS_PROCESSED, 0); }


void
dma_channel::setRateLimit
(
    uint32_t rate,
    uint32_t pcie_gen
)
{
    uint32_t clk_freq = (pcie_gen==2) ? 250000000 : 125000000;
    uint32_t waittime = (rate!=0) ? (clk_freq/rate) : 0;
    m_link->setPciReg(RORC_REG_DMA_RATE_LIMITER_WAITTIME, waittime);
}


uint32_t
dma_channel::rateLimit
(
    uint32_t pcie_gen
)
{
    uint32_t clk_freq = (pcie_gen==2) ? 250000000 : 125000000;
    uint32_t waittime = m_link->pciReg(RORC_REG_DMA_RATE_LIMITER_WAITTIME);
    uint32_t rate = (waittime!=0) ? (clk_freq/waittime) : 0;
    return rate;
}


/*******************************************************************
*    HLT-OUT related
******************************************************************/
void
dma_channel::announceEvent
(
    std::vector<ScatterGatherEntry> sglist
)
{
    std::vector<ScatterGatherEntry>::iterator iter, end;
    iter = sglist.begin();
    end = sglist.end();
    while( iter != end )
    {
        uint32_t ctrl = SGCTRL_WRITE_ENABLE | SGCTRL_TARGET_EBDMRAM;
        // add EOE flag for last entry
        if( (iter+1) == sglist.end() )
        { ctrl |= SGCTRL_EOE_FLAG; }
        pushSglistEntryToRAM(iter->pointer, iter->length, ctrl);
        ++iter;
    }
}


/**************************** protected **********************************/

std::vector<ScatterGatherEntry>
dma_channel::prepareSgList
(
    std::vector<ScatterGatherEntry> list
)
{
    std::vector<ScatterGatherEntry> newlist;
    std::vector<ScatterGatherEntry>::iterator iter, end;
    iter = list.begin();
    end = list.end();
    uint64_t cur_offset;
    uint64_t rem_size;
    ScatterGatherEntry entry;

    while( iter != end )
    {
        cur_offset = iter->pointer;
        rem_size = iter->length;
        // split any segment >4GB / 32bit address width
        // into several smaller segments
        while( rem_size>>32 )
        {
            entry.pointer = cur_offset;
            entry.length = (((uint64_t)1)<<32) - PAGE_SIZE;
            newlist.push_back(entry);
            rem_size -= entry.length;
            cur_offset += entry.length;
        }
        entry.pointer = cur_offset;
        entry.length = rem_size;
        newlist.push_back(entry);
        ++iter;
    }
    return newlist;
}


int
dma_channel::configureBufferDescriptorRam
(
    buffer *buf,
    uint32_t target_ram
)
{
    // get maximum number of scatter gather entries supported in DMA channel
    // firmware
    uint32_t max_num_sg =
        (target_ram) ? getRBDMMaxSgEntries() : getEBDMMaxSgEntries();

    // get scatter gather list from PDA and prepare it for the DMA channel
    std::vector<ScatterGatherEntry> sglist = prepareSgList(buf->sgList());

    // make sure scatter gather list fits into RAM
    if(sglist.size() >= max_num_sg )
    { return EFBIG; }

    uint32_t entry_addr = 0;
    uint32_t ctrl;
    // write scatter gather list into BufferDescriptorRAM
    std::vector<ScatterGatherEntry>::iterator iter, end;
    iter = sglist.begin();
    end = sglist.end();
    while( iter != end )
    {
        ctrl = SGCTRL_WRITE_ENABLE | entry_addr;
        ctrl |= (target_ram) ? SGCTRL_TARGET_RBDMRAM : SGCTRL_TARGET_EBDMRAM;
        pushSglistEntryToRAM(iter->pointer, iter->length, ctrl);
        entry_addr++;
        ++iter;
    }
    // clear trailing descriptor entry
    ctrl = SGCTRL_WRITE_ENABLE | entry_addr;
    ctrl |= (target_ram) ? SGCTRL_TARGET_RBDMRAM : SGCTRL_TARGET_EBDMRAM;
    pushSglistEntryToRAM(0, 0, ctrl);
    return 0;
}


void
dma_channel::configureDmaChannelRegisters
(
    buffer *eventBuffer,
    buffer *reportBuffer,
    uint32_t pcie_packet_size
)
{
    uint64_t eb_size = eventBuffer->getPhysicalSize();
    uint64_t rb_size = reportBuffer->getPhysicalSize();
    uint64_t ebrdptr = eb_size - pcie_packet_size;
    uint64_t rbrdptr = rb_size - sizeof(EventDescriptor);

    DmaChannelConfigRegisters chcfg;
    chcfg.ebdm_n_sg_config      = eventBuffer->getnSGEntries();
    chcfg.ebdm_buffer_size_low  = (eb_size & 0xffffffff);
    chcfg.ebdm_buffer_size_high = (eb_size >> 32);
    chcfg.rbdm_n_sg_config      = reportBuffer->getnSGEntries();
    chcfg.rbdm_buffer_size_low  = (rb_size & 0xffffffff);
    chcfg.rbdm_buffer_size_high = (rb_size >> 32);
    chcfg.swptrs.ebdm_sw_read_pointer_low  = (ebrdptr & 0xffffffff);
    chcfg.swptrs.ebdm_sw_read_pointer_high = (ebrdptr >> 32);
    chcfg.swptrs.rbdm_sw_read_pointer_low  = (rbrdptr & 0xffffffff);
    chcfg.swptrs.rbdm_sw_read_pointer_high = (rbrdptr >> 32);
    chcfg.swptrs.dma_ctrl = (1<<DMACTRL_SYNC_SWRDPTRS_BIT) | m_pci_tag;

    m_link->memcopy( RORC_REG_EBDM_N_SG_CONFIG, &chcfg, sizeof(chcfg) );
    setPciePacketSize(pcie_packet_size);
    m_last_rbdm_offset = rbrdptr;
    m_last_ebdm_offset = ebrdptr;
}


/** ctrl encoding:
 * [31]  : write-enable to write sg_entry to RAM
 * [30]  : select target RAM, 0->EventBufferRAM, 1->ReportBufferRAM
 * [29:0]: RAM address
 **/
void
dma_channel::pushSglistEntryToRAM
(
    uint64_t sg_addr,
    uint32_t sg_len,
    uint32_t ctrl
)
{
    ScatterGatherEntryRegisterMapping sg_entry;
    /** Convert sg list into CRORC compatible format */
    sg_entry.sg_addr_low  = (uint32_t)(sg_addr & 0xffffffff);
    sg_entry.sg_addr_high = (uint32_t)(sg_addr >> 32);
    sg_entry.sg_len       = (uint32_t)(sg_len & 0xffffffff);

    // Write scatter gather entry to device
    m_link->memcopy( RORC_REG_SGENTRY_ADDR_LOW, &sg_entry, sizeof(sg_entry) );
    // write to CTRL may only happen once, so this is not included into the memcopy
    m_link->setPciReg(RORC_REG_SGENTRY_CTRL, ctrl);
}


void
dma_channel::setDMAConfig(uint32_t config)
{ m_link->setPciReg(RORC_REG_DMA_CTRL, config); }


uint32_t
dma_channel::DMAConfig()
{ return m_link->pciReg(RORC_REG_DMA_CTRL); }

}
