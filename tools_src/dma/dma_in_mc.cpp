/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-11-05
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
#ifdef SIM
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
