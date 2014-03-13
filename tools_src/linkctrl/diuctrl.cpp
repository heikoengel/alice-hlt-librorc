/**
 * @file diuctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-03-13
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
#include <getopt.h>
#include <pda.h>
#include <librorc.h>

using namespace std;

#define HELP_TEXT "ddlctrl usage: \n\
        ddlctrl [parameters] \n\
        -n [0...255]     Target device ID \n\
        -c [0...11]      Channel ID \n\
        -t [diu,siu,fee] Set command target \n\
        -r               Read & clear interface status word \n\
        -h               Show this help \n\
"

void
printDiuIfErrors
(
    uint32_t ifstw
)
{
    (ifstw>>31) ? printf("DIU status error\n") : 0;
    ((ifstw>>29)&1) ? printf("Loss of synchronization\n") : 0;
    ((ifstw>>28)&1) ? printf("TX Overflow\n") : 0;
    ((ifstw>>26)&1) ? printf("Ordered set inside the frame\n") : 0;
    ((ifstw>>26)&1) ? printf("Ordered set inside the frame\n") : 0;
    ((ifstw>>25)&1) ? printf("Invalid character inside the frame\n") : 0;
    ((ifstw>>24)&1) ? printf("CRC error\n") : 0;
    ((ifstw>>22)&1) ? printf("Data word out of frame\n") : 0;
    ((ifstw>>21)&1) ? printf("Invalid frame delimiter\n") : 0;
    ((ifstw>>20)&1) ? printf("Frame length error\n") : 0;
    ((ifstw>>19)&1) ? printf("RX overflow\n") : 0;
    ((ifstw>>18)&1) ? printf("Frame error (incomplete)\n") : 0;
}

void
printDiuIfState
(
    uint32_t ifstw
)
{
    //[14:12]: lm_txstate
    switch( (ifstw>>12)&3 )
    {
        case 0:
            printf("TX_POWERONRST\n");
            break;
        case 1:
            printf("TX_OFFLINE\n");
            break;
        case 2:
            printf("TX_ONLINE\n");
            break;
        case 3:
            printf("TX_RXSUSPEND\n");
            break;
        case 4:
            printf("TX_RXNOSIG\n");
            break;
        case 5:
            printf("TX_RXBADSIG\n");
            break;
        case 6:
            printf("TX_LOWPOWER\n");
            break;
        default:
            printf("TX_TESTMODE\n");
            break;
    }
    
    //[17:15]: lm_rxstate
    switch( (ifstw>>12)&3 )
    {
        case 0:
            printf("RX_IDLE\n");
            break;
        case 1:
            printf("RX_DXDATA00\n");
            break;
        case 2:
            printf("RX_ONLINE\n");
            break;
        case 3:
            printf("RX_SUSPEND\n");
            break;
        case 4:
            printf("RX_DXDATA01\n");
            break;
        case 5:
            printf("RX_OTHERS\n");
            break;
        case 6:
            printf("RX_NOSIG\n");
            break;
        default:
            printf("RX_PRBS\n");
            break;
    }
}

void
printSiuIfErrors
(
    uint32_t ifstw
)
{
    (ifstw>>31) ? printf("SIU status error\n") : 0;
    ((ifstw>>30)&1) ? printf("Too long frame\n") : 0;
    ((ifstw>>29)&1) ? printf("Illegal status or data word from FEE\n") : 0;
    ((ifstw>>28)&1) ? printf("Transmit FIFO overflow\n") : 0;
    ((ifstw>>27)&1) ? printf("Illegal data\n") : 0;
    ((ifstw>>26)&1) ? printf("Ordered set inside frame\n") : 0;
    ((ifstw>>25)&1) ? printf("Invalid character\n") : 0;
    ((ifstw>>24)&1) ? printf("CRC error\n") : 0;
    ((ifstw>>23)&1) ? printf("Block length mismatch\n") : 0;
    ((ifstw>>22)&1) ? printf("Data word out of frame\n") : 0;
    ((ifstw>>21)&1) ? printf("Invalid frame delimiter\n") : 0;
    ((ifstw>>20)&1) ? printf("Frame length error\n") : 0;
    ((ifstw>>19)&1) ? printf("Receiver overflow\n") : 0;
    ((ifstw>>18)&1) ? printf("Frame error (incomplete)\n") : 0;
    ((ifstw>>17)&1) ? printf("Protocol error\n") : 0;
    //((ifstw>>16)&1) ? printf("Front-end Bus loopback enable\n") : 0;
    //((ifstw>>15)&1) ? printf("FEE bus is not in idle state\n") : 0;
    //ifstw[14:12]: lmstatus
}

void
printSiuIfState
(
    uint32_t ifstw
)
{
    ((ifstw>>16)&1) ? printf("Front-end Bus loopback enable\n") : 0;
    ((ifstw>>15)&1) ? printf("FEE bus active\n") :
        printf("FEE bus in idle state\n");
    switch( (ifstw>>12)&7 )
    {
        case 0:
            printf("");
            break;
        case 1:
            printf("Offline\n");
            break;
        case 4:
            printf("RXnosig\n");
            break;
        case 5:
            printf("RXbadsig\n");
            break;
        case 2:
            printf("Online\n");
            break;
        case 3:
            printf("Suspend\n");
            break;
        default:
            printf("unknown/reserved status\n");
            break;
    }
}





int main
(
    int argc,
    char *argv[]
)
{
    int do_rcifst = 0;

    uint32_t target = 0;

   /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    int arg;
    while( (arg = getopt(argc, argv, "hn:c:t:r")) != -1 )
    {
        switch(arg)
        {
            case 'n':
            {
                DeviceId = strtol(optarg, NULL, 0);
            }
            break;

            case 'c':
            {
                ChannelId = strtol(optarg, NULL, 0);
            }
            break;

            case 't':
            {
                if( strcmp(optarg, "diu") == 0 )
                { target = 1; }
                else if( strcmp(optarg, "siu") == 0 )
                { target = 2; }
                else if( strcmp(optarg, "fee") == 0 )
                { target = 4; }
                else
                { target = 0; }
            }
            break;

            case 'r':
            {
                do_rcifst = 1;
            }
            break;

            default:
            {
                cout << "Unknown parameter (" << arg << ")!" << endl;
                cout << HELP_TEXT;
                return -1;
            }
        }
    }

    if( target==0 )
    {
        cout << "ERROR: No Target selected!" << endl;
        cout << HELP_TEXT;
        abort();
    }
    

    /** sanity checks on command line arguments **/
    if( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "DeviceId invalid or not set: " << DeviceId << endl;
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
        /** no specific channel selected, iterate over all channels */
        startChannel = 0;
        endChannel = (type_channels & 0xffff) - 1;
    }
    else if( dev->DMAChannelIsImplemented(ChannelId) )
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
        librorc::link *current_link
            = new librorc::link(bar, chID);

        /** check if GTX domain is up */
        if ( !current_link->isGtxDomainReady() )
        {
            cout << "GTX Domain not ready for channel "
                 << chID << " - skipping..." << endl;
            continue;
        }

        librorc::diu *diu = new librorc::diu(current_link);

        if( do_rcifst )
        {
            uint32_t ifstw = 0;
            switch(target)
            {
                case 1:
                    ifstw = diu->readAndClearDiuInterfaceStatus();
                    if( ifstw != 0xffffffff )
                    {
                        printDiuIfErrors(ifstw);
                        printDiuIfState(ifstw);
                    }
                    break;
                case 2:
                    ifstw = diu->readAndClearSiuInterfaceStatus();
                    if( ifstw != 0xffffffff )
                    {
                        printSiuIfErrors(ifstw);
                        printSiuIfState(ifstw);
                    }
                    break;
                default:
                    cout << "R&CIFSTW is not available for selected target"
                         << endl;
            }
            cout << hex << ifstw << dec << endl;

        }

        delete diu;
        delete current_link;
    }

    delete bar;
    delete dev;

    return 0;
}

