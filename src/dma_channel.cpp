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

using namespace std;

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

typedef struct
__attribute__((__packed__))
{
    uint32_t sg_addr_low;  /** lower part of sg address **/
    uint32_t sg_addr_high; /** higher part of sg address **/
    uint32_t sg_len;       /** total length of sg entry in bytes **/
    uint32_t ctrl;         /** BDM control register: [31]:we, [30]:sel, [29:0]BRAM addr **/
} librorc_sg_entry_config;

/** extern error number **/
extern int errno;



namespace librorc
{

    /** Class that programs a scatter-gather list into a device */
    #define BUFFER_PREPARER_ERROR 1

    class buffer_preparer
    {
        public:
            uint32_t                 control_flag;
            buffer                  *m_buffer;
            uint32_t                 bdcfg;
            DMABuffer               *pda_dma_buffer;
            DMABuffer_SGNode        *sglist;
            librorc_sg_entry_config  sg_entry;
            dma_channel             *m_dma_channel;
            uint32_t                 m_flag;
            bar                     *m_bar;
            uint32_t                 m_base;


            buffer_preparer
            (
                dma_channel *dmaChannel,
                buffer      *buf,
                bar         *bar,
                uint32_t     base,
                uint32_t     flag
            )
            {
                m_dma_channel  = dmaChannel;
                m_buffer       = buf;
                m_bar          = bar;
                m_base         = base;

                control_flag   = 0;
                bdcfg          = dmaChannel->getPKT(flag);
                pda_dma_buffer = buf->getPDABuffer();
                sglist         = NULL;
                m_flag         = flag;
                sg_entry;
            }

            void convertControlFlag()
            {
                switch(m_flag)
                {
                    case RORC_REG_RBDM_N_SG_CONFIG:
                    { control_flag = 1; }
                    break;

                    case RORC_REG_EBDM_N_SG_CONFIG:
                    { control_flag = 0; }
                    break;

                    default:
                    { throw BUFFER_PREPARER_ERROR; }
                }
            }

            void sglistFitsIntoDRAM()
            {
                if(m_buffer->getnSGEntries() > (bdcfg >> 16) )
                { throw BUFFER_PREPARER_ERROR; }
            }

            void getSglistFromPDA()
            {
                if(PDA_SUCCESS != DMABuffer_getSGList(pda_dma_buffer, &sglist) )
                { throw BUFFER_PREPARER_ERROR; }
            }

            void programSglistIntoDRAM()
            {
                uint64_t i = 0;
                for(DMABuffer_SGNode *sg=sglist; sg!=NULL; sg=sg->next)
                {
                    /** Convert sg list into CRORC compatible format */
                    sg_entry.sg_addr_low  = (uint32_t)( (uint64_t)(sg->d_pointer) & 0xffffffff);
                    sg_entry.sg_addr_high = (uint32_t)( (uint64_t)(sg->d_pointer) >> 32);
                    sg_entry.sg_len       = (uint32_t)(sg->length & 0xffffffff);

                    sg_entry.ctrl = (1 << 31) | (control_flag << 30) | ((uint32_t)i);

                    /** Write librorc_dma_desc to RORC EBDM */
                    m_bar->memcopy
                        ( (librorc_bar_address)(m_base+RORC_REG_SGENTRY_ADDR_LOW), &sg_entry, sizeof(sg_entry) );
                    i++;
                }
            }

            void clearTrailingDRAM()
            {
                memset(&sg_entry, 0, sizeof(sg_entry) );
                m_bar->memcopy
                    ( (librorc_bar_address)(m_base+RORC_REG_SGENTRY_ADDR_LOW), &sg_entry, sizeof(sg_entry) );
            }


            int32_t program()
            {
                try
                {
                    convertControlFlag();
                    sglistFitsIntoDRAM();
                    getSglistFromPDA();
                    programSglistIntoDRAM();
                    clearTrailingDRAM();
                }
                catch(...){ return -1; }

                return(0);
            }

    };


/**PUBLIC:*/

dma_channel::dma_channel
(
    uint32_t  channel_number,
    device   *dev,
    bar      *bar
)
{
    initMembers(channel_number, 0, dev, bar,NULL, NULL);
    prepareBuffers();
}



dma_channel::dma_channel
(
    uint32_t  channel_number,
    uint32_t  pcie_packet_size,
    device   *dev,
    bar      *bar,
    buffer   *eventBuffer,
    buffer   *reportBuffer
)
{
    initMembers(channel_number, pcie_packet_size, dev, bar, eventBuffer, reportBuffer);
    prepareBuffers();
}



dma_channel::~dma_channel()
{
    if(m_reportBuffer != NULL)
    {
        m_reportBuffer->clear();
    }
}



void
dma_channel::enable()
{
    if( (!m_eventBuffer)||(!m_reportBuffer) )
    { throw LIBRORC_DMA_CHANNEL_ERROR_ENABLE_FAILED; }

    setEnableEB(1);
    setEnableRB(1);

    setDMAConfig( getDMAConfig() | 0x01 );
}



void
dma_channel::disable()
{
    setEnableEB(0);

    while(getDMABusy())
    { usleep(100); }

    setEnableRB(0);

    /** Reset DFIFO, disable DMA PKT */
    setDMAConfig(0X00000002);
}



void
dma_channel::waitForGTXDomain()
{
    /**
     * wait for GTX domain to be ready read asynchronous GTX status
     * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
     * & !gtx_in_rst
    **/
    while( (getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
    { usleep(100); }
}

//---checked global



void
dma_channel::waitForCommandTransmissionStatusWord()
{
    /** wait for command transmission status word (CTSTW) from DIU */
    while( getGTX(RORC_REG_DDL_CTSTW) == 0xffffffff )
    { usleep(100); }
}



void
dma_channel::setEnableEB(int32_t enable)
{
    uint32_t bdcfg = getPKT( RORC_REG_DMA_CTRL );
    if(enable)
    {
        setPKT(RORC_REG_DMA_CTRL, ( bdcfg | (1 << 2) ) );
    }
    else
    {
        setPKT(RORC_REG_DMA_CTRL, ( bdcfg & ~(1 << 2) ) );
    }
}



uint32_t
dma_channel::getEnableEB()
{
    return (getPKT( RORC_REG_DMA_CTRL ) >> 2 ) & 0x01;
}



void
dma_channel::setEnableRB
(
    int32_t enable
)
{
    uint32_t bdcfg = getPKT( RORC_REG_DMA_CTRL );
    if(enable)
    {
        setPKT(RORC_REG_DMA_CTRL, ( bdcfg | (1 << 3) ) );
    }
    else
    {
        setPKT(RORC_REG_DMA_CTRL, ( bdcfg & ~(1 << 3) ) );
    }
}



unsigned int
dma_channel::getEnableRB()
{
    return (getPKT( RORC_REG_DMA_CTRL ) >> 3 ) & 0x01;
}



void
dma_channel::setDMAConfig
(
    uint32_t config
)
{
    setPKT(RORC_REG_DMA_CTRL, config);
}



uint32_t
dma_channel::getDMAConfig()
{
    return getPKT(RORC_REG_DMA_CTRL);
}


void
dma_channel::setPciePacketSize
(
    uint32_t packet_size
)
{
    /**
     * packet size is located in RORC_REG_DMA_PKT_SIZE:
     * max_packet_size = RORC_REG_DMA_PKT_SIZE[9:0]
     */
    assert( m_bar!=NULL );

    /**
     * packet size has to be provided as #DWs
     * -> divide size by 4
     */
    uint32_t mp_size = (packet_size >> 2) & 0x3ff;

    /** write to channel*/
    setPKT( RORC_REG_DMA_PKT_SIZE, mp_size );

    m_pcie_packet_size = packet_size;
}


uint32_t
dma_channel::getPciePacketSize()
{
    return m_pcie_packet_size;
}


/** Rework : remove sepparate methods*/
void
dma_channel::setOffsets
(
    uint64_t eboffset,
    uint64_t rboffset
)
{
    assert(m_bar!=NULL);
    librorc_buffer_software_pointers offsets;

    offsets.ebdm_software_read_pointer_low =
        (uint32_t)(eboffset & 0xffffffff);

    offsets.ebdm_software_read_pointer_high =
        (uint32_t)(eboffset >> 32 & 0xffffffff);

    offsets.rbdm_software_read_pointer_low =
        (uint32_t)(rboffset & 0xffffffff);

    offsets.rbdm_software_read_pointer_high =
        (uint32_t)(rboffset >> 32 & 0xffffffff);

    /** set sync-flag in DMA control register
     * TODO: this is the fail-save version. The following getPKT()
     * can be omitted if the library keeps track on any writes to
     * RORC_REG_DMA_CTRL. This would reduce PCIe traffic.
     **/
    offsets.dma_ctrl = ( getPKT(RORC_REG_DMA_CTRL) | (1<<31) );

    m_bar->memcopy( (librorc_bar_address)(m_base + RORC_REG_EBDM_SW_READ_POINTER_L),
                    &offsets, sizeof(offsets) );

    /** save a local copy of the last offsets written to the channel **/
    m_last_ebdm_offset = eboffset;
    m_last_rbdm_offset = rboffset;
}



void
dma_channel::setEBOffset
(
    uint64_t offset
)
{
    assert(m_bar!=NULL);

    m_bar->memcopy( (librorc_bar_address)(m_base + RORC_REG_EBDM_SW_READ_POINTER_L),
                      &offset, sizeof(offset) );

    uint32_t status = getPKT(RORC_REG_DMA_CTRL);
    setPKT(RORC_REG_DMA_CTRL, status | (1 << 31) );

    /** save a local copy of the last offsets written to the channel **/
    m_last_ebdm_offset = offset;
}



uint64_t
dma_channel::getEBOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_EBDM_SW_READ_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_EBDM_SW_READ_POINTER_L);

    return offset;
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
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_EBDM_FPGA_WRITE_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_EBDM_FPGA_WRITE_POINTER_L);

    return offset;
}



void
dma_channel::setRBOffset
(
    uint64_t offset
)
{
    assert(m_bar!=NULL);

    m_bar->memcopy( (librorc_bar_address)(m_base+RORC_REG_RBDM_SW_READ_POINTER_L),
                    &offset, sizeof(offset) );

    uint32_t status = getPKT(RORC_REG_DMA_CTRL);
    setPKT(RORC_REG_DMA_CTRL, status | (1 << 31) );

    /** save a local copy of the last offsets written to the channel **/
    m_last_rbdm_offset = offset;
}



uint64_t
dma_channel::getRBOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_RBDM_SW_READ_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_RBDM_SW_READ_POINTER_L);

    return offset;
}



uint64_t
dma_channel::getRBDMAOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_RBDM_FPGA_WRITE_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_RBDM_FPGA_WRITE_POINTER_L);

    return offset;
}



uint32_t
dma_channel::getEBDMnSGEntries()
{
    return (getPKT(RORC_REG_EBDM_N_SG_CONFIG) & 0x0000ffff);
}



uint32_t
dma_channel::getRBDMnSGEntries()
{
    return (getPKT(RORC_REG_RBDM_N_SG_CONFIG) & 0x0000ffff);
}



uint32_t
dma_channel::getDMABusy()
{
    return ( (getPKT(RORC_REG_DMA_CTRL) >> 7) & 0x01);
}



uint64_t
dma_channel::getEBSize()
{
    uint64_t size =
        ( (uint64_t)getPKT(RORC_REG_EBDM_BUFFER_SIZE_H) << 32);

    size += (uint64_t)getPKT(RORC_REG_EBDM_BUFFER_SIZE_L);

    return size;
}



uint64_t
dma_channel::getRBSize()
{
    uint64_t size =
        ( (uint64_t)getPKT(RORC_REG_RBDM_BUFFER_SIZE_H) << 32);

    size += (uint64_t)getPKT(RORC_REG_RBDM_BUFFER_SIZE_L);

    return size;
}


/** PKT = Packetizer */
void
dma_channel::setPKT
(
    uint32_t addr,
    uint32_t data
)
{
    m_bar->set32((m_base + addr), data);
}



uint32_t
dma_channel::getPKT
(
    uint32_t addr
)
{
    return m_bar->get32(m_base+addr);
}


/** GTK clock domain */
void
dma_channel::setGTX
(
    uint32_t addr,
    uint32_t data
)
{
    m_bar->set32( m_base+(1<<RORC_DMA_CMP_SEL)+addr, data);
}



uint32_t
dma_channel::getGTX
(
    uint32_t addr
)
{
    return m_bar->get32(m_base+(1<<RORC_DMA_CMP_SEL)+addr);
}



/** configure DMA engine for the current set of buffers */
int32_t
dma_channel::configureChannel(uint32_t pcie_packet_size)
{

    /**
     * N_SG_CONFIG:
     * [15:0] : actual number of sg entries in RAM
     * [31:16]: maximum number of entries
     */
    uint32_t rbdmnsgcfg = getPKT( RORC_REG_RBDM_N_SG_CONFIG );
    uint32_t ebdmnsgcfg = getPKT( RORC_REG_EBDM_N_SG_CONFIG );

    /** check if sglist fits into FPGA buffers */
    if( ( (rbdmnsgcfg >> 16) < m_reportBuffer->getnSGEntries() ) |
        ( (ebdmnsgcfg >> 16) < m_eventBuffer->getnSGEntries() ) )
    {
        errno = -EFBIG;
        return errno;
    }

    if(pcie_packet_size & 0xf)
    {
        /** packet size must be a multiple of 4 DW / 16 bytes */
        errno = -EINVAL;
        return errno;
    }
    else if(pcie_packet_size > 1024)
    {
        errno = -ERANGE;
        return errno;
    }

    librorc_channel_config config;
    config.ebdm_n_sg_config      = m_eventBuffer->getnSGEntries();
    config.ebdm_buffer_size_low  = m_eventBuffer->getPhysicalSize() & 0xffffffff;
    config.ebdm_buffer_size_high = m_eventBuffer->getPhysicalSize() >> 32;
    config.rbdm_n_sg_config      = m_reportBuffer->getnSGEntries();
    config.rbdm_buffer_size_low  = m_reportBuffer->getPhysicalSize() & 0xffffffff;
    config.rbdm_buffer_size_high = m_reportBuffer->getPhysicalSize() >> 32;

    config.swptrs.ebdm_software_read_pointer_low =
        (m_eventBuffer->getPhysicalSize() - pcie_packet_size) & 0xffffffff;
    config.swptrs.ebdm_software_read_pointer_high =
        (m_eventBuffer->getPhysicalSize() - pcie_packet_size) >> 32;
    config.swptrs.rbdm_software_read_pointer_low =
        (m_reportBuffer->getPhysicalSize() - sizeof(struct librorc_event_descriptor) ) & 0xffffffff;
    config.swptrs.rbdm_software_read_pointer_high =
        (m_reportBuffer->getPhysicalSize() - sizeof(struct librorc_event_descriptor) ) >> 32;

    config.swptrs.dma_ctrl = (1 << 31) |      // sync software read pointers
                             (m_channel << 16); // set channel as PCIe tag

    /**
     * Set PCIe packet size
     **/
    setPciePacketSize(pcie_packet_size);

    /**
     * copy configuration struct to RORC, starting
     * at the address of the lowest register(EBDM_N_SG_CONFIG)
     */
    m_bar->memcopy( (librorc_bar_address)(m_base+RORC_REG_EBDM_N_SG_CONFIG),
                    &config, sizeof(librorc_channel_config) );

    m_pcie_packet_size = pcie_packet_size;
    m_last_ebdm_offset = m_eventBuffer->getPhysicalSize() - pcie_packet_size;
    m_last_rbdm_offset = m_reportBuffer->getPhysicalSize() - sizeof(struct librorc_event_descriptor);

    return 0;
}

/**PROTECTED:*/

    void
    dma_channel::initMembers
    (
        uint32_t  channel_number,
        uint32_t  pcie_packet_size,
        device   *dev,
        bar      *bar,
        buffer   *eventBuffer,
        buffer   *reportBuffer
    )
    {
        m_last_ebdm_offset = 0;
        m_last_rbdm_offset = 0;
        m_pcie_packet_size = 0;
        m_pcie_packet_size = pcie_packet_size;

        m_is_pattern_generator = false;
        m_is_gtx               = false;

        m_base         = (channel_number + 1) * RORC_CHANNEL_OFFSET;
        m_channel      = channel_number;
        m_dev          = dev;
        m_bar          = bar;

        m_eventBuffer  = eventBuffer;
        m_reportBuffer = reportBuffer;

        if(m_reportBuffer != NULL)
        { m_reportBuffer->clear(); }
    }



    void
    dma_channel::prepareBuffers()
    {
        if( !m_dev->DMAChannelIsImplemented(m_channel) )
        { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

        if( (m_eventBuffer!=NULL) && (m_reportBuffer!=NULL) )
        {
            cout << "preparing buffers" << endl;
            /** Prepare EventBufferDescriptorManager with scatter-gather list */
            if(prepareEB(m_eventBuffer) < 0)
            { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

            /** Prepare ReportBufferDescriptorManager with scatter-gather list */
            if(prepareRB(m_reportBuffer) < 0)
            { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }

            /** set max payload, buffer sizes, #sgEntries, ... */
            if(configureChannel(m_pcie_packet_size) < 0)
            { throw LIBRORC_DMA_CHANNEL_ERROR_CONSTRUCTOR_FAILED; }
        }
    }



    int32_t
    dma_channel::prepareEB
    (
        buffer *buf
    )
    {
        return(prepare(buf, RORC_REG_EBDM_N_SG_CONFIG));
    }



    int32_t
    dma_channel::prepareRB
    (
        buffer *buf
    )
    {
        return(prepare(buf, RORC_REG_RBDM_N_SG_CONFIG));
    }



    int32_t
    dma_channel::prepare
    (
        buffer   *buf,
        uint32_t  flag
    )
    {
        //Members
        uint32_t                 control_flag;
        uint32_t                 bdcfg;
        DMABuffer               *pda_dma_buffer;
        DMABuffer_SGNode        *sglist;
        uint64_t                 i;
        librorc_sg_entry_config  sg_entry;


        //Constructor
        control_flag   = 0;
        bdcfg          = getPKT(flag);
        pda_dma_buffer = NULL;
        sglist         = NULL;
        i              = 0;
        sg_entry;

        switch(flag)
        {
            case RORC_REG_RBDM_N_SG_CONFIG:
            { control_flag = 1; }
            break;

            case RORC_REG_EBDM_N_SG_CONFIG:
            { control_flag = 0; }
            break;

            default:
                return -1;
        }

        //method sglistFitsIntoDRAM
        if(buf->getnSGEntries() > (bdcfg >> 16) )
        {
            errno = EFBIG;
            return -EFBIG;
        }

        //method getSglistFromPDA
        pda_dma_buffer = buf->getPDABuffer();
        if(PDA_SUCCESS != DMABuffer_getSGList(pda_dma_buffer, &sglist) )
        {
            printf("SG-List fetching failed!\n");
            return -1;
        }

        //method programSglistIntoDRAM
        for(DMABuffer_SGNode *sg=sglist; sg!=NULL; sg=sg->next)
        {
            /** convert sg list into CRORC compatible format */
            sg_entry.sg_addr_low  = (uint32_t)( (uint64_t)(sg->d_pointer) & 0xffffffff);
            sg_entry.sg_addr_high = (uint32_t)( (uint64_t)(sg->d_pointer) >> 32);
            sg_entry.sg_len       = (uint32_t)(sg->length & 0xffffffff);

            sg_entry.ctrl = (1 << 31) | (control_flag << 30) | ((uint32_t)i);

            /** write librorc_dma_desc to RORC EBDM */
            m_bar->memcopy
                ( (librorc_bar_address)(m_base+RORC_REG_SGENTRY_ADDR_LOW), &sg_entry, sizeof(sg_entry) );
            i++;
        }

        //method clearTrailingDRAM
        memset(&sg_entry, 0, sizeof(sg_entry) );
        m_bar->memcopy( (librorc_bar_address)(m_base+RORC_REG_SGENTRY_ADDR_LOW),
                           &sg_entry, sizeof(sg_entry) );

        return(0);
    }



}
