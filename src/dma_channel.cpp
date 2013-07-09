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

#include <pda.h>

using namespace std;

/** struct holding both read pointers and the
 *  DMA engine configuration register contents
 **/
struct
__attribute__((__packed__))
librorc_buffer_software_pointers
{
    /** EBDM read pointer low **/
    uint32_t ebdm_software_read_pointer_low;

    /** EBDM read pointer high **/
    uint32_t ebdm_software_read_pointer_high;

    /** RBDM read pointer low **/
    uint32_t rbdm_software_read_pointer_low;

    /** RBDM read pointer high **/
    uint32_t rbdm_software_read_pointer_high;

    /** DMA control register **/
    uint32_t dma_ctrl;
};



struct
__attribute__((__packed__))
librorc_channel_config
{
    /** EBDM number of sg entries **/
    uint32_t ebdm_n_sg_config;

    /** EBDM buffer size low (in bytes) **/
    uint32_t ebdm_buffer_size_low;

    /** EBDM buffer size high (in bytes) **/
    uint32_t ebdm_buffer_size_high;

    /** RBDM number of sg entries **/
    uint32_t rbdm_n_sg_config;

    /** RBDM buffer size low (in bytes) **/
    uint32_t rbdm_buffer_size_low;

    /** RBDM buffer size high (in bytes) **/
    uint32_t rbdm_buffer_size_high;

    /** struct for read pointers nad control register **/
    struct librorc_buffer_software_pointers swptrs;

};



/** struct t_sg_entry_cfg **/
struct
__attribute__((__packed__))
t_sg_entry_cfg
{
    /** lower part of sg address **/
    uint32_t sg_addr_low;

    /** higher part of sg address **/
    uint32_t sg_addr_high;

    /** total length of sg entry in bytes **/
    uint32_t sg_len;

    /** BDM control register: [31]:we, [30]:sel, [29:0]BRAM addr **/
    uint32_t ctrl;
};



/** extern error number **/
extern int errno;



namespace librorc
{

/**
 * Constructor
 * */
dma_channel::dma_channel()
{
    m_bar              = NULL;
    m_base             = 0;
    m_last_ebdm_offset = 0;
    m_last_rbdm_offset = 0;
    m_pcie_packet_size = 0;
}



/**
 * Desctructor
 * */
dma_channel::~dma_channel()
{
}



/**
 * Initialize Channel:
 * bind to specific BAR, set offset address for register access
 * */

void
dma_channel::init
(
    bar      *dma_bar,
    uint32_t  channel_number
)
{
    m_base    = (channel_number + 1) * RORC_CHANNEL_OFFSET;
    m_channel = channel_number;

    m_bar = dma_bar;
}


/**
 * Fill DescriptorRAM with scatter-gather
 * entries of DMA buffer
 * */

int32_t
dma_channel::_prepare
(
    buffer   *buf,
    uint32_t  flag
)
{
    assert(m_bar!=NULL);

    /** Some generic initialization */
    uint32_t control_flag = 0;
    switch(flag)
    {
        case RORC_REG_RBDM_N_SG_CONFIG:
        {
            control_flag = 1;
        }
        break;

        case RORC_REG_EBDM_N_SG_CONFIG:
        {
            control_flag = 0;
        }
        break;

        default:
            cout << "Invalid flag!" << endl;
            return -1;
    }

    /**
     * get maximum number of sg-entries supported by the firmware
     * N_SG_CONFIG:
     * [15:0] : current number of sg entries in RAM
     * [31:16]: maximum number of entries
     **/
    uint32_t bdcfg = getPKT(flag);

    /** check if buffers SGList fits into DRAM */
    if(buf->getnSGEntries() > (bdcfg >> 16) )
    {
        errno = EFBIG;
        return -EFBIG;
    }

    /** retrieve scatter gather list */
    DMABuffer        *pda_dma_buffer = buf->getPDABuffer();
    DMABuffer_SGNode *sglist         = NULL;
    if(PDA_SUCCESS != DMABuffer_getSGList(pda_dma_buffer, &sglist) )
    {
        printf("SG-List fetching failed!\n");
        return -1;
    }


    /** fetch all sg-entries from sglist */
    uint64_t i = 0;
    struct t_sg_entry_cfg  sg_entry;
    for(DMABuffer_SGNode *sg=sglist; sg!=NULL; sg=sg->next)
    {
        /** convert sg list into CRORC compatible format */
        sg_entry.sg_addr_low  = (uint32_t)( (uint64_t)(sg->d_pointer) & 0xffffffff);
        sg_entry.sg_addr_high = (uint32_t)( (uint64_t)(sg->d_pointer) >> 32);
        sg_entry.sg_len       = (uint32_t)(sg->length & 0xffffffff);

        sg_entry.ctrl = (1 << 31) | (control_flag << 30) | ((uint32_t)i);

        /** write librorc_dma_desc to RORC EBDM */
        m_bar->memcpy_bar( (m_base+RORC_REG_SGENTRY_ADDR_LOW),
                           &sg_entry, sizeof(sg_entry) );
        i++;
    }

    /** clear following BD entry (required!) */
    memset(&sg_entry, 0, sizeof(sg_entry) );
    m_bar->memcpy_bar( (m_base+RORC_REG_SGENTRY_ADDR_LOW),
                       &sg_entry, sizeof(sg_entry) );

    return(0);
}



/**
 * prepareRB
 * Fill ReportBufferDescriptorRAM with scatter-gather
 * entries of DMA buffer
 * */

int32_t
dma_channel::prepareEB
(
    buffer *buf
)
{
    return(_prepare(buf, RORC_REG_EBDM_N_SG_CONFIG));
}



int32_t
dma_channel::prepareRB
(
    buffer *buf
)
{
    return(_prepare(buf, RORC_REG_RBDM_N_SG_CONFIG));
}


/** REWORKED __________________________________________________________*/



/**
 * configure DMA engine for the current
 * set of buffers
 * */

int32_t
dma_channel::configureChannel
(
    buffer   *ebuf,
    buffer   *rbuf,
    uint32_t  pcie_packet_size
)
{

    /**
     * N_SG_CONFIG:
     * [15:0] : actual number of sg entries in RAM
     * [31:16]: maximum number of entries
     */
    uint32_t rbdmnsgcfg = getPKT( RORC_REG_RBDM_N_SG_CONFIG );
    uint32_t ebdmnsgcfg = getPKT( RORC_REG_EBDM_N_SG_CONFIG );

    /** check if sglist fits into FPGA buffers */
    if( ( (rbdmnsgcfg >> 16) < rbuf->getnSGEntries() ) |
        ( (ebdmnsgcfg >> 16) < ebuf->getnSGEntries() ) )
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

    //TODO refactor this into a sepparate method
    struct librorc_channel_config config;
    config.ebdm_n_sg_config      = ebuf->getnSGEntries();
    config.ebdm_buffer_size_low  = ebuf->getPhysicalSize() & 0xffffffff;
    config.ebdm_buffer_size_high = ebuf->getPhysicalSize() >> 32;
    config.rbdm_n_sg_config      = rbuf->getnSGEntries();
    config.rbdm_buffer_size_low  = rbuf->getPhysicalSize() & 0xffffffff;
    config.rbdm_buffer_size_high = rbuf->getPhysicalSize() >> 32;

    config.swptrs.ebdm_software_read_pointer_low =
        (ebuf->getPhysicalSize() - pcie_packet_size) & 0xffffffff;
    config.swptrs.ebdm_software_read_pointer_high =
        (ebuf->getPhysicalSize() - pcie_packet_size) >> 32;
    config.swptrs.rbdm_software_read_pointer_low =
        (rbuf->getPhysicalSize() - sizeof(struct librorc_event_descriptor) ) &
        0xffffffff;
    config.swptrs.rbdm_software_read_pointer_high =
        (rbuf->getPhysicalSize() - sizeof(struct librorc_event_descriptor) ) >> 32;

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
    m_bar->memcpy_bar(m_base + RORC_REG_EBDM_N_SG_CONFIG, &config,
                      sizeof(struct librorc_channel_config) );

    m_pcie_packet_size = pcie_packet_size;
    m_last_ebdm_offset = ebuf->getPhysicalSize() - pcie_packet_size;
    m_last_rbdm_offset = rbuf->getPhysicalSize() - sizeof(struct librorc_event_descriptor);

    return 0;
}



void
dma_channel::setEnableEB
(
    int32_t enable
)
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
    struct librorc_buffer_software_pointers offsets;

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

    m_bar->memcpy_bar(m_base + RORC_REG_EBDM_SW_READ_POINTER_L,
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

    m_bar->memcpy_bar(m_base + RORC_REG_EBDM_SW_READ_POINTER_L,
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

    m_bar->memcpy_bar( (m_base+RORC_REG_RBDM_SW_READ_POINTER_L),
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
    m_bar->set((m_base + addr), data);
}



uint32_t
dma_channel::getPKT
(
    uint32_t addr
)
{
    return m_bar->get(m_base+addr);
}


/** GTK clock domain */
void
dma_channel::setGTX
(
    uint32_t addr,
    uint32_t data
)
{
    m_bar->set( m_base+(1<<RORC_DMA_CMP_SEL)+addr, data);
}



uint32_t
dma_channel::getGTX
(
    uint32_t addr
)
{
    return m_bar->get(m_base+(1<<RORC_DMA_CMP_SEL)+addr);
}

}