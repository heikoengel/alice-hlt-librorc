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
 *
 * @brief
 * Open a DMA Channel and read out data
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
    opts[0].esType = LIBRORC_ES_TO_HOST;

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

    try
    {
        dev = new librorc::device(opts[0].deviceId);
#ifdef MODELSIM
        bar = new librorc::sim_bar(dev, 1);
#else
        bar = new librorc::rorc_bar(dev, 1);
#endif

    }
    catch(...)
    {
        cout << "Failed to initialize dev and bar" << endl;
        abort();
    }

    uint64_t last_bytes_received[MAX_CHANNELS];
    uint64_t last_events_received[MAX_CHANNELS];

    librorc::event_stream *eventStream[MAX_CHANNELS];
    for ( i=0; i<nChannels; i++ )
    {
        cout << "Prepare ES " << i << endl;
        if( !(eventStream[i] = prepareEventStream(dev, bar, opts[i])) )
        { exit(-1); }
        eventStream[i]->setEventCallback(event_callback);

        last_bytes_received[i] = 0;
        last_events_received[i] = 0;
    }


    eventStream[0]->printDeviceStatus();


    /** make clear what will be checked*/
    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE;
    if(opts[0].useRefFile)
    {
        sanity_check_mask |= CHK_FILE;
    }
    else
    {
        sanity_check_mask |= CHK_PATTERN | CHK_ID;
    }

    cout << "Event Loop Start" << endl;

    librorc::event_sanity_checker checker[MAX_CHANNELS];
    for ( i=0; i<nChannels; i++ )
    {
        checker[i] =
        (opts[i].useRefFile) /** is DDL reference file enabled? */
        ?   librorc::event_sanity_checker
            (
                eventStream[i]->m_eventBuffer,
                opts[i].channelId,
                sanity_check_mask,
                logdirectory,
                opts[i].refname
            )
        :   librorc::event_sanity_checker
            (
                eventStream[i]->m_eventBuffer,
                opts[i].channelId,
                sanity_check_mask,
                logdirectory
            ) ;
    }


    /** Create event stream */
    uint64_t result = 0;

    /** Capture starting time */
    timeval start_time;
    eventStream[0]->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;


    /** Event loop */
    while( !done )
    {

        for( i=0; i<nChannels; i++ )
        {
            result = eventStream[i]->handleChannelData( (void*)&(checker[i]) );
        }

        eventStream[0]->m_bar1->gettime(&current_time, 0);

        if(gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
        {
            for( i=0; i<nChannels; i++ )
            {
                //here we need a callback
                printStatusLine
                    (
                     last_time,
                     current_time,
                     eventStream[i]->m_channel_status,
                     last_events_received[i],
                     last_bytes_received[i]
                    );

                last_bytes_received[i]  = eventStream[i]->m_channel_status->bytes_received;
                last_events_received[i] = eventStream[i]->m_channel_status->n_events;
            }
            last_time = current_time;
        }
    }

    timeval end_time;
    eventStream[0]->m_bar1->gettime(&end_time, 0);

    for( i=0; i<nChannels; i++ )
    {
        printFinalStatusLine
        (
            eventStream[i]->m_channel_status,
            opts[i],
            start_time,
            end_time
        );

        /** Cleanup */
        delete eventStream[i];
    }

    delete bar;
    delete dev;

    return result;
}
