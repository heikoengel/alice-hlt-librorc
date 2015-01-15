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

#include <unistd.h>
#include "fmc_tester.hh"

using namespace std;

void
fmcResetTester
(
    librorc::bar *bar
)
{
    /** high:
     * [31]: data_t - 1->input, 0->output
     * [1:0]: data_out[33:32]
     * mid:
     * [31:0]: data_out[31:0]
     * */
    bar->set32(RORC_REG_FMC_CTRL_HIGH, (1<<31));
    bar->set32(RORC_REG_FMC_CTRL_MID, 0);
    //RORC_REG_FMC_CTRL_LOW is read-only
}


/**
 * drive fmc_cfg.data_out[bit] to 'level', all other bits to 'not level'
 * */
void
fmcDriveBit
(
    librorc::bar *bar,
    int bit,
    int level
)
{
    uint32_t midval;
    uint32_t highval;

    /** initialize register values to 'not level' */
    if ( level )
    {
        midval = 0;
        highval = 0;
    } else {
        midval = 0xffffffff;
        highval = 0x00000003;
    }

    /** set output bit to 'level' */
    if ( bit<32 )
    {
        if (level)
        {
            midval = (1<<bit);
        } else {
            midval = ~(1<<bit);
        }
    }
    else
    {
        if (level)
        {
            highval = (1<<(bit-32));
        } else {
            highval = ~(1<<(bit-32));
        }
    }

    /** highval[31] is buffer type: 0->output, 1->input */
    highval &= ~(1<<31);

    bar->set32(RORC_REG_FMC_CTRL_MID, midval);
    bar->set32(RORC_REG_FMC_CTRL_HIGH, highval);
}



int
fmcCheckBit
(
    librorc::bar *bar,
    int bit,
    int level
)
{
    uint32_t highval = bar->get32(RORC_REG_FMC_CTRL_HIGH);
    uint32_t midval = bar->get32(RORC_REG_FMC_CTRL_MID);
    uint32_t lowval = bar->get32(RORC_REG_FMC_CTRL_LOW);
    uint64_t outputval = ( (uint64_t)((highval) & 0x3)<<32 ) + midval;
    uint64_t inputval = ( (uint64_t)((highval>>8) & 0x3)<<32 ) + lowval;

    uint64_t exp_val;
    if ( level )
    {
        exp_val = ((uint64_t)1<<bit);
    } else {
        exp_val = ( ~((uint64_t)1<<bit) ) & (uint64_t)0x3ffffffff;
    }

    int error = 0;
    
    if (outputval != exp_val)
    {
        cout << "ERROR: check_bit failed for output bit " << bit
            << " level " << level << " - expected 0x" << hex
            << exp_val << ", received 0x"
            << outputval << dec << endl;
        error |= 1;
    }
    
    if (inputval != exp_val)
    {
        cout << "ERROR: check_bit failed for input bit " << bit
            << " level " << level << " - expected 0x" << hex
            << exp_val << ", received 0x"
            << inputval << dec << endl;
        error |= 1;
    }
    return error;
}


int
checkFmc
(
    librorc::bar *bar,
    librorc:: sysmon *sm,
    int verbose
)
{
    int result = 0;
    fmcResetTester(bar);
    usleep(T_FMC_SETUP);

    /** FPGA internal pullups should be active, readback
     * should return '1' for each bit */
    uint32_t fmc_low = bar->get32(RORC_REG_FMC_CTRL_LOW);
    uint32_t fmc_high = bar->get32(RORC_REG_FMC_CTRL_HIGH);
    if ( fmc_low != 0xffffffff || ((fmc_high>>8)&0x3) != 0x03)
    {
        result = 1;
        cout << "WARNING: unexpected reset state 0x"
             << hex << ((fmc_high>>8)&0x3) << "_" << fmc_low
             << " - expected 0x3_ffffffff" << dec << endl;
    }

    for ( int i=0; i<34; i++ )
    {
        /** drive bit high, all others low */
        fmcDriveBit(bar, i, 1);
        usleep(T_FMC_SETUP);
        result |= fmcCheckBit(bar, i, 1);
        
        /** drive bit low, all others high */
        fmcDriveBit(bar, i, 0);
        usleep(T_FMC_SETUP);
        result |= fmcCheckBit(bar, i, 0);
    }

    fmcResetTester(bar);

    /** read from i2c temperature sensor */
    uint32_t data_read = 0;
    try
    {
        data_read = sm->i2c_read_mem( 5, 0x4a, 0x00);
    }
    catch (...)
    {
        cout << "ERROR: Failed to read from FMC I2C temperature sensor"
             << endl;
        result = 1;
    }

    /** ADC-value ist the temperature as signed 8 bit value */
    int temp = (int8_t)(data_read);
    if ( temp < FMC_I2C_TEMP_MIN || temp > FMC_I2C_TEMP_MAX )
    {
        cout << "ERROR: FMC temperature sensor reading out of bounds:  "
             << dec << temp << " degC." << endl;
        result = 1;
    }
    else if (verbose)
    {
        cout << "INFO: FMC temperature sensor reports " << dec << temp
             << " degC." << endl;
    }

    return result;
}
