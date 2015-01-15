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

#define MAX_CHANNELS 6

bool done = false;
void abort_handler( int s )
{
    printf("Caught signal %d\n", s);
    if( done==true )
    { exit(-1); }
    else
    { done = true; }
}



int main(int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])    &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }


    DMA_ABORT_HANDLER_REGISTER

    librorc::event_callback event_callback = eventCallBack;
    librorc::device *dev;
    librorc::bar *bar;

    try
    {
        dev = new librorc::device(opts.deviceId);
        bar = new librorc::bar(dev, 1);

    }
    catch(...)
    {
        cout << "Failed to initialize dev and bar" << endl;
        abort();
    }

    uint64_t last_bytes_received[MAX_CHANNELS];
    uint64_t last_events_received[MAX_CHANNELS];

    librorc::high_level_event_stream *hlEventStream[MAX_CHANNELS];
    for(int i=0; i < MAX_CHANNELS; i++) {
        // reset Data Replay
        opts.channelId = i;
        hlEventStream[i] = prepareEventStream(dev, bar, opts);
        if( !hlEventStream[i] )
        { exit(-1); }

        hlEventStream[i]->setEventCallback(event_callback);

        last_bytes_received[i] = 0;
        last_events_received[i] = 0;

        configureDataSource(hlEventStream[i], opts);
    }

    /** make clear what will be checked*/
    //int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE;
    int32_t sanity_check_mask = CHK_SIZES|CHK_EOE|CHK_PATTERN;
    /*if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else if ( opts.datasource == ES_SRC_HWPG )
    { sanity_check_mask |= (CHK_PATTERN|CHK_ID|CHK_SOE); }
    else if ( opts.datasource == ES_SRC_DIU)
    { sanity_check_mask |= (CHK_DIU_ERR); }*/


    librorc::event_sanity_checker checker[MAX_CHANNELS];
    for(int i=0; i < MAX_CHANNELS; i++) {
        checker[i] =librorc::event_sanity_checker
            (
             hlEventStream[i]->m_eventBuffer,
             i,
             sanity_check_mask,
             logdirectory
            );
    }

    /** Create event stream */
    uint64_t result = 0;

    /** Capture starting time */
    timeval start_time;
    hlEventStream[0]->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;


    /** Event loop */
    while( !done )
    {
        for(int i=0; i < MAX_CHANNELS; i++) {
            result = hlEventStream[i]->handleChannelData( (void*)&(checker[i]) );
        }
        hlEventStream[0]->m_bar1->gettime(&current_time, 0);

        if(librorc::gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
        {
            for(int i=0; i < MAX_CHANNELS; i++) {
                //here we need a callback
                printStatusLine(last_time, current_time,
                        hlEventStream[i]->m_channel_status,
                        last_events_received[i], last_bytes_received[i]);

                last_bytes_received[i]  = hlEventStream[i]->m_channel_status->bytes_received;
                last_events_received[i] = hlEventStream[i]->m_channel_status->n_events;
            }
            last_time = current_time;
        }
    }

    timeval end_time;
    hlEventStream[0]->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine
    (
        hlEventStream[0]->m_channel_status,
        opts,
        start_time,
        end_time
    );

    /** Cleanup */
    for(int i=0; i < MAX_CHANNELS; i++) {
        opts.channelId = i;
        printFinalStatusLine
            (
             hlEventStream[i]->m_channel_status,
             opts,
             start_time,
             end_time
            );
        unconfigureDataSource(hlEventStream[i], opts);
        delete hlEventStream[i];
    }
    delete bar;
    delete dev;
    return result;
}
