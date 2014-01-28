/**
 * @file fmc_tester.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-12-19
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

#include "fmc_tester.hh"

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
