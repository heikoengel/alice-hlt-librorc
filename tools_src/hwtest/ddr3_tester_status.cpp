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
#include <unistd.h>

#include "librorc.h"

#define HELP_TEXT "ddr3_tester_status usage: \n\
        ddr3_tester_status [parameters] \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -r [0/1]        Reset DDR3 \n\
        -h              Show this help text \n\
"

using namespace std;


int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int do_reset = 0;
    int32_t reset_val = 0;
    int arg;

    while ( (arg = getopt(argc, argv, "hn:r:")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                cout << HELP_TEXT;
                return 0;
                break;
            
            case 'n':
                device_number = atoi(optarg);
                break;

            case 'r':
                reset_val = atoi(optarg);
                do_reset = 1;
                break;

            default:
                cout << HELP_TEXT;
                return -1;
        }
    }

    /** make sure all required parameters are provided and valid **/
    if (  device_number > 255 || device_number < 0 )
    {
        cout << "ERROR: no or invalid device ID selected: " 
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
        bar = new librorc::bar(dev, 1);
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    librorc::sysmon *sm = NULL;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        cout << "ERROR: failed to Sysmon." << endl;
        delete bar;
        delete dev;
        abort();
    }

    if ( !sm->ddr3Bitrate(0) && !sm->ddr3Bitrate(1) )
    {
        cout << "No DDR3 Controller available in Firmware! "
             << "Won't do anything..." << endl;
        delete sm;
        delete bar;
        delete dev;
        return 0;
    }

    uint32_t ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);

    if( do_reset )
    {

#ifdef MODELSIM
        /** wait for phy_init_done */
        while ( !(bar->get32(RORC_REG_DDR3_CTRL) & (1<<2)) )
        { usleep(100); }
#endif

        ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);
        /** clear reset bits */
        ddrctrl &= ~(1<<0 | 1<<16);
        /** set new values */
        ddrctrl |= (reset_val & 1);
        ddrctrl |= (reset_val & 1)<<16;
        /** write back new value */
        bar->set32(RORC_REG_DDR3_CTRL, ddrctrl);
    }
    else
    {

        if( sm->ddr3Bitrate(0) )
        {
            cout << "C0 Reset: " << (ddrctrl&1) << endl;
            cout << "C0 PhyInitDone: " << ((ddrctrl>>1)&1) << endl;
            cout << "C0 PLL Lock: " << ((ddrctrl>>2)&1) << endl;
            cout << "C0 Read Levelling Started: " << ((ddrctrl>>3)&1) << endl;
            cout << "C0 Read Levelling Done: " << ((ddrctrl>>4)&1) << endl;
            cout << "C0 Read Levelling Error: " << ((ddrctrl>>5)&1) << endl;
            cout << "C0 Write Levelling Started: " << ((ddrctrl>>6)&1) << endl;
            cout << "C0 Write Levelling Done: " << ((ddrctrl>>7)&1) << endl;
            cout << "C0 Write Levelling Error: " << ((ddrctrl>>8)&1) << endl;
            if( sm->firmwareIsHltHardwareTest() )
            {
                uint32_t rdcnt0 = bar->get32(RORC_REG_DDR3_C0_TESTER_RDCNT);
                uint32_t wrcnt0 = bar->get32(RORC_REG_DDR3_C0_TESTER_WRCNT);

                cout << "C0 Read Count: " << rdcnt0 << endl;
                cout << "C0 Write Count: " << wrcnt0 << endl;
                cout << "C0 TG Error: " << ((ddrctrl>>15)&1) << endl;
            }
            cout << endl;
        }

        if( sm->ddr3Bitrate(1) )
        {
            cout << "C1 Reset: " << ((ddrctrl>>16)&1) << endl;
            cout << "C1 PhyInitDone: " << ((ddrctrl>>17)&1) << endl;
            cout << "C1 PLL Lock: " << ((ddrctrl>>18)&1) << endl;
            cout << "C1 Read Levelling Started: " << ((ddrctrl>>19)&1) << endl;
            cout << "C1 Read Levelling Done: " << ((ddrctrl>>20)&1) << endl;
            cout << "C1 Read Levelling Error: " << ((ddrctrl>>21)&1) << endl;
            cout << "C1 Write Levelling Started: " << ((ddrctrl>>22)&1) << endl;
            cout << "C1 Write Levelling Done: " << ((ddrctrl>>23)&1) << endl;
            cout << "C1 Write Levelling Error: " << ((ddrctrl>>24)&1) << endl;
            if( sm->firmwareIsHltHardwareTest() )
            {
                uint32_t rdcnt1 = bar->get32(RORC_REG_DDR3_C1_TESTER_RDCNT);
                uint32_t wrcnt1 = bar->get32(RORC_REG_DDR3_C1_TESTER_WRCNT);

                cout << "C1 Read Count: " << rdcnt1 << endl;
                cout << "C1 Write Count: " << wrcnt1 << endl;
                cout << "C1 TG Error: " << ((ddrctrl>>31)&1) << endl;
            }
        }
    }

    delete sm;
    delete bar;
    delete dev;

    return 0;
}
