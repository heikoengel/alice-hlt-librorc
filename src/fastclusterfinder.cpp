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

#include <librorc/fastclusterfinder.hh>

namespace LIBRARY_NAME
{
    fastclusterfinder::fastclusterfinder( link *link )
    {
        m_link = link;
    }

    fastclusterfinder::~fastclusterfinder()
    {
        m_link = NULL;
    }

    /****************************************************
     * FastClusterFinder Basic Controls
     ***************************************************/
    void
    fastclusterfinder::setState
    (
        uint32_t reset,
        uint32_t enable
    )
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        fcfctrl &= ~( (1<<0) | (1<<15) );
        fcfctrl |= ( ((reset&1)<<15) | (enable&1) );
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
    }

    bool
    fastclusterfinder::isEnabled()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl & 1) == 1);
    }

    bool
    fastclusterfinder::isInReset()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl & 0x00008000) == 0x00008000);
    }


    /****************************************************
     * FastClusterFinder Configuration/Status
     ***************************************************/
    void
    fastclusterfinder::setSinglePadSuppression
    (
        uint32_t spSupprValue
    )
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        fcfctrl &= ~(1<<2);
        fcfctrl |= ((spSupprValue&1)<<2);
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
    }

    uint32_t
    fastclusterfinder::singlePadSuppression()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl>>2)&1);
    }


    void
    fastclusterfinder::setBypassMerger
    (
        uint32_t bypassValue
    )
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        fcfctrl &= ~(1<<3);
        fcfctrl |= ((bypassValue&1)<<3);
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
    }

    uint32_t
    fastclusterfinder::bypassMerger()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl>>3)&1);
    }

    void
    fastclusterfinder::setDeconvPad
    (
        uint32_t deconvValue
    )
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        fcfctrl &= ~(1<<4);
        fcfctrl |= ((deconvValue&1)<<4);
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
    }

    uint32_t
    fastclusterfinder::deconvPad()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl>>4)&1);
    }

    void
    fastclusterfinder::setSingleSeqLimit
    (
        uint8_t singe_seq_limit
    )
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        limits &= ~(0xff<<16);
        limits |= ((singe_seq_limit&0xff)<<16);
        m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
    }

    uint8_t
    fastclusterfinder::singleSeqLimit()
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        return ((limits>>16) & 0xff);
    }

    void
    fastclusterfinder::setClusterLowerLimit
    (
        uint16_t cluster_low_limit
    )
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        limits &= ~(0x0000ffff);
        limits |= (cluster_low_limit&0xffff);
        m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
    }

    uint16_t
    fastclusterfinder::clusterLowerLimit()
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        return (limits & 0xffff);
    }

    void
    fastclusterfinder::setMergerDistance
    (
        uint8_t match_distance
    )
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        limits &= ~(0x0f<<24);
        limits |= ((match_distance&0x0f)<<24);
        m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
    }

    uint8_t
    fastclusterfinder::mergerDistance()
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        return (limits>>24 & 0x0f);
    }

    void
    fastclusterfinder::setMergerAlgorithm
    (
        uint32_t mode
    )
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        fcfctrl &= ~(1<<5);
        fcfctrl |= ((mode&1)<<5);
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
    }

    uint32_t
    fastclusterfinder::mergerAlgorithm()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl>>5)&1);
    }

    void
    fastclusterfinder::setChargeTolerance
    (
        uint8_t charge_tolerance
    )
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        limits &= ~(0x0f<<28);
        limits |= ((charge_tolerance&0x0f)<<28);
        m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
    }

    uint8_t
    fastclusterfinder::chargeTolerance()
    {
        uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
        return (limits>>28 & 0x0f);
    }

    uint8_t
    fastclusterfinder::errorMask()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        return ((fcfctrl>>6) & 0x7f);
    }

    void
    fastclusterfinder::clearErrors()
    {
        uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
        fcfctrl |= (1<<1);
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
        fcfctrl &= ~(1<<1);
        m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
    }


    /****************************************************
     * Mapping RAM access
     ***************************************************/

    /** see header file for 'data' bit mapping */
    void
    fastclusterfinder::writeMappingRamEntry
    (
        uint32_t addr,
        uint32_t data
    )
    {
        m_link->setDdlReg(RORC_REG_FCF_RAM_DATA, data);
        m_link->setDdlReg(RORC_REG_FCF_RAM_CTRL, (addr | (1<<31)));
    }

    uint32_t
    fastclusterfinder::readMappingRamEntry
    (
        uint32_t addr
    )
    {
        m_link->setDdlReg(RORC_REG_FCF_RAM_CTRL, addr);
        return m_link->ddlReg(RORC_REG_FCF_RAM_DATA);
    }


    /****************************************************
     * temporary functions to fill mapping RAM
     * TODO: parameters from framework instead of text file
     ***************************************************/
    uint32_t
    fastclusterfinder::hexstringToUint32
    (
        std::string line
    )
    {
        uint32_t hexval;
        std::stringstream ss;
        ss << std::hex << line;
        ss >> hexval;
        return hexval;
    }



    void
    fastclusterfinder::loadMappingRam
    (
        const char *fname
    )
    {
        std::ifstream memfile(fname);
        if ( !memfile.is_open() )
        {
            std::cout << "Failed to open mapping file" << std::endl;
            abort();
        }

        std::string line;
        uint32_t i = 0;

        while ( getline(memfile, line) )
        {
            if ( i>4095 )
            {
                DEBUG_PRINTF(PDADEBUG_ERROR, "Mapping file has more " \
                        "than 4096 entries - skipping remaining lines");
                break;
            }

            uint32_t hexval = hexstringToUint32(line);
            /** TODO: this only works for RCU1 */
            uint32_t branch = ((i>>11) & 1); // Set this to 0 for RCU2
            hexval |= (branch<<29);
            writeMappingRamEntry(i, hexval);
            i++;
        }

        if ( i<4096 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Mapping file has less " \
                    "than 4096 entries - filling remaining lines");
            while( i<4096 )
            {
                writeMappingRamEntry(i, 0);
                i++;
            }
        }
    }
}
