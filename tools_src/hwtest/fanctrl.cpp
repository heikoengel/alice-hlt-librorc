/**
 * @file fanctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-08-28
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

#define HELP_TEXT "fanctrl usage: \n\
        fanctrl -n [device] (-e [0/1]) \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -e [0,1]        (optional) Enable/Disable the Fan \n\
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
                enable_val = strtol(optarg, NULL, 0) & 0x3f;
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
        sm->systemFanSetEnable(enable_val);
    }

    cout << "Fan speed     : " << sm->systemFanSpeed() << " RPM" << endl;
    if( sm->systemFanIsEnabled() == false)
    {
        cout << "WARNING: fan seems to be disabled!" << endl;
    }

    if( sm->systemFanIsRunning() == false)
    {
        cout << "WARNING: fan seems to be stopped!" << endl;
    }

    delete sm;
    delete bar;
    delete dev;

    return 0;
}
