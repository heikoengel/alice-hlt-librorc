/**
 * @file boardtest.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-26
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
#include "boardtest_modules.hh"

using namespace std;

#define HELP_TEXT "ucctrl usage: \n\
    ucctrl [parameters] \n\
Parameters: \n\
-h              Print this help \n\
-n [0...255]    Target device \n\
-v              Verbose mode \n\
"

/** define value ranges */
#define FPGA_FANSPEED_MIN 8000.0
#define FPGA_FANSPEED_MAX 10000.0


/**
 * MAIN
 * */
int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int sysclk_avail = 1;
    int verbose = 0;

    /** parse command line arguments */
    int arg;
    while ( (arg = getopt(argc, argv, "hn:v")) != -1 )
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
                    device_number = strtol(optarg, NULL, 0);
                }
                break;

            case 'v':
                {
                    verbose = 1;
                }
                break;

            default:
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
        cout << "No or invalid device selected: " << device_number << endl;
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
        cout << "Failed to intialize device " << device_number << endl;
        abort();
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

    cout << "Device [" <<  device_number << "] ";
    cout << " - Firmware date: " << hex << setw(8) << sm->FwBuildDate()
         << ", Revision: "      << hex << setw(8) << sm->FwRevision()
         << dec << endl;

    /** check FW type */
    uint32_t fwtype = bar->get32(RORC_REG_TYPE_CHANNELS);
    if ( (fwtype>>16) != RORC_CFG_PROJECT_hwtest )
    {
        cout << "ERROR: No suitable FW detected - aborting." << endl;
        delete bar;
        delete dev;
        abort();
    }

    checkPcieState( sm );

    /** check sysclk is running */
    if ( !sm->systemClockIsRunning() )
    {
        cout << "ERROR: 200 MHz system clock not detected " 
             << "- have to skipp some test now..." << endl;
        sysclk_avail = 0;
    }

    /** check fan is running */
    checkFpgaFan( sm, verbose );


    /** check QSFP present and i2c status */ 
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        if ( !sm->qsfpIsPresent(i) )
        {
            cout << "WARNING: No QSFP detected in slot "
                 << i << " - skipping some tests..." << endl;
        }
        else if ( sm->qsfpGetReset(i) )
        {
            cout << "WARNING: QSFP " << i 
                 << " seems to be in RESET state - " 
                 << " skipping some tests" << endl;
        }
        else
        {
            checkQsfpTemperature( sm, i, verbose );
            checkQsfpVcc( sm, i, verbose );
            checkQsfpOpticalLevels( sm, i, verbose );
        }
    }

    
    /** check current Si570 refclk setings */
    checkRefClkGen( sm, verbose );

    //TODO
    /** check GTX clk is running */

    if ( sysclk_avail )
    {
        /** check FPGA temp & VCCs */
        checkFpgaSystemMonitor( sm, verbose );

        /** check DDR3 module types in C0/C1 */
        checkDdr3Module( sm, 0, verbose);
        checkDdr3Module( sm, 1, verbose);
    }

    /* check LVDS tester status */
    checkLvdsTester( bar );

    /** read uc signature */
    checkMicrocontroller ( bar, verbose );

    delete sm;
    delete bar;

    /** check flashes */
    checkFlash( dev, 0, verbose );
    checkFlash( dev, 1, verbose );

    delete dev;

    return 0;
}
