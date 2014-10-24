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

#include <librorc/refclk.hh>
#include <librorc/sysmon.hh>
#include <math.h>

namespace LIBRARY_NAME
{

    //------------- public --------------

    refclk::refclk
    (
        sysmon *parent_sysmon
    )
    {
        m_sysmon = NULL;
        if ( !parent_sysmon )
        {
            throw LIBRORC_REFCLK_ERROR_CONSTRUCTOR_FAILED;
        }
        m_sysmon = parent_sysmon;
    }

    refclk::~refclk()
    {
        m_sysmon = NULL;
    }

    refclkopts
    refclk::getCurrentOpts
    (
        double fout
    )
    {
        // get current RFREQ, HSDIV, N1
        refclkopts opts;
        uint8_t value;

        // addr 7: HS_DIV[2:0], N1[6:2]
        value = refclk_read(13);
        opts.hs_div = hsdiv_reg2val((value>>5) & 0x07);
        uint32_t n1 = ((uint32_t)(value&0x1f))<<2;

        value = refclk_read(14);
        n1 += (value>>6)&0x03;
        opts.n1 = n1_reg2val(n1);
        opts.rfreq_int = (uint64_t(value & 0x3f)<<((uint64_t)32));

        // addr 15...18: RFREQ[31:0]
        for(uint8_t i=0; i<=3; i++)
        {
            value = refclk_read(i+15);
            opts.rfreq_int |= (uint64_t(value) << ((3-i)*8));
        }

        opts.rfreq_float = hex2float(opts.rfreq_int);

        // fOUT is known -> get accurate fDCO
        if ( fout!=0 )
        {
            opts.fdco = fout * opts.n1 * opts.hs_div;
            opts.fxtal = opts.fdco/opts.rfreq_float;
        }
        else
        // fOUT is unknown
        // -> use default fXTAL to get approximate fDCO
        {
            opts.fdco = 114.285 * opts.rfreq_float;
            opts.fxtal = 114.285;
        }

        return opts;
    }

    refclkopts
    refclk::calcNewOpts
    (
        double new_freq,
        double fxtal
    )
    {
        // find the lowest value of N1 with the highest value of HS_DIV
        // to get fDCO in the range of 4.85...5.67 GHz
        int32_t vco_found = 0;
        double fDCO_new;
        double lastfDCO = 5670.0;
        refclkopts opts;

        opts.fxtal = fxtal;

        for ( int n=1; n<=128; n++ )
        {
            // N1 can be 1 or any even number up to 128
            if ( n!=1 && (n & 0x1) )
            {
                continue;
            }

            for ( int h=11; h>3; h-- )
            {
                // valid values for HSDIV are 4, 5, 6, 7, 9 or 11
                if ( h==8 || h==10 )
                {
                    continue;
                }
                fDCO_new =  new_freq * h * n;
                if ( fDCO_new >= 4850.0 && fDCO_new <= 5670.0 )
                {
                    vco_found = 1;
                    // find lowest possible fDCO for this configuration
                    if (fDCO_new<lastfDCO)
                    {
                        opts.hs_div = h;
                        opts.n1 =  n;
                        opts.fdco = fDCO_new;
                        opts.rfreq_float = fDCO_new / fxtal;
                        opts.rfreq_int = float2hex(opts.rfreq_float);
                        lastfDCO = fDCO_new;
                    }
                }

            }
        }

        if ( vco_found==0 )
        {
            throw LIBRORC_REFCLK_ERROR_INVALID_PARAMETER;
        }

        return opts;
    }

    float
    refclk::getFout
    (
        refclkopts opts
    )
    {
        return opts.fxtal * opts.rfreq_float / opts.hs_div / opts.n1;
    }

    void
    refclk::writeOptsToDevice
    (
        refclkopts opts
    )
    {
        // Freeze oscillator
        setFreezeDCO();

        // write new osciallator values
        uint8_t value = (hsdiv_val2reg(opts.hs_div)<<5) |
            (n1_val2reg(opts.n1)>>2);
        refclk_write(13, value);

        value = ((n1_val2reg(opts.n1)&0x03)<<6) |
            (opts.rfreq_int>>32);
        refclk_write(14, value);

        // addr 15...18: RFREQ[31:0]
        for(uint8_t i=0; i<=3; i++)
        {
            value = ((opts.rfreq_int>>((3-i)*8)) & 0xff);
            refclk_write(15+i, value);
        }

        // release DCO Freeze
        releaseDCO();

        // wait for NewFreq to be deasserted
        waitForClearance(M_NEWFREQ);
    }

    void
    refclk::reset()
    {
        /** Recall initial conditions */
        setRFMCtrl( M_RECALL );
        /** Wait for RECALL to complete */
        waitForClearance( M_RECALL );
        /** fOUT is now the default freqency */
    }


    //------------- protected --------------

    void
    refclk::releaseDCO()
    {
        // get current FREEZE_DCO settings
        uint8_t freeze_val = refclk_read(137);

        // clear FREEZE_DCO bit
        freeze_val &= 0xef;

        m_sysmon->i2c_write_mem_dual(LIBRORC_REFCLK_I2C_CHAIN,
                LIBRORC_REFCLK_I2C_SLAVE, 137, freeze_val, 135, M_NEWFREQ);
        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                "i2c_write_mem_dual(%02x->%02x, %02x->%02x)\n",
                137, freeze_val, 135, M_NEWFREQ);
    }

    void
    refclk::setRFMCtrl
    (
        uint8_t flag
    )
    {
        refclk_write(135, flag);
    }

    void
    refclk::setFreezeDCO()
    {
        uint8_t val = refclk_read(137);
        val |= FREEZE_DCO;
        refclk_write(137, val);
    }

    void
    refclk::waitForClearance
    (
        uint8_t flag
    )
    {
        // wait for flag to be cleared by device
        uint8_t reg135 = flag;
        while ( reg135 & flag )
        {
            reg135 = refclk_read(135);
        }
    }

    double
    refclk::hex2float
    (
        uint64_t value
    )
    {
        return double( value/((double)(1<<28)) );
    }

    uint64_t
    refclk::float2hex
    (
        double value
    )
    {
        return uint64_t(trunc(value*(1<<28)));
    }

    int32_t
    refclk::hsdiv_reg2val
    (
        uint32_t hs_reg
    )
    {
        return hs_reg + 4;
    }

    int32_t
    refclk::hsdiv_val2reg
    (
        uint32_t hs_val
    )
    {
        return hs_val - 4;
    }

    int32_t
    refclk::n1_reg2val
    (
        uint32_t n1_reg
    )
    {
        return n1_reg + 1;
    }

    int32_t
    refclk::n1_val2reg
    (
        uint32_t n1_val
    )
    {
        return n1_val - 1;
    }


    uint8_t
    refclk::refclk_read
    (
        uint8_t addr
    )
    {
        uint8_t value;
        value = m_sysmon->i2c_read_mem(LIBRORC_REFCLK_I2C_CHAIN,
                LIBRORC_REFCLK_I2C_SLAVE, addr);
        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "refclk_read(%02d)=%02x\n",
                addr, value);
        return value;
    }

    void
    refclk::refclk_write
    (
        uint8_t addr,
        uint8_t value
    )
    {
        m_sysmon->i2c_write_mem(LIBRORC_REFCLK_I2C_CHAIN,
                LIBRORC_REFCLK_I2C_SLAVE, addr, value);
        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "refclk_write(%02d, %02x)\n",
                addr, value);
    }
}
