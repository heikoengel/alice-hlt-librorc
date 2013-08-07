/**
 * @file gtxctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-19
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

using namespace std;

#define HELP_TEXT "gtxctrl usage: \n\
        gtxctrl [parameters] \n\
parameters: \n\
        --device [0...255]   Target device ID \n\
        --channel [0...11]   Channel ID \n\
        --clear              Clear error counters \n\
        --rxreset [0/1]      Set RX reset value \n\
        --txreset [0/1]      Set TX reset value \n\
        --gtxreset [0/1]     Set GTX reset value \n\
        --loopback [0...7]   Set GTX loopback value \n\
        --help               Show this text \n\
"


int main
(
    int argc,
    char *argv[]
)
{
    int clear = 0;
    int status = 0;
    int32_t rxreset = 0;
    int32_t txreset = 0;
    int32_t gtxreset = 0;
    int32_t loopback = 0;

    static struct option long_options[] =
    {
        {"device", required_argument, 0, 'd'},
        {"channel", required_argument, 0, 'c'},
        {"clear", no_argument, &clear, 1},
        {"rxreset", required_argument, 0, 'r'},
        {"txreset", required_argument, 0, 't'},
        {"gtxreset", required_argument, 0, 'g'},
        {"loopback", required_argument, 0, 'l'},
        {"status", no_argument, &status, 1},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /** parse command line arguments **/
    int32_t  DeviceId  = -1;
    int32_t  ChannelId = -1;
    while(1)
    {
        int opt = getopt_long(argc, argv, "", long_options, NULL);
        if ( opt == -1 )
        { break; }

        switch(opt)
        {
            case 'd':
            {
                DeviceId = strtol(optarg, NULL, 0);
            }
            break;

            case 'c':
            {
                ChannelId = strtol(optarg, NULL, 0);
            }
            break;

            case 'r':
            {
                rxreset = strtol(optarg, NULL, 0);
            }
            break;

            case 't':
            {
                txreset = strtol(optarg, NULL, 0);
            }
            break;

            case 'g':
            {
                gtxreset = strtol(optarg, NULL, 0);
            }
            break;

            case 'l':
            {
                loopback = strtol(optarg, NULL, 0);
            }
            break;

            case 'h':
            {
                cout << HELP_TEXT;
                exit(0);
            }
            break;

            default:
            {
                break;
            }
        }
    }

    /** sanity checks on command line arguments **/
    if( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "DeviceId invalid or not set: " << DeviceId << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( gtxreset > 1 )
    {
        cout << "gtxreset value invalid, allowed values are 0,1." << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( txreset > 1 )
    {
        cout << "txreset value invalid, allowed values are 0,1." << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( rxreset > 1 )
    {
        cout << "rxreset value invalid, allowed values are 0,1." << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( loopback > 7 )
    {
        cout << "loopback value invalid, allowed values are 0...7." << endl;
        cout << HELP_TEXT;
        abort();
    }

    /** Create new device instance */
    librorc::device *dev = NULL;
    try{ dev = new librorc::device(DeviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }

    /** bind to BAR1 */
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
        printf("ERROR: failed to initialize BAR1.\n");
        abort();
    }

    /** get number channels implemented in firmware */
    uint32_t type_channels = bar->get32(RORC_REG_TYPE_CHANNELS);

    uint32_t startChannel, endChannel;
    if ( ChannelId==-1 )
    {
        /** iterate over all channels */
        startChannel = 0;
        endChannel = (type_channels & 0xffff) - 1;
    }
    else if ( ChannelId < (int32_t)(type_channels & 0xffff) )
    {
        /** use only selected channel */
        startChannel = ChannelId;
        endChannel = ChannelId;
    }
    else
    {
        cout << "ERROR: Selected Channel " << ChannelId
             << " is not implemented in Firmware." << endl;
        abort();
    }

    /** iterate over selected channels */
    for ( uint32_t chID=startChannel; chID<=endChannel; chID++ )
    {
        /** Create DMA channel and bind channel to BAR1 */
        librorc::dma_channel *ch = new librorc::dma_channel();
        ch->init(bar, chID);

        /** get current GTX configuration */
        uint32_t gtxasynccfg = ch->getPKT(RORC_REG_GTX_ASYNC_CFG);

        if ( status )
        {
            cout << "CH " << setw(2) << chID << ": 0x"
                 << hex << setw(8) << setfill('0') << gtxasynccfg
                 << dec << setfill(' ') << endl;
        }
        else if ( clear )
        {
            /** make sure GTX clock is running */
            if ( (gtxasynccfg & (1<<8)) != 0 )
            {
                cout << "WARNING: CH " << chID
                     << " : GTX clock is not running - skipping." << endl;
                continue;
            }

            /** clear disparity error count */
            ch->setGTX(RORC_REG_GTX_DISPERR_CNT, 0);

            /** clear RX-not-in-table count */
            ch->setGTX(RORC_REG_GTX_RXNIT_CNT, 0);

            /** clear RX-Loss-Of-Signal count */
            ch->setGTX(RORC_REG_GTX_RXLOS_CNT, 0);

            /** clear RX-Byte-Realign count */
            ch->setGTX(RORC_REG_GTX_RXBYTEREALIGN_CNT, 0);

        }
        else /** set {rx/tx/gtx}reset, loopback */
        {
            /** clear previous reset bits (0,1,3) */
            gtxasynccfg &= ~(0x0000000b);

            /** clear previous loopback bits */
            gtxasynccfg &= ~(0x00000007<<9);

            /** set new reset values */
            gtxasynccfg |= (gtxreset&1);
            gtxasynccfg |= ((rxreset&1)<<1);
            gtxasynccfg |= ((txreset&1)<<3);

            /** set new loopback value */
            gtxasynccfg |= ((loopback&7)<<9);

            /** write new values to RORC */
            ch->setPKT(RORC_REG_GTX_ASYNC_CFG, gtxasynccfg);

        }

        delete ch;
    }

    delete bar;
    delete dev;

    return 0;
}
