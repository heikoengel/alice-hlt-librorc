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

#include <iomanip>
#include <getopt.h>
#include <librorc.h>
#include "board_to_board_test_modules.hh"

#define BOARD_TO_BOARD_HELP_TEXT "board_to_board_test usage: \n\
    board_to_board_test [parameters] \n\
Parameters: \n\
-h              Print this help \n\
-s [0...255]    Source device \n\
-d [0...255]    Target device \n\
-v              Verbose mode \n\
"

using namespace std;

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
    int32_t devnr[2] = {-1, -1};
    int verbose = 0;

    /** parse command line arguments */
    int arg;
    while ( (arg = getopt(argc, argv, "hs:d:v")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                {
                    cout << BOARD_TO_BOARD_HELP_TEXT;
                    return 0;
                }
                break;

            case 's':
                { devnr[0] = strtol(optarg, NULL, 0); }
                break;

            case 'd':
                { devnr[1] = strtol(optarg, NULL, 0); }
                break;

            case 'v':
                { verbose = 1; }
                break;

            default:
                {
                    cout << "Unknown parameter!"
                         << endl << BOARD_TO_BOARD_HELP_TEXT;
                    return -1;
                }
                break;
        } //switch
    } //while

    if ( devnr[0] < 0 )
    {
        cout << "FATAL: No source device selected." << endl
             << BOARD_TO_BOARD_HELP_TEXT;
        abort();
    }
    else if ( devnr[1] < 0 )
    {
        cout << "FATAL: No target device selected." << endl
             << BOARD_TO_BOARD_HELP_TEXT;
        abort();
    }
    if ( devnr[0] == devnr[1] )
    {
        cout << "FATAL: Source and target device cannot be the same!"
             << endl << BOARD_TO_BOARD_HELP_TEXT;
        abort();
    }

    librorc::device *dev[2] = {NULL, NULL};
    librorc::bar *bar[2] = {NULL, NULL};
    librorc::sysmon *sm[2] = {NULL, NULL};

    uint32_t nchannels[2];
    for ( int i=0; i<2; i++ )
    {
        initLibrorcInstances( &dev[i], devnr[i], &bar[i], &sm[i]);
        nchannels[i] = sm[i]->numberOfChannels();
    }


    int32_t nconfigs = sizeof(available_configs) /
                       sizeof(librorc::gtxpll_settings);

    if ( verbose )
    {
        cout << "Testing " << nconfigs << " Link configurations."
             << endl;
    }

    for ( int i=0; i<nconfigs; i++ )
    {

        cout << "Configuring for Link Rate "
             << setprecision(3) << fixed
             << LinkRateFromPllSettings( available_configs[i])
             << " Gbps, RefClk "
             << available_configs[i].refclk
             << " MHz" << endl;

        if ( verbose )
        { cout << "Resetting GTX" << endl; }
        /** put all GTX transceivers in reset */
        resetAllGtx( bar[0], nchannels[0], 1 );
        resetAllGtx( bar[1], nchannels[1], 1 );

        if ( verbose )
        { cout << "Resetting QSFPs" << endl; }
        /** put all installed QSFPs in reset */
        resetAllQsfps( sm[0], 1 );
        resetAllQsfps( sm[1], 1 );

        /** configure reference clock for target clock frequency */
        if ( verbose )
        { cout << "Configuring RefClks" << endl; }
        configureRefclk( sm[0], available_configs[i].refclk );
        configureRefclk( sm[1], available_configs[i].refclk );

        /** configure all GTX instances for new link rate */
        if ( verbose )
        { cout << "Configuring GTX via DRP" << endl; }
        configureAllGtx( bar[0], nchannels[0], available_configs[i] );
        configureAllGtx( bar[1], nchannels[1], available_configs[i] );

        /** release GTX resets */
        if ( verbose )
        { cout << "Releasing GTX resets" << endl; }
        resetAllGtx( bar[0], nchannels[0], 0 );
        resetAllGtx( bar[1], nchannels[1], 0 );

        /** release QSFP resets */
        if ( verbose )
        { cout << "Releasing QSFP resets" << endl; }
        resetAllQsfps( sm[0], 0 );
        resetAllQsfps( sm[1], 0 );

        /** wait for link to go up, break after timeout */
        uint32_t lnkup0 = waitForLinkUp( bar[0], nchannels[0] );
        uint32_t lnkup1 = waitForLinkUp( bar[1], nchannels[0] );

        if ( lnkup0 != (uint32_t)((1<<nchannels[0])-1) ||
                lnkup1 != (uint32_t)((1<<nchannels[1])-1) )
        {
            cout << "ERROR: Not all links came up - aborting..."
                 << endl;
            abort();
        }

        clearAllErrorCounters( bar[0], nchannels[0] );
        clearAllErrorCounters( bar[1], nchannels[1] );

        /** wait some time */
        if ( verbose )
        { cout << "Starting BER test" << endl; }
        countDown( bar[0], WAIT_LINK_TEST );

        checkErrorCounters( bar[0], nchannels[0], devnr[0] );
        checkErrorCounters( bar[1], nchannels[1], devnr[1] );
    }

    /**
     * Sequence
     * - Reset all GTX
     * - Reset all QSFPs
     * - Reset RefClk
     * - Configure GTX
     * - Release all resets
     * - make sure links are up
     * - Clear all error counters
     * - wait TIME
     * - check error counters
     *
     * - Reset all GTX
     * - Reset all QSFPs
     * - Reconfigure Refclk
     * - Configure GTX
     * - Release all resets
     * - make sure links are up
     * - Clear all error counters
     * - wait TIME
     * - check error counters
     **/


    for ( int i=0; i<2; i++ )
    {
        delete sm[i];
        delete bar[i];
        delete dev[i];
    }

    return 0;
}
