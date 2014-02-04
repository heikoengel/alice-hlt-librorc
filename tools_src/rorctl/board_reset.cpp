/**
 * @file board_reset.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-03
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

#include <librorc.h>

using namespace std;

#define HELP_TEXT "board_reset usage: \n\
reset_error_counters [parameters] \n\
Parameters: \n\
        -h              Print this help \n\
        -n [0...255]    Target device \n\
        -c [channelID]  (optional) channel ID \n\
"

int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    uint32_t channel_number = 0xffffffff;
    int arg;
    int do_full_reset = 0;

    /** parse command line arguments */
    while ( (arg = getopt(argc, argv, "hn:c:f")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                cout << HELP_TEXT;
                return 0;
                break;
            case 'n':
                device_number = strtol(optarg, NULL, 0);
                break;
            case 'c':
                channel_number = strtol(optarg, NULL, 0);
                break;
            case 'f':
                do_full_reset = 1;
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

    librorc::sysmon *sm = NULL;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize System Manager." << endl;
        delete bar;
        delete dev;
        abort();
    }

    sm->clearSysmonErrorCounters();

    uint32_t start_channel = (channel_number!=0xffffffff) ?
        channel_number : 0;
    uint32_t end_channel = sm->numberOfChannels()-1;

    for ( uint32_t i=start_channel; i<=end_channel; i++ )
    {
        librorc::link *link = new librorc::link(bar, i);

        if ( do_full_reset )
        {
            uint32_t linkcfg = link->packetizer(RORC_REG_GTX_ASYNC_CFG);
            linkcfg |= 0x00000001; // set GTX_RESET
            link->setPacketizer(RORC_REG_GTX_ASYNC_CFG, linkcfg);
            usleep(1000);
            linkcfg &= ~(0x00000001); // release GTX_RESET
            link->setPacketizer(RORC_REG_GTX_ASYNC_CFG, linkcfg);
            link->waitForGTXDomain();
        }

        if(link->isGtxDomainReady())
        {

            link->clearAllGtxErrorCounters();

            switch( sm->firmwareType() )
            {
                case RORC_CFG_PROJECT_hlt_in:
                    link->clearAllLastDiuStatusWords();
                    link->disablePatternGenerator();
                    break;
                case RORC_CFG_PROJECT_hwtest:
                    link->disablePatternGenerator();
                    break;
                case RORC_CFG_PROJECT_hlt_out:
                    link->disablePatternGenerator();
                    break;
                case RORC_CFG_PROJECT_hlt_in_fcf:
                    link->disableDdr3DataReplayChannel();
                    bar->set32(RORC_REG_DATA_REPLAY_CTRL, 0);
                    link->disableFcf();
                    break;
                default:
                    break;
            }
            link->disableDdl();
        }

        link->disableDmaEngine();
        link->clearAllDmaCounters();

        delete link;
    }

    return 0;
}
