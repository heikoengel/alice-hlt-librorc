/**
 * @file qsfpctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-08-21
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

#define HELP_TEXT "qsfpctrl usage: \n\
qsfpctrl [parameters] \n\
Parameters: \n\
        -h              Print this help \n\
        -n [0...255]    Target device \n\
        -q [0...2]      QSFP Select \n\
        -r [0/1]        Set Reset \n\
"

string
check_bit_yn
(
    uint32_t value,
    uint32_t bit,
    uint32_t polarity
)
{
    if ( polarity )
    {
        if ( value & (1<<bit) ) { return "YES"; }
        else { return "NO"; }
    }
    else
    {
        if ( value & (1<<bit) ) { return "NO"; }
        else { return "YES"; }
    }
}





int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int32_t qsfp_number = -1;
    uint32_t reset_val;

    int do_reset = 0;
    int arg;

    /** parse command line arguments */
    while ( (arg = getopt(argc, argv, "hn:q:r:")) != -1 )
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
            case 'q':
                qsfp_number = atoi(optarg);
                break;
            case 'r':
                reset_val = atoi(optarg);
                do_reset = 1;
                break;
            default:
                cout << "Unknown parameter (" << arg << ")!" << endl;
                cout << HELP_TEXT;
                return -1;
                break;
        } //switch
    } //while

    if ( device_number < 0 || device_number > 255 )
    {
        cout << "No or invalid device selected: " << device_number << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( qsfp_number<0 || qsfp_number>2 )
    {
        cout << "No or invalid QSFP selected: " << qsfp_number << endl;
        cout << HELP_TEXT;
        abort();
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

    uint32_t qsfpctrl = bar->get32(RORC_REG_QSFP_CTRL);

    if ( do_reset )
    {
        /** QSFP Reset is active low */

        /** clear current reset setting */
        qsfpctrl &= ~(1<<(3+(8*qsfp_number)));
        if ( !reset_val )
        {
            /** Set bit to disable reset */
            qsfpctrl |= (1<<(3+(8*qsfp_number)));
        }

        /** write back new value */
        bar->set32(RORC_REG_QSFP_CTRL, qsfpctrl);
    }

    cout << "CTRL: 0x" << hex << setw(8) << setfill('0')
         << qsfpctrl << endl;

    cout << "Module Present: "
        << check_bit_yn(qsfpctrl, 2+(8*qsfp_number), 0)
        << endl;

    cout << "Reset         : "
        << check_bit_yn(qsfpctrl, 3+(8*qsfp_number), 0)
        << endl;

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

    if( sm->qsfpIsPresent(qsfp_number) && !do_reset )
    {
        try
        {
            cout << "Vendor Name   : " << *(sm->qsfpVendorName(qsfp_number))
                 << endl;
            cout << "Part Number   : " << *(sm->qsfpPartNumber(qsfp_number))
                 << endl;
            cout << "Temperature   : " << sm->qsfpTemperature(qsfp_number)
                 << "°C" << endl;
        }
        catch(...)
        {
            cout << "QSFP readout failed!" << endl;
        }
    }

    delete bar;
    delete dev;
    return 0;
}
