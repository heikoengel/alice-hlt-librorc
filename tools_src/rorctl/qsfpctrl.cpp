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
#include <cmath>
#include <unistd.h>

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



float
mWatt2dBm
(
    float mwatt
)
{
    return 10*log10(mwatt);
}


int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int32_t qsfp_number   = -1;
    uint32_t reset_val    =  0;
    int do_reset          =  0;
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
    try
    { dev = new librorc::device(device_number); }
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

    if ( do_reset )
    {
        cout << "Setting QSFP" << qsfp_number << " Reset to "
             << reset_val << endl;
        sm->qsfpSetReset(qsfp_number, reset_val);
    }

    cout << "Module Present: " << sm->qsfpIsPresent(qsfp_number) << endl;
    cout << "Module Reset  : " << sm->qsfpGetReset(qsfp_number) << endl;

    if( !do_reset && sm->qsfpIsPresent(qsfp_number) &&
            !sm->qsfpGetReset(qsfp_number) )
    {
        try
        {
            cout << "Vendor Name   : " << sm->qsfpVendorName(qsfp_number)
                 << endl;
            cout << "Part Number   : " << sm->qsfpPartNumber(qsfp_number)
                 << endl;
            cout << "Revision      : "
                 << sm->qsfpRevisionNumber(qsfp_number) << endl;
            cout << "Serial Number : "
                 << sm->qsfpSerialNumber(qsfp_number) << endl;
            cout << "Wavelength    : " << sm->qsfpWavelength(qsfp_number)
                 << " nm" << endl;
            cout << "Temperature   : " << sm->qsfpTemperature(qsfp_number)
                 << " ��C" << endl;
            cout << "Voltage       : " << sm->qsfpVoltage(qsfp_number)
                 << " V" << endl;
            cout << "RateSel Sup.  : " ;
            switch (sm->qsfpGetRateSelectionSupport(qsfp_number))
            {
                case LIBRORC_SYSMON_QSFP_NO_RATE_SELECTION:
                    cout << "NONE";
                    break;
                case LIBRORC_SYSMON_QSFP_EXT_RATE_SELECTION:
                    cout << "EXTENDED";
                    break;
                case LIBRORC_SYSMON_QSFP_APT_RATE_SELECTION:
                    cout << "APT";
                    break;
            }
            cout << endl;
            cout << "Power Class   : "
                << (int)sm->qsfpPowerClass(qsfp_number) << endl;
            cout << "CDR in TX     : "
                << (int)sm->qsfpHasTXCDR(qsfp_number) << endl;
            cout << "CDR in RX     : "
                << (int)sm->qsfpHasRXCDR(qsfp_number) << endl;

            cout << "TX Fault Map  : "
                 << (int)sm->qsfpTxFaultMap(qsfp_number)
                 << endl;
            for ( int i=0; i<4; i++ )
            {
                float mwatt = sm->qsfpRxPower(qsfp_number, i);
                cout << "RX Power CH" << i << "  : "
                     << mwatt << " mW (" << mWatt2dBm(mwatt)
                     << " dBm)" << endl;
            }
            for ( int i=0; i<4; i++ )
            {
                float txbias = sm->qsfpTxBias(qsfp_number, i);
                cout << "TX Bias CH" << i << "   : "
                     << txbias << " mA" << endl;
            }
        }
        catch(...)
        {
            cout << "QSFP readout failed!" << endl;
        }
    }

    delete sm;
    delete bar;
    delete dev;
    return 0;
}
