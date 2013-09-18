/**
 * @file
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>, Heiko Engel <hengel@cern.ch>
 * @date 2011-09-18
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
#include <librorc/link.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>

#include <pda.h>

using namespace std;

namespace librorc
{
    link::link
    (
        bar      *bar,
        uint32_t  link_number
    )
    {
        m_bar         = bar;
        m_base        = (link_number + 1) * RORC_CHANNEL_OFFSET;
        m_link_number = link_number;
    };



    void
    link::setGTX
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32( m_base+(1<<RORC_DMA_CMP_SEL)+addr, data);
    }



    uint32_t
    link::getGTX(uint32_t addr)
    {
        return m_bar->get32(m_base+(1<<RORC_DMA_CMP_SEL)+addr);
    }



    void
    link::setPacketizer
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_bar->set32((m_base + addr), data);
    }



    uint32_t
    link::packetizer
    (
        uint32_t addr
    )
    {
        return m_bar->get32(m_base+addr);
    }



    void
    link::printDiuState()
    {
        uint32_t status = getGTX(RORC_REG_DDL_CTRL);

        printf("\nDIU_IF: ");

        (status & 1)       ? printf("DIU_ON ") : printf("DIU_OFF ");
        ((status>>1) & 1)  ? printf("FC_ON ")  : printf("FC_OFF ");
        ((status>>4) & 1)  ? 0                 : printf("LF ");
        ((status>>5) & 1)  ? 0                 : printf("LD ");
        ((status>>30) & 1) ? 0                 : printf("BSY ");

        /** PG disabled or enabled */
        ((status>>8) & 1)  ? printf("PG_ON")   : printf("PG_OFF");
        ((status>>8) & 1)  ? 0                 : printf("CTSTW: %08x ", getGTX(RORC_REG_DDL_CTSTW));
        ((status>>8) & 1)  ? 0                 : printf("DEADTIME: %08x ", getGTX(RORC_REG_DDL_DEADTIME));
        ((status>>8) & 1)  ? 0                 : printf("EC: %08x ", getGTX(RORC_REG_DDL_EC));
    }



    void
    link::printDMAState()
    {
        printf("\nPKT:\n");
        printf("#Events: 0x%08x; ", packetizer(RORC_REG_DMA_N_EVENTS_PROCESSED));
        printf("#Stall: 0x%08x; ", packetizer(RORC_REG_DMA_STALL_CNT));

        uint32_t dma_ctrl = packetizer(RORC_REG_DMA_CTRL);

        printf
        (
            "PKT_EN:%d; FIFO_RST:%d; EOE_IN_FIFO:%d; FIFO_EMPTY:%d; "
            "FIFO_PEMPTY:%d; BUSY:%d; EBDM_EN:%d, RBDM_EN:%d\n",
            (dma_ctrl)&1,
            (dma_ctrl>>1)&1,
            (dma_ctrl>>4)&1,
            (dma_ctrl>>5)&1,
            (dma_ctrl>>6)&1,
            (dma_ctrl>>7)&1,
            (dma_ctrl>>2)&1,
            (dma_ctrl>>3)&1
        );

        printf("EBDM:\n");
        printf
        (
            "EBDM rdptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_EBDM_SW_READ_POINTER_L),
            packetizer(RORC_REG_EBDM_SW_READ_POINTER_H)
        );

        printf
        (
            "EBDM wrptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_EBDM_FPGA_WRITE_POINTER_L),
            packetizer(RORC_REG_EBDM_FPGA_WRITE_POINTER_H)
        );

        printf("\n");
        printf("RBDM:\n");
        printf
        (
            "RBDM rdptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_RBDM_SW_READ_POINTER_L),
            packetizer(RORC_REG_RBDM_SW_READ_POINTER_H)
        );

        printf
        (
            "RBDM wrptr: 0x%08x_%08x; ",
            packetizer(RORC_REG_RBDM_FPGA_WRITE_POINTER_L),
            packetizer(RORC_REG_RBDM_FPGA_WRITE_POINTER_H)
        );
    }

}
