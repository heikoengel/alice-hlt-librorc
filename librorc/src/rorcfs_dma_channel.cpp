/**
 * @file rorcfs_dma_channel.cpp
 * @author Heiko Engel <hengel@cern.ch>
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <pda.h>

#include "rorcfs_bar.hh"
#include "rorcfs_buffer.hh"
#include "rorcfs_dma_channel.hh"
#include <librorc_registers.h>

using namespace std;

/** struct holding both read pointers and the
 *  DMA engine configuration register contents
 **/
struct
__attribute__((__packed__))
rorcfs_buffer_software_pointers
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

/** struct rorcfs_channel_config **/
struct
__attribute__((__packed__))
rorcfs_channel_config
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
    struct rorcfs_buffer_software_pointers swptrs;

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

/**
 * Constructor
 * */
rorcfs_dma_channel::rorcfs_dma_channel()
{
    m_bar        = NULL;
    m_base       = 0;
    m_MaxPayload = 0;
}



/**
 * Desctructor
 * */
rorcfs_dma_channel::~rorcfs_dma_channel()
{
}



/**
 * Initialize Channel:
 * bind to specific BAR, set offset address for register access
 * */

void
rorcfs_dma_channel::init
(
    librorc_bar *dma_bar,
    uint32_t     channel_number
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
rorcfs_dma_channel::_prepare
(
    rorcfs_buffer *buf,
    uint32_t       flag
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

        /** write rorcfs_dma_desc to RORC EBDM */
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
rorcfs_dma_channel::prepareEB
(
    rorcfs_buffer *buf
)
{
    return(_prepare(buf, RORC_REG_EBDM_N_SG_CONFIG));
}



int32_t
rorcfs_dma_channel::prepareRB
(
    rorcfs_buffer *buf
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
rorcfs_dma_channel::configureChannel
(
    struct rorcfs_buffer *ebuf,
    struct rorcfs_buffer *rbuf,
    uint32_t              max_payload,
    uint32_t              max_rd_req
)
{

    /**
     * MAX_PAYLOAD has to be provided as #DWs
     *  -> divide size by 4
     */
    uint32_t mp_size = max_payload >> 2;
    uint32_t mr_size = max_rd_req >> 2;

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

    if(max_payload & 0x3)
    {
        /** max_payload must be a multiple of 4 DW */
        errno = -EINVAL;
        return errno;
    }
    else if(max_payload > 1024)
    {
        errno = -ERANGE;
        return errno;
    }

    if(max_rd_req & 0x3)
    {
        /** max_rd_req must be a multiple of 4 DW */
        errno = -EINVAL;
        return errno;
    }
    else if(max_rd_req > 1024)
    {
        errno = -ERANGE;
        return errno;
    }

    //TODO refactor this into a sepparate method
    struct rorcfs_channel_config config;
    config.ebdm_n_sg_config      = ebuf->getnSGEntries();
    config.ebdm_buffer_size_low  = ebuf->getPhysicalSize() & 0xffffffff;
    config.ebdm_buffer_size_high = ebuf->getPhysicalSize() >> 32;
    config.rbdm_n_sg_config      = rbuf->getnSGEntries();
    config.rbdm_buffer_size_low  = rbuf->getPhysicalSize() & 0xffffffff;
    config.rbdm_buffer_size_high = rbuf->getPhysicalSize() >> 32;

    config.swptrs.ebdm_software_read_pointer_low =
        (ebuf->getPhysicalSize() - max_payload) & 0xffffffff;
    config.swptrs.ebdm_software_read_pointer_high =
        (ebuf->getPhysicalSize() - max_payload) >> 32;
    config.swptrs.rbdm_software_read_pointer_low =
        (rbuf->getPhysicalSize() - sizeof(struct rorcfs_event_descriptor) ) &
        0xffffffff;
    config.swptrs.rbdm_software_read_pointer_high =
        (rbuf->getPhysicalSize() - sizeof(struct rorcfs_event_descriptor) ) >> 32;

    /** set new MAX_PAYLOAD and MAX_READ_REQUEST size */
    m_bar->set( (m_base+RORC_REG_DMA_PKT_SIZE),
                ((mr_size << 16)+mp_size) );

    config.swptrs.dma_ctrl = (1 << 31) |      // sync software read pointers
                             (m_channel << 16); // set PCIe tag

    /**
     * copy configuration struct to RORC, starting
     * at the address of the lowest register(EBDM_N_SG_CONFIG)
     */
    m_bar->memcpy_bar(m_base + RORC_REG_EBDM_N_SG_CONFIG, &config,
                      sizeof(struct rorcfs_channel_config) );
    m_MaxPayload = max_payload;

    return 0;
}



void
rorcfs_dma_channel::setEnableEB
(
    int32_t enable
)
{
    unsigned int bdcfg = getPKT( RORC_REG_DMA_CTRL );
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
rorcfs_dma_channel::getEnableEB()
{
    return (getPKT( RORC_REG_DMA_CTRL ) >> 2 ) & 0x01;
}



void
rorcfs_dma_channel::setEnableRB
(
    int32_t enable
)
{
    unsigned int bdcfg = getPKT( RORC_REG_DMA_CTRL );
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
rorcfs_dma_channel::getEnableRB()
{
    return (getPKT( RORC_REG_DMA_CTRL ) >> 3 ) & 0x01;
}



void
rorcfs_dma_channel::setDMAConfig
(
    uint32_t config
)
{
    setPKT(RORC_REG_DMA_CTRL, config);
}



uint32_t
rorcfs_dma_channel::getDMAConfig()
{
    return getPKT(RORC_REG_DMA_CTRL);
}



void
rorcfs_dma_channel::setMaxPayload
(
    int32_t size
)
{
    _setMaxPayload( size );
}



void
rorcfs_dma_channel::setMaxPayload()
{
    _setMaxPayload( MAX_PAYLOAD );
}



void
rorcfs_dma_channel::_setMaxPayload
(
    int32_t size
)
{
    /**
     * MAX_PAYLOAD is located in the higher WORD of
     * RORC_REG_DMA_CTRL: [25:16], 10 bits wide
     */
    assert( m_bar!=NULL );

    /**
     * assure valid values for "size":
     * size <= systems MAX_PAYLOAD
     */
    assert( size <= MAX_PAYLOAD );

    uint32_t status
        = getPKT(RORC_REG_DMA_PKT_SIZE);

    /**
     * MAX_PAYLOAD has to be provided as #DWs
     * -> divide size by 4
     */
    uint32_t mp_size
        = size >> 2;

    /** clear current MAX_PAYLOAD setting
     *
     *  clear bits 15:0 */
    status &= 0xffff0000;
    /** set new MAX_PAYLOAD size */
    status |= mp_size;

    /** write DMA_CTRL */
    setPKT(RORC_REG_DMA_PKT_SIZE, status);
    m_MaxPayload = size;
}



uint32_t
rorcfs_dma_channel::getMaxPayload()
{
    assert(m_bar!=NULL);

    /** RORC_REG_DMA_CTRL = {max_rd_req, max_payload} */
    unsigned int status = getPKT(RORC_REG_DMA_PKT_SIZE);

    status &= 0xffff;
    status = status << 2;

    return status;
}


/** Rework : remove sepparate methods*/
void
rorcfs_dma_channel::setOffsets
(
    uint64_t eboffset,
    uint64_t rboffset
)
{
    assert(m_bar!=NULL);
    struct rorcfs_buffer_software_pointers offsets;

    offsets.ebdm_software_read_pointer_low =
        (uint32_t)(eboffset & 0xffffffff);

    offsets.ebdm_software_read_pointer_high =
        (uint32_t)(eboffset >> 32 & 0xffffffff);

    offsets.rbdm_software_read_pointer_low =
        (uint32_t)(rboffset & 0xffffffff);

    offsets.rbdm_software_read_pointer_high =
        (uint32_t)(rboffset >> 32 & 0xffffffff);

    offsets.dma_ctrl = (1 << 31) | /** sync pointers     */
                       (1 << 2)  | /** enable EB         */
                       (1 << 3)  | /** enable RB         */
                       (1 << 0);   /** enable DMA engine */

    m_bar->memcpy_bar(m_base + RORC_REG_EBDM_SW_READ_POINTER_L,
                      &offsets, sizeof(offsets) );
}



void
rorcfs_dma_channel::setEBOffset
(
    uint64_t offset
)
{
    assert(m_bar!=NULL);
    unsigned int status;

    m_bar->memcpy_bar(m_base + RORC_REG_EBDM_SW_READ_POINTER_L,
                      &offset, sizeof(offset) );
    status = getPKT(RORC_REG_DMA_CTRL);
    setPKT(RORC_REG_DMA_CTRL, status | (1 << 31) );
}



uint64_t
rorcfs_dma_channel::getEBOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_EBDM_SW_READ_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_EBDM_SW_READ_POINTER_L);

    return offset;
}



uint64_t
rorcfs_dma_channel::getEBDMAOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_EBDM_FPGA_WRITE_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_EBDM_FPGA_WRITE_POINTER_L);

    return offset;
}

void
rorcfs_dma_channel::setRBOffset
(
    uint64_t offset
)
{
    assert(m_bar!=NULL);
    uint32_t status;

    m_bar->memcpy_bar( (m_base+RORC_REG_RBDM_SW_READ_POINTER_L),
                        &offset, sizeof(offset) );

    status = getPKT(RORC_REG_DMA_CTRL);

    setPKT(RORC_REG_DMA_CTRL, status | (1 << 31) );
}



uint64_t
rorcfs_dma_channel::getRBOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_RBDM_SW_READ_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_RBDM_SW_READ_POINTER_L);

    return offset;
}



uint64_t
rorcfs_dma_channel::getRBDMAOffset()
{
    uint64_t offset =
        ( (uint64_t)getPKT(RORC_REG_RBDM_FPGA_WRITE_POINTER_H) << 32);

    offset += (uint64_t)getPKT(RORC_REG_RBDM_FPGA_WRITE_POINTER_L);

    return offset;
}



uint32_t
rorcfs_dma_channel::getEBDMnSGEntries()
{
    return (getPKT(RORC_REG_EBDM_N_SG_CONFIG) & 0x0000ffff);
}



uint32_t
rorcfs_dma_channel::getRBDMnSGEntries()
{
    return (getPKT(RORC_REG_RBDM_N_SG_CONFIG) & 0x0000ffff);
}



uint32_t
rorcfs_dma_channel::getDMABusy()
{
    return ( (getPKT(RORC_REG_DMA_CTRL) >> 7) & 0x01);
}



uint64_t
rorcfs_dma_channel::getEBSize()
{
    uint64_t size =
        ( (uint64_t)getPKT(RORC_REG_EBDM_BUFFER_SIZE_H) << 32);

    size += (uint64_t)getPKT(RORC_REG_EBDM_BUFFER_SIZE_L);

    return size;
}



uint64_t
rorcfs_dma_channel::getRBSize()
{
    uint64_t size =
        ( (uint64_t)getPKT(RORC_REG_RBDM_BUFFER_SIZE_H) << 32);

    size += (uint64_t)getPKT(RORC_REG_RBDM_BUFFER_SIZE_L);

    return size;
}


/** PKT = Packetizer */
void
rorcfs_dma_channel::setPKT
(
    uint32_t addr,
    uint32_t data
)
{
    m_bar->set((m_base + addr), data);
}



uint32_t
rorcfs_dma_channel::getPKT
(
    uint32_t addr
)
{
    return m_bar->get(m_base+addr);
}


/** GTK clock domain */
void
rorcfs_dma_channel::setGTX
(
    uint32_t addr,
    uint32_t data
)
{
    m_bar->set( m_base+(1<<RORC_DMA_CMP_SEL)+addr, data);
}



uint32_t
rorcfs_dma_channel::getGTX
(
    uint32_t addr
)
{
    return m_bar->get(m_base+(1<<RORC_DMA_CMP_SEL)+addr);
}
