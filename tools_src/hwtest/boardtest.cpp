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

#include "librorc.h"
#include "crorc-smbus-ctrl.hh"
#include "boardtest_modules.hh"
#include "fmc_tester.hh"
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
-f              First run mode - configure uC and flashes \n\
-q              Quick run, skip DMA test \n\
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
    int firstrun = 0;
    bool do_long_test = true;

    /** parse command line arguments */
    int arg;
    while ( (arg = getopt(argc, argv, "hn:vqf")) != -1 )
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

            case 'f':
                {
                    firstrun = 1;
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
        cout << "FATAL: No or invalid device selected: " << device_number << endl;
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
        cout << "FATAL: Failed to intialize device " << device_number << endl;
        abort();
    }

    printHeader("PCIe");

    struct pci_dev *pdev = initLibPciDev(dev);

    if( !checkForValidBarConfig(pdev) )
    {
        cout << "FATAL: invalid BAR configuration - "
             << "did you rescan the bus after programming?"
             << endl;
        abort();
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
        cout << "FATAL: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    librorc::sysmon *sm;
    try
    { sm = new librorc::sysmon(bar); }
    catch(...)
    {
        cout << "FATAL:Sysmon init failed!" << endl;
        delete bar;
        delete dev;
        abort();
    }

    printPcieInfos(dev, sm);

    cout << "INFO: FPGA Unique Device ID: "
        << HEXSTR(getPcieDSN(pdev), 16) << endl;

    /** check FW type */
    uint32_t fwtype = bar->get32(RORC_REG_TYPE_CHANNELS);
    if ( (fwtype>>16) != RORC_CFG_PROJECT_hwtest )
    {
        cout << "FATAL: No suitable FW detected - aborting." << endl;
        delete bar;
        delete dev;
        abort();
    }
    uint32_t nchannels = (fwtype & 0xffff);

    checkPcieState( sm, verbose );

    checkAndReleaseQsfpResets( sm, verbose );

    /** check fan is running */
    checkFpgaFan( sm, verbose );

    printHeader("Clocking/Monitoring");

    /** check sysclk is running */
    int sysclk_avail = checkSysClkAvailable( sm );

    librorc::link *link[nchannels];
    for  ( uint32_t i=0; i<nchannels; i++ )
    {
        link[i] = new librorc::link(bar, i);

        checkAndReleaseGtxReset( link[i], verbose );

        if (link[i]->isGtxDomainReady())
        {
            link[i]->clearAllGtxErrorCounters();
            link[i]->setGtxReg(RORC_REG_GTX_ERROR_CNT, 0);
        }
        else
        {
            cout << "ERROR: No clock on link " << i
                 << " - Skipping some checks!" << endl;
        }
    }

    /** check current Si570 refclk setings */
    checkRefClkGen( sm, verbose );
        
    if ( sysclk_avail )
    {
        /** check FPGA temp & VCCs */
        checkFpgaSystemMonitor( sm, verbose );
    }


    printHeader("QSFPs");

    /** check QSFP module slow eletrical interfaces */
    bool qsfpReady[LIBRORC_MAX_QSFP];
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        qsfpReady[i] = checkQsfp( sm, i, verbose );
    }
    

    if ( sysclk_avail )
    {
        printHeader("DDR3");
        /** check DDR3 modules in C0/C1 */
        checkDdr3ModuleSpd( sm, 0, verbose);
        checkDdr3ModuleCalib( bar, 0, verbose);
        checkDdr3ModuleSpd( sm, 1, verbose);
        checkDdr3ModuleCalib( bar, 1, verbose);


        printHeader("FMC Loopback Adapter");
        if ( checkFmc(bar, sm, verbose) != 0 )
        {
            cout << "ERROR: FMC Loopback Adapter Test failed." << endl;
        } else if (verbose)
        {
            cout << "INFO: FMC Loopback Test passed." << endl;
        }
    }

    printHeader("LVDS");
    /* check LVDS tester status */
    checkLvdsTester( bar, verbose );

    printHeader("Microcontroller");
    checkMicrocontroller ( bar, firstrun, verbose );
    checkSmBus ( bar, verbose );

    printHeader("Flash Chips");
    /** check flashes */
    uint64_t flash0_devnr = checkFlash( dev, 0, verbose );
    uint64_t flash1_devnr = checkFlash( dev, 1, verbose );
    checkFlashDeviceNumbersAreValid(flash0_devnr, flash1_devnr);


    if ( do_long_test )
    {
        printHeader("DMA to Host");
        if ( link[0]->isGtxDomainReady() )
        {
        testDmaChannel( dev, bar, 0, DMA_TIMEOUT, verbose );
        }
        else
        {
            cout << "ERROR: Link 0 Clock is down - skipping DMA test"
                 << endl;
        }
    }


    printHeader("DDR3 Pattern Generator");
    /** check DDR3 Traffic generator status */
    checkDdr3ModuleTg( bar, 0, verbose );
    checkDdr3ModuleTg( bar, 1, verbose );


    printHeader("Optical Link Tester");
    /** check link status */
    for  ( uint32_t i=0; i<nchannels; i++ )
    {
        if (qsfpReady[i>>2])
        {
            checkLinkState( link[i], i );
        }
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
