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

#include <librorc/registers.h>
#include <librorc/dma_channel.hh>

#include <librorc/device.hh>
#include <librorc/bar.hh>
#include <librorc/buffer.hh>
#include <librorc/link.hh>

#include <pda.h>



namespace LIBRARY_NAME
{

    #define DMACTRL_EBDM_ENABLE_BIT 2
    #define DMACTRL_RBDM_ENABLE_BIT 3

    /** Class that configures a DMA channel which already has a stored scatter gather list */
    #define DMA_CHANNEL_CONFIGURATOR_ERROR 1
    #define SYNC_SOFTWARE_READ_POINTERS (1 << 31)

    typedef struct
    __attribute__((__packed__))
    {
        volatile uint32_t ebdm_software_read_pointer_low;  /** EBDM read pointer low **/
        volatile uint32_t ebdm_software_read_pointer_high; /** EBDM read pointer high **/
        volatile uint32_t rbdm_software_read_pointer_low;  /** RBDM read pointer low **/
        volatile uint32_t rbdm_software_read_pointer_high; /** RBDM read pointer high **/
        volatile uint32_t dma_ctrl;                        /** DMA control register **/
    } SwReadPointerRegisters;

    typedef struct
    __attribute__((__packed__))
    {
        volatile uint32_t ebdm_n_sg_config;                /** EBDM number of sg entries **/
        volatile uint32_t ebdm_buffer_size_low;            /** EBDM buffer size low (in bytes) **/
        volatile uint32_t ebdm_buffer_size_high;           /** EBDM buffer size high (in bytes) **/
        volatile uint32_t rbdm_n_sg_config;                /** RBDM number of sg entries **/
        volatile uint32_t rbdm_buffer_size_low;            /** RBDM buffer size low (in bytes) **/
        volatile uint32_t rbdm_buffer_size_high;           /** RBDM buffer size high (in bytes) **/
        volatile SwReadPointerRegisters swptrs;            /** struct for read pointers and control register **/
    } DmaChannelConfigRegisters;

    class dma_channel_configurator
    {
        public:


            dma_channel_configurator
            (
                uint32_t     pcie_packet_size,
                buffer      *eventBuffer,
                buffer      *reportBuffer,
                link        *link,
                dma_channel *dmaChannel
            )
            {
                m_dma_channel      = dmaChannel;
                m_pcie_packet_size = pcie_packet_size;
                m_eventBuffer      = eventBuffer;
                m_reportBuffer     = reportBuffer;
                m_link             = link;
                m_pci_tag          = (m_link->linkNumber()<<16); // set channel ID as tag
            }

            int32_t configure()
            {
                try
                {
                    fillConfigurationStructure();
                    m_dma_channel->setPciePacketSize(m_pcie_packet_size);
                    copyConfigToDevice();
                }
                catch(...){ return -1; }

                return(0);
            }

            void setOffsets
            (
                uint64_t eboffset,
                uint64_t rboffset
            )
            {
                SwReadPointerRegisters offsets;
                offsets.ebdm_software_read_pointer_low  = (uint32_t)(eboffset & 0xffffffff);
                offsets.ebdm_software_read_pointer_high = (uint32_t)(eboffset>>32 & 0xffffffff);
                offsets.rbdm_software_read_pointer_low  = (uint32_t)(rboffset & 0xffffffff);
                offsets.rbdm_software_read_pointer_high = (uint32_t)(rboffset>>32 & 0xffffffff);

                offsets.dma_ctrl =
                    SYNC_SOFTWARE_READ_POINTERS | // sync pointers)
                    m_pci_tag | // set channel ID as tag
                    (1<<DMACTRL_EBDM_ENABLE_BIT)  | // enable EB
                    (1<<DMACTRL_RBDM_ENABLE_BIT)  | // enable RB
                    (1<<0);   // enable DMA engine

                m_link->memcopy( RORC_REG_EBDM_SW_READ_POINTER_L, &offsets, sizeof(offsets) );
            }

        protected:
            dma_channel            *m_dma_channel;
            buffer                 *m_eventBuffer;
            buffer                 *m_reportBuffer;
            uint32_t                m_pcie_packet_size;
            uint32_t                m_pci_tag;
            DmaChannelConfigRegisters m_config;
            link                   *m_link;

            void fillConfigurationStructure()
            {
                uint64_t eb_size = m_eventBuffer->getPhysicalSize();
                m_config.ebdm_n_sg_config      = m_eventBuffer->getnSGEntries();
                m_config.ebdm_buffer_size_low  = (eb_size & 0xffffffff);
                m_config.ebdm_buffer_size_high = (eb_size >> 32);

                uint64_t rb_size = m_reportBuffer->getPhysicalSize();
                m_config.rbdm_n_sg_config      = m_reportBuffer->getnSGEntries();
                m_config.rbdm_buffer_size_low  = (rb_size & 0xffffffff);
                m_config.rbdm_buffer_size_high = (rb_size >> 32);

                uint64_t ebrdptr = eb_size - m_pcie_packet_size;
                m_config.swptrs.ebdm_software_read_pointer_low  = (ebrdptr & 0xffffffff);
                m_config.swptrs.ebdm_software_read_pointer_high = (ebrdptr >> 32);

                uint64_t rbrdptr = rb_size - sizeof(EventDescriptor);
                m_config.swptrs.rbdm_software_read_pointer_low  = (rbrdptr & 0xffffffff);
                m_config.swptrs.rbdm_software_read_pointer_high = (rbrdptr >> 32);

                m_config.swptrs.dma_ctrl = SYNC_SOFTWARE_READ_POINTERS | m_pci_tag;
            }

            void copyConfigToDevice()
            {
                m_link->memcopy( RORC_REG_EBDM_N_SG_CONFIG, &m_config, sizeof(m_config) );
                DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                        "Setting ptrs: RBDM=%08x%08x EBDM=%08x%08x\n",
                        m_config.swptrs.rbdm_software_read_pointer_high, m_config.swptrs.rbdm_software_read_pointer_low,
                        m_config.swptrs.ebdm_software_read_pointer_high, m_config.swptrs.ebdm_software_read_pointer_low);
            }
    };



/**PUBLIC:*/

dma_channel::dma_channel
(
    uint32_t              channel_number,
    uint32_t              pcie_packet_size,
    device               *dev,
    bar                  *bar,
    buffer               *eventBuffer,
    buffer               *reportBuffer,
    EventStreamDirection  esType
)
{
    initMembers(pcie_packet_size, dev, bar, channel_number, eventBuffer, reportBuffer, esType);
    prepareBuffers();
}

dma_channel::~dma_channel()
{
    // TODO: no check if disable() failed, but does this matter at this point?!
    disable();
    delete(m_channelConfigurator);

    if(m_reportBuffer != NULL)
    { m_reportBuffer->clear(); }

    if(m_link != NULL)
    { delete m_link; }
}



void
dma_channel::enable()
{
    if( (!m_eventBuffer)||(!m_reportBuffer) )
    { throw LIBRORC_DMA_CHANNEL_ERROR_ENABLE_FAILED; }

    setSuspend(0);
    setEbdmEnable(1);
    setRbdmEnable(1);

    setDMAConfig( DMAConfig() | 0x01 );
}

int
dma_channel::disable()
{
    uint32_t timeout = LIBRORC_DMA_CHANNEL_TIMEOUT;
    // suspend DMA transfer only if active
    if( m_link->dmaEngineIsActive() )
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

    setEbdmEnable(0);
    setRbdmEnable(0);

    /** Reset DFIFO, disable DMA engine, disable SUSPEND */
    setDMAConfig(0X00000002);

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
    m_channelConfigurator->setOffsets(eboffset, rboffset);
    m_last_ebdm_offset = eboffset;

    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
            "Setting ptrs: RBDM=%016lx EBDM=%016lx\n",
            rboffset, eboffset);
}


//TODO : this is protected when hlt out writer is refactored
void
dma_channel::setDMAConfig(uint32_t config)
{
    m_link->setPciReg(RORC_REG_DMA_CTRL, config);
}

uint32_t
dma_channel::DMAConfig()
{
    return m_link->pciReg(RORC_REG_DMA_CTRL);
}


//TODO : this is protected when hlt out writer is refactored
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
{
    return m_link->pciReg(RORC_REG_DMA_PKT_SIZE);
}



//---checked global

void
dma_channel::setEBOffset
(
    uint64_t offset
)
{
    m_link->memcopy( RORC_REG_EBDM_SW_READ_POINTER_L, &offset, sizeof(offset) );
    setDMAConfig( DMAConfig() | (1<<31) );
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
{
    return m_last_ebdm_offset;
}



uint64_t
dma_channel::getLastRBOffset()
{
    return m_last_rbdm_offset;
}



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
    setDMAConfig( DMAConfig() | (1<<31) );
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
{
    return ((DMAConfig()>>7) & 0x01);
}


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

    /** Write librorc_dma_desc to RORC BufferDescriptorManager */
    m_link->memcopy( RORC_REG_SGENTRY_ADDR_LOW, &sg_entry, sizeof(sg_entry) );
    // write to CTRL may only happen once, so this is not included into the memcopy
    m_link->setPciReg(RORC_REG_SGENTRY_CTRL, ctrl);
}





/**PROTECTED:*/
    void
    dma_channel::initMembers
    (
        uint32_t              pcie_packet_size,
        device               *dev,
        bar                  *bar,
        uint32_t              channel_number,
        buffer               *eventBuffer,
        buffer               *reportBuffer,
        EventStreamDirection  esType
    )
    {
        m_channel_number = channel_number;
        m_bar            = bar;
        m_eventBuffer    = eventBuffer;
        m_reportBuffer   = reportBuffer;
        m_link           = new link(bar, channel_number);
        m_esType         = esType;

        if(m_reportBuffer != NULL)
        {
            m_reportBuffer->clear();
            m_last_rbdm_offset
                = m_reportBuffer->getPhysicalSize() - sizeof(EventDescriptor);
        }

        if(m_eventBuffer != NULL)
        {
            m_last_ebdm_offset
                = m_eventBuffer->getPhysicalSize() - pcie_packet_size;
        }

        m_channelConfigurator
            = new dma_channel_configurator
                (pcie_packet_size, m_eventBuffer, m_reportBuffer, m_link, this);
    }



    void
    dma_channel::prepareBuffers()
    {
        if( (m_eventBuffer==NULL) || (m_reportBuffer==NULL) )
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

        if( m_esType == kEventStreamToHost )
        {
            if(configureBufferDescriptorRam(m_eventBuffer, 0) < 0)
            { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }
        }

        if( m_esType == kEventStreamToDevice)
        { m_outFifoDepth = outFifoDepth(); }

        if(configureBufferDescriptorRam(m_reportBuffer, 1) < 0)
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

        if( m_channelConfigurator->configure() < 0)
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }
    }

    link*
    dma_channel::getLink()
    {
        return m_link;
    }


    int
    dma_channel::configureBufferDescriptorRam
    (
        buffer *buf,
        uint32_t target_ram
    )
    {
        if( target_ram > 1 )
        { return -1; }

        // get maximum number of scatter gather entries supported in DMA channel firmware
        uint32_t srcreg = (target_ram) ? RORC_REG_RBDM_N_SG_CONFIG : RORC_REG_EBDM_N_SG_CONFIG;
        uint32_t bdcfg = m_link->pciReg(srcreg);
        if(buf->getnSGEntries() >= (bdcfg >> 16) )
        { return -1; }

        // get scatter gather list from PDA
        std::vector<ScatterGatherEntry> sglist = buf->sgList();
        std::vector<ScatterGatherEntry>::iterator iter;
        uint32_t entry_addr = 0;
        uint32_t ctrl;
        // write scatter gather list into BufferDescriptorRAM
        for(iter=sglist.begin(); iter!=sglist.end(); iter++)
        {
            // TODO: handle sg->length > 32bit
            ctrl = SGCTRL_WRITE_ENABLE | entry_addr;
            ctrl |= (target_ram) ? SGCTRL_TARGET_RBDMRAM : SGCTRL_TARGET_EBDMRAM;
            pushSglistEntryToRAM(iter->pointer, iter->length, ctrl);
            entry_addr++;
        }
        // clear trailing descriptor entry
        ctrl = SGCTRL_WRITE_ENABLE | entry_addr;
        ctrl |= (target_ram) ? SGCTRL_TARGET_RBDMRAM : SGCTRL_TARGET_EBDMRAM;
        pushSglistEntryToRAM(0, 0, ctrl);
        return 0;
    }


    uint32_t
    dma_channel::getEBDMnSGEntries()
    {
        return m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG) & 0x0000ffff;
    }


    uint32_t
    dma_channel::getRBDMnSGEntries()
    {
        return m_link->pciReg(RORC_REG_RBDM_N_SG_CONFIG) & 0x0000ffff;
    }


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


    /**
     * set suspend before disabling DMA engine. DMA-Busy will only go low
     * when suspend is set. Clear suspend before starting any readout.
     **/
    void
    dma_channel::setSuspend
    (
        uint32_t value
    )
    {
        uint32_t dmacfg = DMAConfig();
        // clear suspend bit and set new value
        dmacfg &= ~(1<<10);
        dmacfg |= ( (value&1)<<10 );
        setDMAConfig(dmacfg);
    }
    
    uint32_t
    dma_channel::getSuspend()
    {
        return ((DMAConfig()>>10) & 1);
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
        std::vector<ScatterGatherEntry>::iterator iter;
        for(iter=sglist.begin(); iter!=sglist.end(); iter++)
        {
            uint32_t ctrl = SGCTRL_WRITE_ENABLE | SGCTRL_TARGET_EBDMRAM;
            // add EOE flag for last entry
            if( (iter+1) == sglist.end() )
            { ctrl |= SGCTRL_EOE_FLAG; }
            pushSglistEntryToRAM(iter->pointer, iter->length, ctrl);
        }
    }

}
