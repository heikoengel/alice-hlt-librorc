/**
 * @file lvdsctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-08-08
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
#include <getopt.h>
#include <stdint.h>

#include "librorc.h"

#define HELP_TEXT "ledctrl usage: \n\
        ledctrl [paramters] \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -r [0,1]        Set LVDS Tester Reset \n\
        -e [0,1]        Set LVDS Buffer Enable \n\
        -s              Print current LVDS Tester Status\n\
"

using namespace std;


string 
test_bit
(
    uint32_t status,
    uint32_t bit
)
{
    if ( status & (1<<bit) ) { return "UP"; }
    else { return "DOWN"; }
}


int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;

    int set_reset         =  0;
    int set_enable        =  0;
    int print_status      =  0;

    uint32_t reset_val    =  0;
    uint32_t enable_val   =  0;

    int arg;

    while ( (arg = getopt(argc, argv, "hn:e:r:s")) != -1 )
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
                set_enable = 1;
                break;
            case 'r':
                reset_val = strtol(optarg, NULL, 0);
                set_reset = 1;
                break;
            case 's':
                print_status = 1;
                break;
            default:
                cout << HELP_TEXT;
                return -1;
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
        dev = new librorc::device(device_number, LIBRORC_ES_BOTH);
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
        abort();
    }

    /** get current settings
     * RORC_REG_LVDS_CTRL:
     * 31: Reset
     *  2: LVDS Link1 up
     *  1: LVDS Link0 up
     *  0: LVDS Buffer enable
     *  */
    uint32_t lvdsctrl = bar->get32(RORC_REG_LVDS_CTRL);
    if ( set_reset )
    {
        /** clear current reset setting */
        lvdsctrl &= ~(1<<31);
        /** set new values */
        lvdsctrl |= (reset_val<<31);
    }

    if ( set_enable )
    {
        /** Clear current Enable setting */
        lvdsctrl &= ~(1<<0);
        /* set new values */
        lvdsctrl |= (enable_val<<0);
    }

    if ( set_enable || set_reset )
    {
        /** write back new values */
        bar->set32(RORC_REG_LVDS_CTRL, lvdsctrl);
    }

    if ( print_status )
    {
        cout << "LVDS Tester Reset : " << (lvdsctrl>>31) << endl;
        cout << "LVDS Tester Link0 : " << test_bit(lvdsctrl, 1) << endl;
        cout << "LVDS Tester Link1 : " << test_bit(lvdsctrl, 2) << endl;
        cout << "LVDS Buffer Enable: " << (lvdsctrl & 1) << endl;
    }

    delete bar;
    delete dev;

    return 0;
}
