/**
 * @file fmc_loopback_tester.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-12-03
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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "librorc.h"

#define HELP_TEXT "fmc_loopback_tester usage: \n\
        fmc_loopback_tester -n [0...255] \n\
        where -n specifies the target device. \n\
"

#define T_FMC_SETUP 100

using namespace std;


void
reset_fmc_tester
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
drive_bit
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
check_bit
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
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int arg;

    while ( (arg = getopt(argc, argv, "hn:")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                {
                    cout << HELP_TEXT;
                    return 0;
                }
                break;
            case 'n':
                {
                    device_number = atoi(optarg);
                }
                break;default:
                {
                    cout << "Unknown parameter (" << arg << ")!" << endl;
                    cout << HELP_TEXT;
                    return -1;
                }
                break;
        } //switch
    } //while

    if ( device_number < 0 || device_number > 255 )
    {
        cout << "No or invalid device selected, using default device 0"
            << endl;
        device_number = 0;
    }

    /** Instantiate device **/
    librorc::device *dev = NULL;
    try
    {
        dev = new librorc::device(device_number);
    }
    catch(...)
    {
        cout << "Failed to intialize device " << device_number
            << endl;
        return -1;
    }

    /** Instantiate a new bar */
    librorc::bar *bar = NULL;
    try
    {
#ifdef MODELSIM
        bar = new librorc::sim_bar(dev, 1);
#else
        bar = new librorc::rorc_bar(dev, 1);
#endif
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    /** Check if firmware is 'hwtest' */
    if ( bar->get32(RORC_REG_TYPE_CHANNELS)>>16 != RORC_CFG_PROJECT_hwtest )
    {
        cout << "Current firmware is no hwtest firmware! "
            << "Won't do anything..." << endl;
        delete bar;
        delete dev;
        return 0;
    }

    reset_fmc_tester(bar);
    usleep(T_FMC_SETUP);

    /** FPGA internal pullups should be active, readback
     * should return '1' for each bit */
    uint32_t fmc_low = bar->get32(RORC_REG_FMC_CTRL_LOW);
    uint32_t fmc_high = bar->get32(RORC_REG_FMC_CTRL_HIGH);
    if ( fmc_low != 0xffffffff || ((fmc_high>>8)&0x3) != 0x03)
    {
        cout << "WARNING: unexpected reset state 0x"
             << hex << ((fmc_high>>8)&0x3) << "_" << fmc_low
             << " - expected 0x3_ffffffff" << dec << endl;
    }

    /** check if a FMC addon board is detected:
     * fmc_high[30] = fmc_prsnt_n -> 0 when present
     **/
    if ( fmc_high & (1<<30) )
    {
        cout << "WARNING: no FMC addon board detected!" << endl;
    }

    int result = 0;

    for ( int i=0; i<34; i++ )
    {
        /** drive bit high, all others low */
        drive_bit(bar, i, 1);
        usleep(T_FMC_SETUP);
        result |= check_bit(bar, i, 1);
        
        /** drive bit low, all others high */
        drive_bit(bar, i, 0);
        usleep(T_FMC_SETUP);
        result |= check_bit(bar, i, 0);
    }

    reset_fmc_tester(bar);

    delete bar;
    delete dev;

    if ( !result )
    {
        cout << "FMC LA test done." << endl;
        return 0;
    } else {
        cout << "FMC LA test failed." << endl;
        return -1;
    }
}
