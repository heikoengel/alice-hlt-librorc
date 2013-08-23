/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-08-23
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
 * Open DMA Channel sourced by PatternGenerator
 *
 **/

#include <librorc.h>
#include <event_handling.h>

#include "dma_handling.hh"

using namespace std;

DMA_ABORT_HANDLER


int main(int argc, char *argv[])
{
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0]) &&
        checkEventSize(opts.eventSize, argv[0])
    ) )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    channelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    /** Create event stream */
    librorc::event_stream *eventStream = NULL;
    try
    { eventStream = new librorc::event_stream(opts.deviceId, opts.channelId); }
    catch( int error )
    {
        switch(error)
        {
            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED:
            { cout << "ERROR: failed to initialize device." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BAR_FAILED:
            { cout << "ERROR: failed to initialize BAR1." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BUFFER_FAILED:
            { cout << "ERROR: failed to allocate buffer." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DCHANNEL_FAILED:
            { cout << "ERROR: failed to allocate DMA-channel." << endl; }
            break;
        }
        exit(-1);
    }

    printDeviceStatus(eventStream);

    /** Setup DMA channel */
    try
    {
        eventStream->m_channel->enable();

        cout << "Waiting for GTX to be ready..." << endl;
        eventStream->m_channel->waitForGTXDomain();

        cout << "Configuring pattern generator ..." << endl;
        eventStream->m_channel->configurePatternGenerator(opts.eventSize);
    }
    catch( int error )
    {
        cout << "DMA channel failed (ERROR :" << error << ")" << endl;
        return(-1);
    }

    /** Capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;

    /** Event loop */
    int     result        = 0;
    int32_t sanity_checks = 0xff; /** no checks defaults */

    while( !done )
    {
        result = handle_channel_data
        (
            eventStream->m_reportBuffer,
            eventStream->m_eventBuffer,
            eventStream->m_channel,
            chstats,
            sanity_checks,              /** do sanity check    */
            NULL,                       /** no reference DDL   */
            0                           /** reference DDL size */
        );

        if( result < 0 )
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            abort();
        }
        else if( result==0 )
        { usleep(100); } /** no events available */

        eventStream->m_bar1->gettime(&cur_time, 0);

        last_time =
            printStatusLine
                (last_time, cur_time, chstats,
                    &last_events_received, &last_bytes_received);
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(chstats, opts, start_time, end_time);

    try
    { eventStream->m_channel->closePatternGenerator(); }
    catch(...)
    { cout << "Pattern generator was never configured !!!" << endl; }

    eventStream->m_channel->disable();

    /** Cleanup */
    shmdt(chstats);
    delete eventStream;

    return result;
}
