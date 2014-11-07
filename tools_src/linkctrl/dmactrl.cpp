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

#include <getopt.h>
#include <pda.h>
#include <librorc.h>

#define HELP_TEXT "dmactrl usage: \n\
        dmactrl [parameters] \n\
parameters: \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -x            Clear error counters \n\
        -R [rate]     Set event rate limit in Hz \n\
        -s            Show link status \n\
        -r            Set DMA channel reset \n\
        -h            Show this text \n\
\n"

using namespace std;

int main
(
    int argc,
    char *argv[]
)
{
    int do_clear = 0;
    int do_status = 0;
    int do_reset = 0;
    int do_ratelimit = 0;

    /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    uint32_t ratelimit = 0;

    int arg;
    while( (arg = getopt(argc, argv, "hn:c:xsrR:")) != -1 )
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
            
            case 's':
            {
                do_status = 1;
            }
            break;

            case 'r':
            {
                do_reset = 1;
            }
            break;

            case 'R':
            {
                ratelimit = strtol(optarg, NULL, 0);

                do_ratelimit = 1;
            }
            break;

            case 'x':
            {
                do_clear = 1;
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
    #ifdef MODELSIM
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

    librorc::sysmon *sm;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        printf("ERROR: failed to initialize Sysmon.\n");
        abort();
    }

    uint32_t startChannel, endChannel;
    uint32_t nChannels = sm->numberOfChannels();
    if ( ChannelId==-1 )
    {
        /** no specific channel selected, iterate over all channels */
        startChannel = 0;
        endChannel = nChannels - 1;
    }
    else if ( ChannelId < (int)nChannels )
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

    uint32_t pcie_gen = 1;
    if( do_ratelimit )
    { pcie_gen = sm->pcieGeneration(); }


    /** iterate over selected channels */
    for ( uint32_t chID=startChannel; chID<=endChannel; chID++ )
    {
        /** Create DMA channel and bind channel to BAR1 */
        librorc::link *current_link
            = new librorc::link(bar, chID);
        librorc::dma_channel *ch = new librorc::dma_channel(current_link);

        if(do_status)
        {
            cout << "CH" << dec << chID << " - DMA Stall Count: 0x"
                 << hex << ch->stallCount()
                 << "; #Events processed: 0x"
                 << ch->eventCount()
                 << endl;
        }

        if(do_clear)
        {
            ch->clearStallCount();
            ch->clearEventCount();
            ch->readAndClearPtrStallFlags();
        }

        if ( do_reset )
        { ch->disable(); }

        if ( do_ratelimit )
        {
            ch->setRateLimit(ratelimit, pcie_gen);
        }

        delete ch;
        delete current_link;
    }

    delete sm;
    delete bar;
    delete dev;

    return 0;
}
