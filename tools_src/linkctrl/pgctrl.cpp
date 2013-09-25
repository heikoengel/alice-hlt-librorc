/**
 * @file pgctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-20
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

#define HELP_TEXT "pgctrl usage: \n\
        pgtrl [parameters] \n\
parameters: \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -e [0,1]      Enable Pattern Generator \n\
        -m [0...3]    set PG mode \n\
\n\
"

int main
(
    int argc,
    char *argv[]
)
{
    int set_mode = 0;
    int modeval = -1;
    int set_en = 0;
    int enval = -1;

    /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    
    int arg;
    while( (arg = getopt(argc, argv, "hn:c:m:e:")) != -1 )
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

            case 'm':
            {
                modeval = strtol(optarg, NULL, 0);
                set_mode = 1;
            }
            break;

            case 'e':
            {
                enval = strtol(optarg, NULL, 0);
                set_en = 1;
            }
            break;

            case 'h':
            {
                cout << HELP_TEXT;
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

        /** wait for GTX domain to be ready */
        while( !current_link->isGtxDomainReady() ) { usleep(1000); }

        /** get PG config */
        uint32_t pgctrl = current_link->GTX(RORC_REG_DDL_CTRL);

        if ( set_mode )
        {
            if ( modeval<0 || modeval>3 )
            {
                cout << "Invalid PG mode value " << modeval << endl;
            }
            else
            {
                /** clear bits 12:11, set new value and write back */
                pgctrl &= ~(3<<11);
                pgctrl |= (modeval<<11);
            }
        }

        if ( set_en )
        {
            if ( enval!=0 && enval!=1 )
            {
                cout << "Invalid Enable value " << enval << endl;
            }
            else
            {
                /** clear bits 10:8, set new value and write back */
                pgctrl &= ~(7<<8);
                pgctrl |= (enval<<8);
                pgctrl |= (1<<9); //adaptive on
                pgctrl |= (1<<10); //continuous on
            }
        }

        if ( set_mode || set_en )
        {
            current_link->setGTX(RORC_REG_DDL_CTRL, pgctrl);
        }

        cout << "Channel " << chID << " PG Mode: "
             << ((pgctrl>>10)&3) << endl;

        delete current_link;
    }

    delete bar;
    delete dev;

    return 0;
}
