/**
 * Copyright (c) 2015, Heiko Engel <hengel@cern.ch>
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
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/
#include <librorc/ddr3.hh>
#include <librorc/bar.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME {
    ddr3::ddr3( bar *bar, uint32_t moduleId) {
        m_bar = bar;
        m_id = moduleId;
        m_offset = (m_id * 16);
    }

    ddr3::~ddr3(){}

    void ddr3::setReset(uint32_t value) {
        uint32_t ddrctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        /** clear reset bits */
        ddrctrl &= ~(1 << m_offset);
        /** set new values */
        ddrctrl |= ((value & 1) << m_offset);
        /** write back new value */
        m_bar->set32(RORC_REG_DDR3_CTRL, ddrctrl);
    }

    uint32_t ddr3::getReset() {
        uint32_t ddrctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        return ((ddrctrl>>m_offset) & 1);
    }

    uint32_t ddr3::getBitrate() {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        /** C0: bits [10:9], C1: bits [26:25] */
        switch ((ddr3ctrl >> (m_offset + 9)) & 3) {
            case 1:
                return 606;
            case 2:
                return 800;
            case 3:
                return 1066;
            default: // controller not implemented
                return 0;
        }
    }

    bool ddr3::isImplemented() {
        return (getBitrate() != 0);
    }

    uint32_t ddr3::maxModuleSize() {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        // get controller address width encoded as (32-regval)
        uint32_t addr_width = (32 - ((ddr3ctrl >> (m_offset + 11)) & 0x7));
        // number of ranks: 0 => single ranked, 1 => dual ranked
        uint32_t nranks = (((ddr3ctrl >> (14 + m_offset)) & 1) + 1);
        // highest address bit cannot be counted on single ranked modules
        if (nranks == 1) {
            addr_width -= 1;
        }
        return 8 * (1 << addr_width);
    }

    bool ddr3::initSuccessful() {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        uint16_t controller_status = ((ddr3ctrl >> m_offset) & 0xffff);
        uint16_t checkmask = (1 << LIBRORC_DDR3_RESET) | // module reset
            (1 << LIBRORC_DDR3_PHYINITDONE) |  // PHY init done
            (1 << LIBRORC_DDR3_PLLLOCK) |      // PLL lock
            (1 << LIBRORC_DDR3_RDLVL_START) |  // read levelling started
            (1 << LIBRORC_DDR3_RDLVL_DONE) |   // read levelling done
            (1 << LIBRORC_DDR3_RDLVL_ERROR) |  // read levelling error
            (1 << LIBRORC_DDR3_WRLVL_DONE) |   // write levelling done
            (1 << LIBRORC_DDR3_WRLVL_ERROR);   // write levelling error
        uint16_t expected_value = (0 << LIBRORC_DDR3_RESET) | // not in reset
            (1 << LIBRORC_DDR3_PHYINITDONE) |  // PHY init done
            (1 << LIBRORC_DDR3_PLLLOCK) |      // PLL locked
            (1 << LIBRORC_DDR3_RDLVL_START) |  // read levelling started
            (1 << LIBRORC_DDR3_RDLVL_DONE) |   // read levelling done
            (0 << LIBRORC_DDR3_RDLVL_ERROR) |  // no read levelling error
            (1 << LIBRORC_DDR3_WRLVL_DONE) |   // write levelling done
            (0 << LIBRORC_DDR3_WRLVL_ERROR);   // no write levelling error
        return ((controller_status & checkmask) == expected_value);
    }

    bool ddr3::initPhaseDone() {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        uint16_t controller_status = ((ddr3ctrl >> m_offset) & 0xffff);
        uint16_t checkmask = (1 << LIBRORC_DDR3_RESET) | // module reset
            (1 << LIBRORC_DDR3_PHYINITDONE) |  // PHY init done
            (1 << LIBRORC_DDR3_PLLLOCK) |      // PLL lock
            (1 << LIBRORC_DDR3_RDLVL_DONE) |   // read levelling done
            (1 << LIBRORC_DDR3_WRLVL_DONE);    // write levelling done
        uint16_t expected_value = (0 << LIBRORC_DDR3_RESET) | // not in reset
            (1 << LIBRORC_DDR3_PHYINITDONE) |  // PHY init done
            (1 << LIBRORC_DDR3_PLLLOCK) |      // PLL locked
            (1 << LIBRORC_DDR3_RDLVL_DONE) |   // read levelling done
            (1 << LIBRORC_DDR3_WRLVL_DONE);    // write levelling done
        return ((controller_status & checkmask) == expected_value);
    }

    uint16_t ddr3::controllerState() {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        return ((ddr3ctrl >> m_offset) & 0xffff);
    }

    uint32_t ddr3::getTgErrorFlag() {
        uint32_t ddr3ctrl = m_bar->get32(RORC_REG_DDR3_CTRL);
        return ((ddr3ctrl >> LIBRORC_DDR3_TG_ERROR) & 1);
    }

    uint32_t ddr3::getTgRdCount() {
        if (m_id==0) {
            return m_bar->get32(RORC_REG_DDR3_C0_TESTER_RDCNT);
        } else {
            return m_bar->get32(RORC_REG_DDR3_C1_TESTER_RDCNT);
        }
    }

    uint32_t ddr3::getTgWrCount() {
        if (m_id==0) {
            return m_bar->get32(RORC_REG_DDR3_C0_TESTER_WRCNT);
        } else {
            return m_bar->get32(RORC_REG_DDR3_C1_TESTER_WRCNT);
        }
    }

}
