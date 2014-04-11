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

    uint32_t firmware_type = sm->firmwareType();

    for ( uint32_t i=start_channel; i<=end_channel; i++ )
    {
        librorc::link *link = new librorc::link(bar, i);
        uint32_t link_type = link->linkType();


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
            link->setFlowControlEnable(0);
            link->clearAllGtxErrorCounters();

            switch( link_type )
            {
                case RORC_CFG_LINK_TYPE_DIU:
                    {
                        librorc::diu *diu = new librorc::diu(link);
                        diu->setReset(1);
                        diu->disableInterface();
                        diu->clearEventcount();
                        diu->clearDeadtime();
                        diu->clearAllLastStatusWords();
                        delete diu;

                        if( firmware_type == RORC_CFG_PROJECT_hlt_in_fcf )
                        {
                            librorc::fastclusterfinder *fcf = 
                                new librorc::fastclusterfinder(link);
                            fcf->setState(1, 0); // reset, not enabled
                            delete fcf;
                        }
                    }
                    break;

                case RORC_CFG_LINK_TYPE_SIU:
                    {
                        librorc::siu *siu = new librorc::siu(link);
                        siu->setReset(1);
                        siu->disableInterface();
                        siu->clearEventcount();
                        siu->clearDeadtime();
                        delete siu;
                    }
                    break;

                case RORC_CFG_LINK_TYPE_VIRTUAL:
                    {
                        librorc::ddl *ddlraw = new librorc::ddl(link);
                        ddlraw->disableInterface();
                        ddlraw->clearDeadtime();
                        delete ddlraw;
                    }
                    break;

                default: // LINK_TEST, IBERT
                    {
                    }
                    break;
            }

            // PG on HLT_IN, HLT_OUT, HWTEST
            if( link->patternGeneratorAvailable() )
            {
                librorc::patterngenerator *pg =
                    new librorc::patterngenerator(link);
                pg->disable();
                delete pg;
            }

            // reset DDR3 Data Replay if available
            if( link->ddr3DataReplayAvailable() )
            {
                link->setDdr3DataReplayChannelReset(1);
            }
        }

        link->disableDmaEngine();
        link->clearAllDmaCounters();

        delete link;
    }

    return 0;
}
