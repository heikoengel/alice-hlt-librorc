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
#include "../dma/dma_handling.hh"

using namespace std;

#define DMA_TIMEOUT 10.0

#define LIBRORC_INTERNAL
#define BOARDTEST_HELP_TEXT "boardtest usage: \n\
    ucctrl [parameters] \n\
Parameters: \n\
-h              Print this help \n\
-n [0...255]    Target device \n\
-v              Verbose mode \n\
"

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
    int verbose = 0;
    bool do_long_test = true;

    /** parse command line arguments */
    int arg;
    while ( (arg = getopt(argc, argv, "hn:v")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                {
                    cout << BOARDTEST_HELP_TEXT;
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

            case 'q':
                {
                    do_long_test = false;
                }
                break;

            default:
                {
                    cout << "Unknown parameter (" << arg << ")!" << endl;
                    cout << BOARDTEST_HELP_TEXT;
                    return -1;
                }
                break;
        } //switch
    } //while

    if ( device_number < 0 || device_number > 255 )
    {
        cout << "No or invalid device selected: " << device_number << endl;
        cout << BOARDTEST_HELP_TEXT;
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
    uint32_t nchannels = (fwtype & 0xffff);

    checkPcieState( sm );

    /** check sysclk is running */
    int sysclk_avail = checkSysClkAvailable( sm );


    librorc::link *link[nchannels];
    for  ( uint32_t i=0; i<nchannels; i++ )
    {
        link[i] = new librorc::link(bar, i);
        if (link[i]->isGtxDomainReady())
        {
            link[i]->clearAllGtxErrorCounters();
            link[i]->setGTX(RORC_REG_GTX_ERROR_CNT, 0);
        }
        else
        {
            cout << "ERROR: No clock on link " << i
                 << " - Skipping some checks!" << endl;
        }
    }

    /** check fan is running */
    checkFpgaFan( sm, verbose );


    /** check QSFP module slow eletrical interfaces */
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        checkQsfp( sm, i, verbose );
    }
    
    /** check current Si570 refclk setings */
    checkRefClkGen( sm, verbose );

    if ( sysclk_avail )
    {
        /** check FPGA temp & VCCs */
        checkFpgaSystemMonitor( sm, verbose );

        /** check DDR3 modules in C0/C1 */
        checkDdr3ModuleSpd( sm, 0, verbose);
        checkDdr3ModuleCalib( bar, 0);
        checkDdr3ModuleSpd( sm, 1, verbose);
        checkDdr3ModuleCalib( bar, 1);
    }

    /* check LVDS tester status */
    checkLvdsTester( bar );

    /** read uc signature */
    checkMicrocontroller ( bar, verbose );

    /** check flashes */
    checkFlash( dev, 0, verbose );
    checkFlash( dev, 1, verbose );

    if ( do_long_test )
    {

        testDmaChannel( dev, bar, DMA_TIMEOUT );
    }

    /** check DDR3 Traffic generator status */
    checkDdr3ModuleTg( bar, 0 );
    checkDdr3ModuleTg( bar, 1 );

    /** check link status */
    for  ( uint32_t i=0; i<nchannels; i++ )
    {
        checkLinkState( link[i], i );
    }

    for  ( uint32_t i=0; i<nchannels; i++ )
    {
        delete link[i];
    }
    delete sm;
    delete bar;
    delete dev;

    return 0;
}
