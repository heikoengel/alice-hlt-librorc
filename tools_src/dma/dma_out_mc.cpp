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

#define LIBRORC_INTERNAL

#include <librorc.h>

#include "dma_handling.hh"

using namespace std;

#define MAX_CHANNELS 12

bool done = false;
void abort_handler( int s )                                          \
{                                        \
    printf("Caught signal %d\n", s);     \
    if( done==true )      \
    { exit(-1); }                        \
    else                                 \
    { done = true; }                     \
}



int main(int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
    DMAOptions opts[MAX_CHANNELS];
    opts[0] = evaluateArguments(argc, argv);
    int32_t nChannels = opts[0].channelId;
    opts[0].channelId = 0;
    opts[0].esType = LIBRORC_ES_TO_DEVICE;

    int i;

    for (i=1; i<nChannels; i++)
    {
        opts[i] = opts[0];
        opts[i].channelId = i;
    }

    DMA_ABORT_HANDLER_REGISTER

    librorc_event_callback event_callback = eventCallBack;


    librorc::device *dev;
    librorc::bar *bar;
    librorc::sysmon *sm;

    try
    {
        dev = new librorc::device(opts[0].deviceId);
    #ifdef MODELSIM
        bar = new librorc::sim_bar(dev, 1);
    #else
        bar = new librorc::rorc_bar(dev, 1);
    #endif
        sm = new librorc::sysmon(bar);

    }
    catch(...)
    {
        cout << "Failed to initialize dev, bar or sm" << endl;
        abort();
    }

    bar->simSetPacketSize(128);

    // check if firmware is HLT_OUT
    if( !sm->firmwareIsHltOut() )
    {
        cout << "Firmware is not HLT_OUT - exiting." << endl;
        abort();
    }


    uint64_t last_bytes_received[MAX_CHANNELS];
    uint64_t last_events_received[MAX_CHANNELS];

    librorc::high_level_event_stream *hlEventStream[MAX_CHANNELS];
    for(i=0; i<nChannels; i++ )
    {
        cout << "Prepare Event Stream " << i << endl;
        if( !(hlEventStream[i] = prepareEventStream(dev, bar, opts[i])) )
        { exit(-1); }

        hlEventStream[i]->setEventCallback(event_callback);

        last_bytes_received[i] = 0;
        last_events_received[i] = 0;
    }

    hlEventStream[0]->printDeviceStatus();

    /** make clear what will be checked*/
    int32_t sanity_check_mask = CHK_SIZES;
    if(opts[0].useRefFile)
    { sanity_check_mask |= CHK_FILE; }

    cout << "Event Loop Start" << endl;

    librorc::event_generator generators[MAX_CHANNELS];
    for(i=0; i<nChannels; i++)
    { generators[i] = librorc::event_generator(hlEventStream[i]); }

    librorc::event_sanity_checker checkers[MAX_CHANNELS];
    for(i=0; i<nChannels; i++)
    {
        checkers[i] =
            (opts[i].useRefFile) /** is DDL reference file enabled? */
            ?   librorc::event_sanity_checker
            (
             hlEventStream[i]->m_eventBuffer,
             opts[i].channelId,
             sanity_check_mask,
             logdirectory,
             opts[i].refname
            )
            :   librorc::event_sanity_checker
            (
             hlEventStream[i]->m_eventBuffer,
             opts[i].channelId,
             sanity_check_mask,
             logdirectory
            );
    }


    /** Create event stream */
    uint64_t result = 0;
    int refill[nChannels];
    for( i=0; i<nChannels; i++ )
    {
        refill[i] = 1;
    }

    /** Capture starting time */
    timeval start_time;
    hlEventStream[0]->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;


    /** Event loop */
    while( !done )
    {

        for ( i=0; i<nChannels; i++ )
        {
            if( refill[i] )
            {
                uint32_t nevents =
                    generators[i].fillEventBuffer(opts[i].eventSize);

                if(nevents > 0)
                { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", nevents);}
            }

            result = hlEventStream[i]->handleChannelData( (void*)&(checkers[i]) );
            refill[i] = (result!=0) ? 1 : 0;
        }

        hlEventStream[0]->m_bar1->gettime(&current_time, 0);

        if(gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
        {
            for ( i=0; i<nChannels; i++ )
            {
                //here we need a callback
                printStatusLine
                (
                 last_time,
                 current_time,
                 hlEventStream[i]->m_channel_status,
                 last_events_received[i],
                 last_bytes_received[i]
                );

                last_bytes_received[i]  = hlEventStream[i]->m_channel_status->bytes_received;
                last_events_received[i] = hlEventStream[i]->m_channel_status->n_events;
            }
            last_time = current_time;
        }
    }

    timeval end_time;
    hlEventStream[0]->m_bar1->gettime(&end_time, 0);

    for ( i=0; i<nChannels; i++ )
    {
        printFinalStatusLine
        (
            hlEventStream[i]->m_channel_status,
            opts[i],
            start_time,
            end_time
        );

        // wait until EL_FIFO runs empty
        // wait for pending transfers to complete (dma_busy->0)

        // disable EBDM Engine
        hlEventStream[i]->m_channel->setEbdmEnable(0);

        // disable RBDM
        hlEventStream[i]->m_channel->setRbdmEnable(0);

        // reset DFIFO, disable DMA PKT
        hlEventStream[i]->m_channel->setDMAConfig(0X00000002);

        // clear reportbuffer
        memset(hlEventStream[i]->m_reportBuffer->getMem(), 0, hlEventStream[i]->m_reportBuffer->getMappingSize());

        /** Cleanup */
        delete hlEventStream[i];
    }

    delete sm;
    delete bar;
    delete dev;

    return result;
}
