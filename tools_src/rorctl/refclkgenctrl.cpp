/**
 * @file refclkgenctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-19
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
#include <math.h>
#include <pda.h>

#include "librorc.h"

using namespace std;

#define HELP_TEXT "Configure/Read Si570 Reference Clock \n\
usage: refclkgenctrl [parameters] \n\
parameters: \n\
        -h              Print this help screen \n\
        -d [0..255]     Target device ID \n\
        -w [freq]       Target frequency in MHz \n\
Examples: \n\
get current configuration from device 0 \n\
        refclkgenctrl -d 0 \n\
set new frequency to 125.00 MHz for device 0 \n\
        refclkgenctrl -d 0 -s 125.00 \n\
"

#define REFCLK_I2C_SLAVE 0x55
#define REFCLK_CHAIN 4
#define REFCLK_DEFAULT_FOUT 212.500

/** Register 135 Pin Mapping */
#define M_RECALL (1<<0)
#define M_FREEZE (1<<5)
#define M_NEWFREQ (1<<6)
/** Register 137 Pin Mapping */
#define FREEZE_DCO (1<<4)


typedef struct
{
    uint32_t hs_div;
    uint32_t n1;
    uint64_t rfreq_int;
    double rfreq_float;
    double fdco;
}clkopts;


double
refclk_hex2float
(
    uint64_t value
)
{
    return double( value/((double)(1<<28)) );
}

uint64_t
refclk_float2hex
(
    double value
)
{
    return uint64_t(trunc(value*(1<<28)));
}

int32_t
refclk_hsdiv_reg2val
(
    uint32_t hs_reg
)
{
    return hs_reg + 4;
}


int32_t
refclk_hsdiv_val2reg
(
    uint32_t hs_val
)
{
    return hs_val - 4;
}

int32_t
refclk_n1_reg2val
(
    uint32_t n1_reg
)
{
    return n1_reg + 1;
}


int32_t
refclk_n1_val2reg
(
    uint32_t n1_val
)
{
    return n1_val - 1;
}


uint8_t
refclk_read
(
   librorc::sysmon *sm,
   uint8_t addr
)
{
    uint8_t value;
    try
    {
        value = sm->i2c_read_mem(REFCLK_CHAIN,
                REFCLK_I2C_SLAVE, addr);
    }
    catch (...)
    {
        cout << "Failed to read from refclk addr 0x"
             << hex << (int)addr << dec << endl;
        abort();
    }
    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "refclk_read(%02x)=%02x\n",
            addr, value);
    return value;
}


void refclk_write
(
   librorc::sysmon *sm,
   uint8_t addr,
   uint8_t value
)
{
    try
    {
        sm->i2c_write_mem(REFCLK_CHAIN,
                REFCLK_I2C_SLAVE, addr, value);
    }
    catch (...)
    {
        cout << "Failed to write 0x" << setw(2) << hex << (int)value
             << " to refclk addr 0x" << (int)addr << dec << endl;
        abort();
    }
    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "refclk_write(%02x, %02x)\n",
            addr, value);
}

void
refclk_release
(
   librorc::sysmon *sm,
   uint8_t freeze_val,
   uint8_t rfmval
)
{
    try
    {
        sm->i2c_write_mem_dual(REFCLK_CHAIN,
                REFCLK_I2C_SLAVE, 137, freeze_val, 135, rfmval);
    }
    catch (...)
    {
        cout << "i2c_write_mem_dual failed" << endl;
        abort();
    }
    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, 
            "i2c_write_mem_dual(%02x->%02x, %02x->%02x)\n",
            137, freeze_val, 135, rfmval);
}









clkopts
refclk_getCurrentOpts
(
        librorc::sysmon *sm
)
{
    /** get current RFREQ, HSDIV, N1 */
    clkopts opts;
    uint8_t value;

    /** addr 7: HS_DIV[2:0], N1[6:2] */
    value = refclk_read(sm, 0x07);
    opts.hs_div = (value>>5) & 0x07;
    opts.n1 = ((uint32_t)(value&0x1f))<<2;

    value = refclk_read(sm, 0x08);
    opts.n1 += (value>>6)&0x03;
    opts.rfreq_int = (uint64_t(value & 0x3f)<<((uint64_t)32));

    /** addr 9...12: RFREQ[31:0] */
    for(uint8_t i=9; i<=12; i++)
    {
        value = refclk_read(sm, i);
        opts.rfreq_int |= (uint64_t(value) << ((12-i)*8));
    }

    opts.fdco = REFCLK_DEFAULT_FOUT * refclk_n1_reg2val(opts.n1) *
        refclk_hsdiv_reg2val(opts.hs_div);
    opts.rfreq_float = refclk_hex2float(opts.rfreq_int);

    return opts;
}


clkopts
refclk_getNewOpts
(
    librorc::sysmon *sm,
    double new_freq,
    double fxtal
)
{
    // N1:     1,2,...,128
    // HS_DIV: 4,5,6,7,9,11
    // find the lowest value of N1 with the highest value of HS_DIV
    // to get fDCO in the range of 4.85...5.67 GHz
    int32_t vco_found = 0;
    double fDCO_new;
    clkopts opts;

    for ( int n=1; n<128; n++ )
    {
        for ( int h=11; h>3; h-- )
        {
            if ( h==8 || h==10 )
            {
                continue;
            }
            fDCO_new =  new_freq * h * n;
            if ( fDCO_new >= 4850.0 && fDCO_new <= 5670.0 &&
                    vco_found==0 )
            {
                vco_found = 1;
                opts.hs_div = refclk_hsdiv_val2reg(h);
                opts.n1 =  refclk_n1_val2reg(n);
                opts.fdco = fDCO_new;
                opts.rfreq_float = fDCO_new / fxtal;
                opts.rfreq_int = refclk_float2hex(opts.rfreq_float);
                break;
            }

        }

        /** break outer loop if values are found */
        if ( vco_found==1 )
        {
            break;
        }
    }

    if ( vco_found==0 )
    {
        cout << "Could not get HSDIV/N1 for given frequency." << endl;
        abort();
    }


    return opts;
}



void
refclk_setRFMCtrl
(
    librorc::sysmon *sm,
    uint8_t flag
)
{
    refclk_write(sm, 135, flag);
}



void
refclk_setFreezeDCO
(
    librorc::sysmon *sm,
    uint8_t flag
)
{
    uint8_t val = refclk_read(sm, 137);
    val &= 0xef;
    val |= flag;
    refclk_write(sm, 137, val);
}



void
refclk_waitForClearance
(
    librorc::sysmon *sm,
    uint8_t flag
)
{
    /** wait for flag to be cleared by device */
    uint8_t reg135 = flag;
    while ( reg135 & flag )
    {
        reg135 = refclk_read(sm, 135);
    }
}


void refclk_setOpts
(
    librorc::sysmon *sm,
    clkopts opts
)
{
    /** Freeze oscillator */
    refclk_setFreezeDCO(sm, FREEZE_DCO);

    /** write new osciallator values */
    uint8_t value = (opts.hs_div<<5) | (opts.n1>>2);
    refclk_write(sm, 0x07, value);

    value = ((opts.n1&0x03)<<6)|(opts.rfreq_int>>32);
    refclk_write(sm, 0x08, value);

    /** addr 9...12: RFREQ[31:0] */
    for(uint8_t i=9; i<=12; i++)
    {
        value = ((opts.rfreq_int>>((12-i)*8)) & 0xff);
        refclk_write(sm, i, value);
    }

    /** release DCO Freeze */
    refclk_setFreezeDCO(sm, 0);

    /** release M_FREEZE, set NewFreq */
    refclk_setRFMCtrl(sm, M_NEWFREQ);

    /** wait for NewFreq to be deasserted */
    refclk_waitForClearance(sm, M_NEWFREQ);
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
    int do_write = 0;
    int do_reset = 0;
    int do_freeze = 0;
    double new_freq = 0.0;

    /** parse command line arguments **/
    while ( (arg = getopt(argc, argv, "d:c:s:a:w:rf")) != -1 )
    {
        switch (arg)
        {
            case 'd':
                device_number = strtol(optarg, NULL, 0);
                break;
            case 'w':
                new_freq = atof(optarg);
                do_write = 1;
                break;
            case 's':
                break;
            case 'f':
                do_freeze = 1;
                break;
            case 'r':
                do_reset = 1;
                break;
            case 'h':
                cout << HELP_TEXT;
                return 0;
        }
    }

    /** make sure all required parameters are provided and valid **/
    if ( device_number == -1 )
    {
        cout << "ERROR: device ID was not provided." << endl;
        cout << HELP_TEXT;
        return -1;
    }
    else if ( device_number > 255 || device_number < 0 )
    {
        cout << "ERROR: invalid device ID selected: "
             << device_number << endl;
        cout << HELP_TEXT;
        return -1;
    }


    /** Instantiate device **/
    librorc::device *dev = NULL;
    try{
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
    #ifdef SIM
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

    /** Instantiate a new sysmon */
    librorc::sysmon *sm;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar;
        delete dev;
        abort();
    }


    if ( do_write || do_reset )
    {
        /** Recall initial conditions */
        refclk_setRFMCtrl( sm, M_RECALL );
        refclk_waitForClearance( sm, M_RECALL );
    }

    if (do_freeze)
    {
        uint8_t val = refclk_read(sm, 137);
        val |= FREEZE_DCO;
        /** Freeze oscillator */
        refclk_write(sm, 137, val);
        val &= 0xef;

        refclk_release(sm, val, M_NEWFREQ);
    }


    clkopts opts = refclk_getCurrentOpts( sm );
    double fxtal = REFCLK_DEFAULT_FOUT * refclk_hsdiv_reg2val(opts.hs_div) *
        refclk_n1_reg2val(opts.n1) / opts.rfreq_float;

    cout << "HS_DIV    : " << refclk_hsdiv_reg2val(opts.hs_div) << endl;
    cout << "N1        : " << refclk_n1_reg2val(opts.n1) << endl;
    cout << "RFREQ_INT : 0x" << hex << opts.rfreq_int << dec << endl;
    cout << "RFREQ     : " << opts.rfreq_float << " MHz" << endl;
    cout << "F_DCO     : " << opts.fdco << " MHz" << endl;
    cout << "F_XTAL    : " << fxtal << " MHz" << endl;

    double cur_freq = 114.285 * opts.rfreq_float /
        (refclk_hsdiv_reg2val(opts.hs_div) * refclk_n1_reg2val(opts.n1));
    cout << "cur FREQ  : " << cur_freq << " MHz (approx.)" << endl;

    uint8_t RFMC = refclk_read(sm, 135);
    cout << "RFMC      : 0x" << hex << (int)RFMC << endl;
    uint8_t FR = refclk_read(sm, 137);
    cout << "DCOfreeze : 0x" << hex << (int)FR << endl;



    if ( do_write )
    {

        clkopts new_opts = refclk_getNewOpts(sm, new_freq, fxtal);

        cout << endl << "New Oscillator Values" << endl;
        cout << "HS_DIV    : " << refclk_hsdiv_reg2val(new_opts.hs_div) << endl;
        cout << "N1        : " << refclk_n1_reg2val(new_opts.n1) << endl;
        cout << "RFREQ_INT : 0x" << hex << new_opts.rfreq_int << dec << endl;
        cout << "RFREQ     : " << new_opts.rfreq_float << " MHz" << endl;
        cout << "F_DCO     : " << new_opts.fdco << " MHz" << endl;
        cout << "F_OUT     : " << fxtal * new_opts.rfreq_float /
            (refclk_hsdiv_reg2val(new_opts.hs_div) *
             refclk_n1_reg2val(new_opts.n1))
             << endl;

        refclk_setOpts(sm, new_opts);

    }


    delete sm;
    delete bar;
    delete dev;

    return 0;
}
