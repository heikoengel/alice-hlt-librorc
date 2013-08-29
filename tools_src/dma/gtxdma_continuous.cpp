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
 * Open DMA Channel sourced by optical DDL
 *
 * */

#include <librorc.h>

#include "event_handling.h"
#include "dma_handling.hh"

using namespace std;

DMA_ABORT_HANDLER


int main( int argc, char *argv[])
{
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    librorcChannelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    DDLRefFile ddlref = getDDLReferenceFile(opts);

    /** Create event stream */
    librorc::event_stream *eventStream = NULL;
    if( !(eventStream = prepareEventStream(opts)) )
    { exit(-1); }

    printDeviceStatus(eventStream);

    /** Setup DMA channel */
    try
    {
        eventStream->m_channel->enable();

        cout << "Waiting for GTX to be ready..." << endl;
        eventStream->m_channel->waitForGTXDomain();

        cout << "Configuring GTX ..." << endl;
        eventStream->m_channel->configureGTX();
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
    if(ddlref.map && ddlref.size) {sanity_checks = CHK_FILE;}
    else {sanity_checks = CHK_SIZES;}
    while( !done )
    {
        result = handle_channel_data
        (
            eventStream->m_reportBuffer,
            eventStream->m_eventBuffer,
            eventStream->m_channel,
            chstats,
            sanity_checks,              /** do sanity check    */
            ddlref.map,
            ddlref.size
        );

        if(result < 0)
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            return result;
        }
        else if(result==0)
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
    { eventStream->m_channel->closeGTX(); }
    catch(...)
    { cout << "Link is down - unable to send EOBTR !!!" << endl; }

    eventStream->m_channel->disable();

    /** Cleanup */
    deleteDDLReferenceFile(ddlref);
    shmdt(chstats);
    delete eventStream;

    return 0;
}
