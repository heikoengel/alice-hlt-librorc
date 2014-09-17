/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.1
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

#include <librorc/registers.h>
#include <librorc/dma_channel.hh>

#include <librorc/device.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/buffer.hh>

#include <pda.h>



namespace LIBRARY_NAME
{

    /** Class that programs a scatter-gather list into a device */
    #define BUFFER_SGLIST_PROGRAMMER_ERROR 1
    #define DMACTRL_EBDM_ENABLE_BIT 2
    #define DMACTRL_RBDM_ENABLE_BIT 3

    class buffer_sglist_programmer
    {
        public:

            buffer_sglist_programmer
            (
                dma_channel *dmaChannel,
                buffer      *buf,
                uint32_t     target
            )
            {
                m_buffer         = buf;
                m_channel        = dmaChannel;
                m_link           = dmaChannel->getLink();
                m_bdcfg          = 0;
                m_sglist         = NULL;
                m_target_ram     = target;
            }

            int32_t program()
            {
                try
                {
                    readSelectedRAMStatus();
                    CheckSglistFitsIntoDescriptorRAM();
                    getSglistFromPDA();
                    programSglistIntoDescriptorRAM();
                }
                catch(...){ return -1; }

                return(0);
            }


        protected:
            uint32_t                 m_target_ram;
            buffer                  *m_buffer;
            uint32_t                 m_bdcfg;
            DMABuffer_SGNode        *m_sglist;
            link                    *m_link;
            dma_channel             *m_channel;

            void readSelectedRAMStatus()
            {
                switch(m_target_ram)
                {
                    case 0:
                        { m_bdcfg = m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG); }
                        break;
                    case 1:
                        { m_bdcfg = m_link->pciReg(RORC_REG_EBDM_N_SG_CONFIG); }
                        break;
                    default:
                    { throw BUFFER_SGLIST_PROGRAMMER_ERROR; }
                }
            }

            void CheckSglistFitsIntoDescriptorRAM()
            {
                if(m_buffer->getnSGEntries() >= (m_bdcfg >> 16) )
                { throw BUFFER_SGLIST_PROGRAMMER_ERROR; }
            }

            void getSglistFromPDA()
            {
                if(PDA_SUCCESS != DMABuffer_getSGList(m_buffer->getPDABuffer(), &m_sglist) )
                { throw BUFFER_SGLIST_PROGRAMMER_ERROR; }
            }

            void programSglistIntoDescriptorRAM()
            {
                uint64_t i = 0;
                uint32_t ctrl;
                // write scatter gather list into BufferDescriptorRAM
                for(DMABuffer_SGNode *sg=m_sglist; sg!=NULL; sg=sg->next)
                {
                    ctrl = SGCTRL_WRITE_ENABLE | (uint32_t)i;
                    ctrl |= (m_target_ram) ? SGCTRL_TARGET_RBDMRAM : SGCTRL_TARGET_EBDMRAM;
                    m_channel->pushSglistEntryToRAM((uint64_t)sg->d_pointer, sg->length, ctrl);
                    i++;
                }
                // clear trailing descriptor entry
                ctrl = SGCTRL_WRITE_ENABLE | (uint32_t)i;
                ctrl |= (m_target_ram) ? SGCTRL_TARGET_RBDMRAM : SGCTRL_TARGET_EBDMRAM;
                m_channel->pushSglistEntryToRAM(0, 0, ctrl);
            }

    };



    /** Class that configures a DMA channel which already has a stored scatter gather list */
    #define DMA_CHANNEL_CONFIGURATOR_ERROR 1
    #define SYNC_SOFTWARE_READ_POINTERS (1 << 31)
    #define SET_CHANNEL_AS_PCIE_TAG (m_channel_id << 16)

    class dma_channel_configurator
    {
        public:
            typedef struct
            __attribute__((__packed__))
            {
                volatile uint32_t ebdm_software_read_pointer_low;  /** EBDM read pointer low **/
                volatile uint32_t ebdm_software_read_pointer_high; /** EBDM read pointer high **/
                volatile uint32_t rbdm_software_read_pointer_low;  /** RBDM read pointer low **/
                volatile uint32_t rbdm_software_read_pointer_high; /** RBDM read pointer high **/
                volatile uint32_t dma_ctrl;                        /** DMA control register **/
            } librorc_buffer_software_pointers;

            typedef struct
            __attribute__((__packed__))
            {
                volatile uint32_t ebdm_n_sg_config;                /** EBDM number of sg entries **/
                volatile uint32_t ebdm_buffer_size_low;            /** EBDM buffer size low (in bytes) **/
                volatile uint32_t ebdm_buffer_size_high;           /** EBDM buffer size high (in bytes) **/
                volatile uint32_t rbdm_n_sg_config;                /** RBDM number of sg entries **/
                volatile uint32_t rbdm_buffer_size_low;            /** RBDM buffer size low (in bytes) **/
                volatile uint32_t rbdm_buffer_size_high;           /** RBDM buffer size high (in bytes) **/
                volatile librorc_buffer_software_pointers swptrs;  /** struct for read pointers nad control register **/
            } librorc_channel_config;

            dma_channel_configurator
            (
                uint32_t     pcie_packet_size,
                uint32_t     channel_id,
                uint32_t     base,
                buffer      *eventBuffer,
                buffer      *reportBuffer,
                bar         *bar,
                dma_channel *dmaChannel
            )
            {
                m_dma_channel      = dmaChannel;
                m_pcie_packet_size = pcie_packet_size;
                m_eventBuffer      = eventBuffer;
                m_reportBuffer     = reportBuffer;
                m_channel_id       = channel_id;
                m_base             = base;
                m_bar              = bar;
            }

            int32_t configure()
            {
                try
                {
                    fillConfigurationStructure();
                    setPciePacketSize(m_pcie_packet_size);
                    copyConfigToDevice();
                }
                catch(...){ return -1; }

                return(0);
            }

            void setPciePacketSize(uint32_t packet_size)
            {
                m_pcie_packet_size = packet_size;
                checkPacketSize();
                /**
                 * The packet size is located in RORC_REG_DMA_PKT_SIZE:
                 * max_packet_size = RORC_REG_DMA_PKT_SIZE[9:0]
                 * packet_size is in bytes, but the FW expects #DWs -> divide size by 4
                 * write stuff to channel after this
                 */
                m_dma_channel->getLink()->setPciReg
                    (RORC_REG_DMA_PKT_SIZE, ((packet_size >> 2) & 0x3ff) );
            }

            uint32_t pciePacketSize()
            {
                return(m_pcie_packet_size);
            }

            void setOffsets
            (
                uint64_t eboffset,
                uint64_t rboffset
            )
            {
                librorc_buffer_software_pointers offsets;
                offsets.ebdm_software_read_pointer_low  = (uint32_t)(eboffset & 0xffffffff);
                offsets.ebdm_software_read_pointer_high = (uint32_t)(eboffset>>32 & 0xffffffff);
                offsets.rbdm_software_read_pointer_low  = (uint32_t)(rboffset & 0xffffffff);
                offsets.rbdm_software_read_pointer_high = (uint32_t)(rboffset>>32 & 0xffffffff);

                offsets.dma_ctrl =
                    SYNC_SOFTWARE_READ_POINTERS | // sync pointers)
                    SET_CHANNEL_AS_PCIE_TAG | // set channel ID as tag
                    (1<<DMACTRL_EBDM_ENABLE_BIT)  | // enable EB
                    (1<<DMACTRL_RBDM_ENABLE_BIT)  | // enable RB
                    (1<<0);   // enable DMA engine

                m_bar->memcopy
                (
                    (librorc_bar_address)(m_base + RORC_REG_EBDM_SW_READ_POINTER_L),
                    &offsets,
                    sizeof(librorc_buffer_software_pointers)
                );
            }

        protected:
            dma_channel            *m_dma_channel;
            buffer                 *m_eventBuffer;
            buffer                 *m_reportBuffer;
            uint32_t                m_pcie_packet_size;
            uint32_t                m_channel_id;
            librorc_channel_config  m_config;
            uint32_t                m_base;
            bar                    *m_bar;

            void checkPacketSize()
            {
                /*
                 * Rules
                 * - multiple of 4DWs / 16 bytes
                 * - must not be zero
                 * - less or equal 512 for HLT_OUT
                 * - less or equal for HLT_IN
                 **/
                if( (m_pcie_packet_size & 0xf) || (m_pcie_packet_size==0) ||
                        (m_pcie_packet_size>512 && m_dma_channel->m_esType==LIBRORC_ES_TO_DEVICE) ||
                        (m_pcie_packet_size>256 && m_dma_channel->m_esType==LIBRORC_ES_TO_HOST) )
                { throw DMA_CHANNEL_CONFIGURATOR_ERROR; }
            }

            void fillConfigurationStructure()
            {
                m_config.ebdm_n_sg_config      = m_eventBuffer->getnSGEntries();
                m_config.ebdm_buffer_size_low  = bufferDescriptorManagerBufferSizeLow(m_eventBuffer);
                m_config.ebdm_buffer_size_high = bufferDescriptorManagerBufferSizeHigh(m_eventBuffer);

                m_config.rbdm_n_sg_config      = m_reportBuffer->getnSGEntries();
                m_config.rbdm_buffer_size_low  = bufferDescriptorManagerBufferSizeLow(m_reportBuffer);
                m_config.rbdm_buffer_size_high = bufferDescriptorManagerBufferSizeHigh(m_reportBuffer);

                m_config.swptrs.ebdm_software_read_pointer_low  = softwareReadPointerLow(m_eventBuffer, m_pcie_packet_size);
                m_config.swptrs.ebdm_software_read_pointer_high = softwareReadPointerHigh(m_eventBuffer, m_pcie_packet_size);

                m_config.swptrs.rbdm_software_read_pointer_low  = softwareReadPointerLow(m_reportBuffer, sizeof(librorc_event_descriptor));
                m_config.swptrs.rbdm_software_read_pointer_high = softwareReadPointerHigh(m_reportBuffer, sizeof(librorc_event_descriptor));

                m_config.swptrs.dma_ctrl = SYNC_SOFTWARE_READ_POINTERS | SET_CHANNEL_AS_PCIE_TAG;
            }

            uint32_t
            bufferDescriptorManagerBufferSizeLow(buffer *buffer)
            {
                return(buffer->getPhysicalSize() & 0xffffffff);
            }

            uint32_t
            bufferDescriptorManagerBufferSizeHigh(buffer *buffer)
            {
                return(buffer->getPhysicalSize() >> 32);
            }

            uint32_t
            softwareReadPointerLow
            (
                buffer   *buffer,
                uint32_t  offset
            )
            {
                return( (buffer->getPhysicalSize() - offset) & 0xffffffff );
            }

            uint32_t
            softwareReadPointerHigh
            (
                buffer   *buffer,
                uint32_t  offset
            )
            {
                return( (buffer->getPhysicalSize() - offset) >> 32 );
            }

            void copyConfigToDevice()
            {
                m_bar->memcopy
                (
                    (librorc_bar_address)(m_base+RORC_REG_EBDM_N_SG_CONFIG),
                    &m_config,
                    sizeof(librorc_channel_config)
                );
                DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                        "Setting ptrs: RBDM=%016lx EBDM=%016lx\n",
                        (((uint64_t)m_config.swptrs.rbdm_software_read_pointer_high<<32)+m_config.swptrs.rbdm_software_read_pointer_low),
                        (((uint64_t)m_config.swptrs.ebdm_software_read_pointer_high<<32)+m_config.swptrs.ebdm_software_read_pointer_low));
            }
    };



/**PUBLIC:*/

dma_channel::dma_channel
(
    uint32_t  channel_number,
    uint32_t  pcie_packet_size,
    device   *dev,
    bar      *bar,
    buffer   *eventBuffer,
    buffer   *reportBuffer,
    LibrorcEsType esType
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

    if(m_sm != NULL)
    { delete m_sm; }

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

uint32_t
dma_channel::isEBDMEnabled()
{
    return ((DMAConfig()>>DMACTRL_EBDM_ENABLE_BIT) & 0x01);
}

unsigned int
dma_channel::isRBDMEnabled()
{
    return ((DMAConfig()>>DMACTRL_RBDM_ENABLE_BIT) & 0x01);
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
    m_channelConfigurator->setPciePacketSize(packet_size);
}

uint32_t
dma_channel::pciePacketSize()
{
    return m_channelConfigurator->pciePacketSize();
}



//---checked global

void
dma_channel::setEBOffset
(
    uint64_t offset
)
{
    m_bar->memcopy
    (
        (librorc_bar_address)(m_link->base() + RORC_REG_EBDM_SW_READ_POINTER_L),
        &offset,
        sizeof(offset)
    );

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
    m_bar->memcopy
    (
        (librorc_bar_address)(m_link->base()+RORC_REG_RBDM_SW_READ_POINTER_L),
        &offset,
        sizeof(offset)
    );

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
    librorc_sg_entry_config sg_entry;
    /** Convert sg list into CRORC compatible format */
    sg_entry.sg_addr_low  = (uint32_t)(sg_addr & 0xffffffff);
    sg_entry.sg_addr_high = (uint32_t)(sg_addr >> 32);
    sg_entry.sg_len       = (uint32_t)(sg_len & 0xffffffff);

    /** Write librorc_dma_desc to RORC BufferDescriptorManager */
    m_link->getBar()->memcopy
        ( (librorc_bar_address)(m_link->base()+RORC_REG_SGENTRY_ADDR_LOW), &sg_entry, sizeof(sg_entry) );
    // write to CTRL may only happen once, so this is not included into the memcopy
    m_link->setPciReg(RORC_REG_SGENTRY_CTRL, ctrl);
}


/**PROTECTED:*/
    void
    dma_channel::initMembers
    (
        uint32_t  pcie_packet_size,
        device   *dev,
        bar      *bar,
        uint32_t  channel_number,
        buffer   *eventBuffer,
        buffer   *reportBuffer,
        LibrorcEsType esType
    )
    {
        m_channel_number = channel_number;
        m_dev            = dev;
        m_bar            = bar;
        m_eventBuffer    = eventBuffer;
        m_reportBuffer   = reportBuffer;
        m_link           = new link(bar, channel_number);
        m_sm             = new sysmon(bar);
        m_esType         = esType;

        if(m_reportBuffer != NULL)
        {
            m_reportBuffer->clear();
            m_last_rbdm_offset
                = m_reportBuffer->getPhysicalSize() - sizeof(librorc_event_descriptor);
        }

        if(m_eventBuffer != NULL)
        {
            m_last_ebdm_offset
                = m_eventBuffer->getPhysicalSize() - pcie_packet_size;
        }

        m_channelConfigurator
            = new dma_channel_configurator
                (pcie_packet_size, channel_number, m_link->base(),
                    m_eventBuffer, m_reportBuffer, m_bar, this);
    }



    void
    dma_channel::prepareBuffers()
    {
        if( m_channel_number >= m_sm->numberOfChannels() )
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

        if( (m_eventBuffer==NULL) || (m_reportBuffer==NULL) )
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

        if( m_esType == LIBRORC_ES_TO_HOST )
        {
            if(programSglistForEventBuffer(m_eventBuffer) < 0)
            { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }
        }

        if( m_esType == LIBRORC_ES_TO_DEVICE )
        { m_outFifoDepth = outFifoDepth(); }

        if(programSglistForReportBuffer(m_reportBuffer) < 0)
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

        if( m_channelConfigurator->configure() < 0)
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }
    }

    link*
    dma_channel::getLink()
    {
        return m_link;
    }



    int32_t
    dma_channel::programSglistForEventBuffer
    (
        buffer *buf
    )
    {
        buffer_sglist_programmer
            programmer(this, buf, 0);
        return(programmer.program());
    }



    int32_t
    dma_channel::programSglistForReportBuffer
    (
        buffer *buf
    )
    {
        buffer_sglist_programmer
            programmer(this, buf, 1);
        return(programmer.program());
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
        std::vector<librorc_sg_entry> sglist
    )
    {
        std::vector<librorc_sg_entry>::iterator iter;
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
