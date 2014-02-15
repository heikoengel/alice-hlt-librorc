/**
 * @file refclk.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-27
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
 * */

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
    refclk::getNewOpts
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

    void 
    refclk::setOpts
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


    //------------- protected --------------
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
