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


typedef struct
{
    uint32_t hs_div;
    uint32_t n1;
    uint64_t rfreq_int;
}clkopts;


clkopts 
refclk_getCurrentOpts 
( 
        librorc::sysmon *sm
)
{
    /** get current RFREQ, HSDIV, N1 */
    clkopts opts;

    /** addr 7: HS_DIV[2:0], N1[6:2] */
    try { 
        uint8_t value = sm->i2c_read_mem(REFCLK_CHAIN, 
                REFCLK_I2C_SLAVE, 0x07); 
        opts.hs_div = (value>>5) & 0x03;
        opts.n1 = (value&0x1f)<<2;
    }
    catch (...)
    {
        cout << "Failed to read HS_DIV/N1" << endl;
        abort();
    }

    /** addr 8: N1[1:0], RFREQ[37:32] */
    try { 
        uint8_t value = sm->i2c_read_mem(REFCLK_CHAIN, 
                REFCLK_I2C_SLAVE, 0x08); 
        opts.n1 += (value>>6)&0x03;
        opts.rfreq_int = (uint64_t(value & 0x3f)<<((uint64_t)32));
    }
    catch (...)
    {
        cout << "Failed to read N1/RFREQ" << endl;
        abort();
    }

    /** addr 9...12: RFREQ[31:0] */
    for(uint8_t i=9; i<=12; i++)
    {
        try 
        { 
            uint8_t value = sm->i2c_read_mem(REFCLK_CHAIN, 
                    REFCLK_I2C_SLAVE, i); 
            opts.rfreq_int |= (uint64_t(value) << ((12-i)*8));
        }
        catch (...)
        {
            cout << "Failed to read RFREQ" << endl;
            abort();
        }
    }
    return opts;
}


clkopts
refclk_getNewOpts
(
        librorc::sysmon *sm,
        double new_freq
        )
{
    // N1:     1,2,...,128
    // HS_DIV: 4,5,6,7,9,11
    // find the lowest value of N1 with the highest value of HS_DIV
    // to get fDCO in the range of 4.85...5.67 GHz
    int32_t vco_found = 0;
    clkopts opts;

    for ( int n=1; n<128; n++ )
    {
        for ( int h=11; h>3; h-- )
        {
            if ( h==8 || h==10 )
            {
                continue;
            }
            double fDCO_new =  new_freq * h * n;
            if ( fDCO_new >= 4850.0 && fDCO_new <= 5670.0 && 
                    vco_found==0 )
            {
                vco_found = 1;
                opts.hs_div = h;
                opts.n1 = n;
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
    double new_freq = 0.0;

    /** parse command line arguments **/
    while ( (arg = getopt(argc, argv, "d:c:s:a:w:")) != -1 )
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
    #ifdef SIM
        librorc::bar *bar = new librorc::sim_bar(dev, 1);
    #else
        librorc::bar *bar = new librorc::rorc_bar(dev, 1);
    #endif

    if ( bar->init() == -1 )
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

        
    clkopts opts = refclk_getCurrentOpts( sm );

    cout << "HS_DIV    : " << opts.hs_div << endl;
    cout << "N1        : " << opts.n1 << endl;
    cout << "RFREQ_INT : 0x" << hex << opts.rfreq_int << dec << endl;
    cout << "RFREQ     : " << ((double)(opts.rfreq_int))/((double)(1<<28)) 
         << " MHz" << endl;


    if ( do_write )
    {

        // TODO: 
        // * set to initial conditions first
        //   * set RECALL
        //   * wait for RECALL to be cleared by device
        // * get current config
        // * set FreezeM

        clkopts new_opts = refclk_getNewOpts(sm, new_freq);

        cout << endl << "New Oscillator Values" << endl;
        cout << "HS_DIV    : " << new_opts.hs_div << endl;
        cout << "N1        : " << new_opts.n1 << endl;

        //TODO: get new RFREQ

        //TODO: 
        // * write values to device
        // * unfreeze and set NewFreq bit
        // * wait for NewFreq to be cleared by device

    } 

    delete sm;
    delete bar;
    delete dev;

    return 0;
}
