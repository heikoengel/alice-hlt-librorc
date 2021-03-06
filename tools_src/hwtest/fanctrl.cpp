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

#define HELP_TEXT "fanctrl usage: \n\
        fanctrl -n [device] (-e [0/1]) \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -e [0,1,2]     (optional) Enable(1) / Disable(0) the Fan, \n\
                        or set to automatic control(2).\n\
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
    int32_t enable_val    =  0;
    int do_enable         =  0;
    int arg               =  0;

    while ( (arg = getopt(argc, argv, "hn:e:")) != -1 )
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
            case 'e':
                enable_val = strtol(optarg, NULL, 0);
                do_enable = 1;
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

    if ( enable_val < 0 || enable_val > 2 )
    {
        cout << "ERROR: invalid 'enable' value " << enable_val << endl;
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
        bar = new librorc::bar(dev, 1);
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
    { sm = new librorc::sysmon(bar); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar;
        delete dev;
        abort();
    }

    if ( do_enable )
    {
        if (enable_val==2)
        { sm->systemFanSetEnable(0, 1); }
        else
        { sm->systemFanSetEnable(1, enable_val); }
    }

    bool enabled = sm->systemFanIsEnabled();
    bool running = sm->systemFanIsRunning();
    bool automode = sm->systemFanIsAutoMode();

    cout << "Fan speed     : ";
    if( running )
    { cout << sm->systemFanSpeed(); }
    else
    { cout << "0"; }
    cout << " RPM" << endl;
    cout << "Fan mode      : ";
    if( automode )
    { cout << "auto"; }
    else
    { cout << "MANUAL OVERRIDE"; }
    cout << endl;

    cout << "Fan enabled   : " << enabled << endl;
    cout << "Fan Running   : " << running << endl;

    if( enabled && !running )
    { cout << "WARNING: fan is enabled but not running!" << endl; }

    delete sm;
    delete bar;
    delete dev;

    return 0;
}
