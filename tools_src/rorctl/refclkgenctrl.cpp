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
        -n [0..255]     Target device ID \n\
        -w [freq]       Target frequency in MHz \n\
        -r              Reset ClkGen to default Frequency \n\
Examples: \n\
get current configuration from device 0 \n\
        refclkgenctrl -n 0 \n\
set new frequency to 125.00 MHz for device 0 \n\
        refclkgenctrl -n 0 -w 125.00 \n\
"


void
print_refclk_settings
(
    librorc::refclkopts opts
)
{
    cout << "HS_DIV    : " << dec << opts.hs_div << endl;
    cout << "N1        : " << dec << opts.n1 << endl;
    cout << "RFREQ_INT : 0x" << hex << opts.rfreq_int << dec << endl;
    cout << "RFREQ     : " << opts.rfreq_float << " MHz" << endl;
    cout << "F_DCO     : " << opts.fdco << " MHz" << endl;
    cout << "F_XTAL    : " << opts.fxtal << " MHz" << endl;

    double cur_freq = opts.fxtal * opts.rfreq_float /
        (opts.hs_div * opts.n1);
    cout << "cur FREQ  : " << cur_freq << " MHz" << endl;
}



int
main
(
    int argc,
    char *argv[]
)
{
    double fout = 0;
    int32_t device_number = -1;
    int arg;
    int do_write = 0;
    int do_reset = 0;
    double new_freq = 0.0;

    /** parse command line arguments **/
    while ( (arg = getopt(argc, argv, "n:w:rh")) != -1 )
    {
        switch (arg)
        {
            case 'n':
                device_number = strtol(optarg, NULL, 0);
                break;
            case 'w':
                new_freq = atof(optarg);
                do_write = 1;
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

    librorc::refclk *rc;
    try
    {
        rc = new librorc::refclk(sm);
    }
    catch(...)
    {
        cout << "Refclk init failed!" << endl;
        delete sm;
        delete bar;
        delete dev;
        abort();
    }


    if ( do_reset || do_write )
    {
        /** Recall initial conditions */
        rc->setRFMCtrl( M_RECALL );

        /** Wait for RECALL to complete */
        rc->waitForClearance( M_RECALL );

        /** fOUT is now the default freqency */
        fout = LIBRORC_REFCLK_DEFAULT_FOUT;
    }

    /** get current configuration */
    librorc::refclkopts opts = rc->getCurrentOpts( fout );

    /** Print configuration */
    cout << endl << "Current Oscillator Values" << endl;
    print_refclk_settings( opts );

    if ( do_write )
    {
        /** get new values for desired frequency */
        librorc::refclkopts new_opts = rc->getNewOpts(new_freq, opts.fxtal);

        /** Print new configuration */
        cout << endl << "New Oscillator Values" << endl;
        print_refclk_settings( new_opts );

        /** write new configuration to device */
        rc->setOpts(new_opts);
    }


    delete rc;
    delete sm;
    delete bar;
    delete dev;

    return 0;
}
