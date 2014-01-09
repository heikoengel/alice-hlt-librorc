/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.1
 * @date 2013-06-20
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


#include <librorc.h>
#include "dma_handling.hh"
#include "event_handling.h"
#include "event_generation.h"

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

    struct stat ddlstat;
    if( opts.useRefFile )
    {
        if ( stat(opts.refname, &ddlstat) == -1 )
        {
            perror("stat ddlfile");
            exit(-1);
        }
    }
    else
    {
        if ( !checkEventSize(opts.eventSize, argv[0]) )
        { exit(-1); }
    }

    DMA_ABORT_HANDLER_REGISTER

    librorc::event_stream *eventStream
        = prepareEventStream(opts);
    if( !eventStream )
    { exit(-1); }


    printf("EventBuffer size: 0x%lx bytes\n", EBUFSIZE);
    printf("ReportBuffer size: 0x%lx bytes\n", RBUFSIZE);

    eventStream->printDeviceStatus();

    // check if firmware is HLT_OUT
    if ( (eventStream->m_bar1->get32(RORC_REG_TYPE_CHANNELS)>>16) != RORC_CFG_PROJECT_hlt_out )
    {
        cout << "Firmware is not HLT_OUT - exiting." << endl;
        abort();
    }

    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;

    int32_t sanity_checks = CHK_SIZES|CHK_SOE;
    if(opts.useRefFile)
    { sanity_checks |= CHK_FILE; }
    else
    { sanity_checks |= CHK_PATTERN | CHK_ID; }


    event_generator
    eventGen
    (
        eventStream->m_reportBuffer,
        eventStream->m_eventBuffer,
        eventStream->m_channel
    );

    // wait for RB entry
    int result = 0;
    uint64_t number_of_events;
    while(!done)
    {
        number_of_events = eventGen.fillEventBuffer(opts.eventSize);

        if( number_of_events > 0 )
        { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", number_of_events); }

        result =
            handle_channel_data
            (
                eventStream->m_reportBuffer,
                eventStream->m_eventBuffer,
                eventStream->m_channel,
                eventStream->m_channel_status,
                sanity_checks, // do sanity check
                NULL, // no DDL reference file
                0 //DDL reference size
             );

        if (result<0)
        {
            printf("handle_channel_data failed for channel %d\n",
                    opts.channelId);
        }
        else if(result==0)
        { usleep(100); }

        eventStream->m_bar1->gettime(&cur_time, 0);

        // print status line each second
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL) {
            printf("Events OUT: %10ld, Size: %8.3f GB",
                    eventStream->m_channel_status->n_events,
                    (double)eventStream->m_channel_status->bytes_received/(double)(1<<30));

            if(eventStream->m_channel_status->bytes_received-last_bytes_received)
            {
                printf(" Rate: %9.3f MB/s",
                        (double)(eventStream->m_channel_status->bytes_received-last_bytes_received)/
                        gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
            } else {
                printf(" Rate: -");
            }

            if(eventStream->m_channel_status->n_events - last_events_received)
            {
                printf(" (%.3f kHz)",
                        (double)(eventStream->m_channel_status->n_events-last_events_received)/
                        gettimeofday_diff(last_time, cur_time)/1000.0);
            } else {
                printf(" ( - )");
            }
            printf(" Errors: %ld\n", eventStream->m_channel_status->error_count);
            last_time = cur_time;
            last_bytes_received  = eventStream->m_channel_status->bytes_received;
            last_events_received = eventStream->m_channel_status->n_events;
        }

    }

    // EOR
    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    // print summary
    printf("%ld Byte / %ld events in %.2f sec"
            "-> %.1f MB/s.\n",
            (eventStream->m_channel_status->bytes_received), eventStream->m_channel_status->n_events,
            gettimeofday_diff(start_time, end_time),
            ((float)eventStream->m_channel_status->bytes_received/
             gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

    if(!eventStream->m_channel_status->set_offset_count) //avoid DivByZero Exception
        printf("CH%d: No Events\n", opts.channelId);
    else
        printf("CH%d: Events %ld, max_epi=%ld, min_epi=%ld, "
                "avg_epi=%ld, set_offset_count=%ld\n", opts.channelId,
                eventStream->m_channel_status->n_events,
                eventStream->m_channel_status->max_epi,
                eventStream->m_channel_status->min_epi,
                eventStream->m_channel_status->n_events/eventStream->m_channel_status->set_offset_count,
                eventStream->m_channel_status->set_offset_count);

    // wait until EL_FIFO runs empty
    // TODO: add timeout
    while( eventStream->m_channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO) & 0xffff )
    { usleep(100); }

    // wait for pending transfers to complete (dma_busy->0)
    // TODO: add timeout
    while( eventStream->m_channel->getDMABusy() )
    { usleep(100); }

    // disable EBDM Engine
    eventStream->m_channel->disableEventBuffer();

    // disable RBDM
    eventStream->m_channel->disableReportBuffer();

    // reset DFIFO, disable DMA PKT
    eventStream->m_channel->setDMAConfig(0X00000002);

    // clear reportbuffer
    memset(eventStream->m_reportBuffer->getMem(), 0, eventStream->m_reportBuffer->getMappingSize());


    if(eventStream)
    {
        delete eventStream;
    }

    return result;
}
