/**
 * @file channeldump.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-11
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

#include "librorc.h"

using namespace std;

#define HELP_TEXT "channeldump usage: \n\
        channeldump [parameters] \n\
parameters: \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -h            Show this text \n\
\n"

#define HEX32(x) setw(8) << setfill('0') << hex << x << setfill(' ')


int 
main
(
    int argc,
    char *argv[]
)
{
    /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;

    int arg;
    while( (arg = getopt(argc, argv, "hn:c:xs")) != -1 )
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


    if( !(dev->DMAChannelIsImplemented(ChannelId)) )
    {
        cout << "ChannelId invalid or not set: " << ChannelId << endl;
        cout << HELP_TEXT;
        abort();
    }

    uint32_t m_base = (ChannelId + 1) * RORC_CHANNEL_OFFSET;

    for ( int32_t i=0; i<25; i++ )
    {
        cout << i << " 0x" 
             << HEX32(bar->get32(m_base + i)) 
             << endl;
    }

    delete bar;
    delete dev;

    return 0;
}
