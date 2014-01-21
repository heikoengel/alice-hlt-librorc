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
#include <event_generation.h>
#include "dma_handling.hh"


using namespace std;

uint64_t
handle_channel_data
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    librorcChannelStatus *stats,
    int                   do_sanity_check,
    bool                  ddl_reference_is_enabled,
    char                 *ddl_path,
    librorc::event_sanity_checker *checker
);


DMA_ABORT_HANDLER

void
printSummary
(
    const timeval         start_time,
    const timeval         end_time,
    librorcChannelStatus *channel_status,
    DMAOptions opts
)
{
    printf
    (
        "%ld Byte / %ld events in %.2f sec -> %.1f MB/s.\n",
        channel_status->bytes_received,
        channel_status->n_events,
        gettimeofdayDiff(start_time, end_time),
        ((float) (channel_status->bytes_received) / gettimeofdayDiff(start_time, end_time)) /
        (float) ((1 << 20))
    );

    if(!channel_status->set_offset_count)
    { printf("CH%d: No Events\n", opts.channelId); }
    else
    {
        printf
        (
            "CH%d: Events %ld, max_epi=%ld, min_epi=%ld, avg_epi=%ld, set_offset_count=%ld\n",
            opts.channelId,
            channel_status->n_events,
            channel_status->max_epi,
            channel_status->min_epi,
            (channel_status->n_events / channel_status->set_offset_count),
            channel_status->set_offset_count
        );
    }
}

int main( int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
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

    eventStream = prepareEventStream(opts);
    if( !eventStream )
    { exit(-1); }

    eventStream->printDeviceStatus();

    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE;
    if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else
    { sanity_check_mask |= CHK_PATTERN | CHK_ID; }


    event_generator
    eventGen
    (
        eventStream->m_reportBuffer,
        eventStream->m_eventBuffer,
        eventStream->m_channel
    );

    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    librorc::event_sanity_checker checker =
        (opts.useRefFile)
        ?
            librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC,
                sanity_check_mask,
                logdirectory,
                opts.refname
            )
        :
            librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC,
                sanity_check_mask,
                logdirectory
            )
        ;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;
    uint64_t number_of_events     = 0;
    /** wait for RB entry */
    while(!eventStream->m_done)
    {
        number_of_events = eventGen.fillEventBuffer(opts.eventSize);

        if( number_of_events > 0 )
        { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", number_of_events); }

        if
        (
            handle_channel_data
            (
                eventStream->m_reportBuffer,
                eventStream->m_eventBuffer,
                eventStream->m_channel,
                eventStream->m_channel_status,
                sanity_check_mask, // do sanity check
                NULL, // no DDL reference file
                0, //DDL reference size
                &checker
             ) == 0
        )
        { usleep(100); }

        eventStream->m_bar1->gettime(&cur_time, 0);

        // print status line each second
        if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
        {
            printf("Events OUT: %10ld, Size: %8.3f GB",
                    eventStream->m_channel_status->n_events,
                    (double)eventStream->m_channel_status->bytes_received/(double)(1<<30));

            if(eventStream->m_channel_status->bytes_received-last_bytes_received)
            {
                printf(" Rate: %9.3f MB/s",
                        (double)(eventStream->m_channel_status->bytes_received-last_bytes_received)/
                        gettimeofdayDiff(last_time, cur_time)/(double)(1<<20));
            }
            else
            { printf(" Rate: -"); }

            if(eventStream->m_channel_status->n_events - last_events_received)
            {
                printf(" (%.3f kHz)",
                        (double)(eventStream->m_channel_status->n_events-last_events_received)/
                        gettimeofdayDiff(last_time, cur_time)/1000.0);
            }
            else
            { printf(" ( - )"); }

            printf(" Errors: %ld\n", eventStream->m_channel_status->error_count);
            last_time = cur_time;
            last_bytes_received  = eventStream->m_channel_status->bytes_received;
            last_events_received = eventStream->m_channel_status->n_events;
        }

    }

    // EOR
    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printSummary(start_time, end_time, eventStream->m_channel_status, opts);

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

    delete eventStream;

    return 1;
}



/**
 * handle incoming data
 *
 * check if there is a reportbuffer entry at the current polling index
 * if yes, handle all available reportbuffer entries
 * @param rbuf pointer to ReportBuffer
 * @param eventbuffer pointer to EventBuffer Memory
 * @param channel pointer
 * @param ch_stats pointer to channel stats struct
 * @param do_sanity_check mask of sanity checks to be done on the
 * received data. See CHK_* defines above.
 * @return number of events processed
 **/
uint64_t
handle_channel_data
(
    librorc::buffer               *rbuf,
    librorc::buffer               *ebuf,
    librorc::dma_channel          *channel,
    librorcChannelStatus          *stats,
    int                            do_sanity_check,
    bool                           ddl_reference_is_enabled,
    char                          *ddl_path,
    librorc::event_sanity_checker *checker
)
{
    librorc_event_descriptor *raw_report_buffer =
        (librorc_event_descriptor *)(rbuf->getMem());

    // new event received
    uint64_t events_processed = 0;
    if( raw_report_buffer[stats->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        uint64_t start_index = stats->index;

        // handle all following entries
        uint64_t events_per_iteration = 0;
        uint64_t report_buffer_offset = 0;
        uint64_t event_buffer_offset  = 0;
        while( raw_report_buffer[stats->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform validity tests on the received data (if enabled)
            if(do_sanity_check)
            {
                uint64_t EventID ;
                try
                { EventID = checker->check(raw_report_buffer, stats); }
                catch( int error )
                { abort(); }

                stats->last_id = EventID;
            }

            DEBUG_PRINTF
            (
                PDADEBUG_CONTROL_FLOW,
                "CH%2d - RB[%3ld]: calc_size=%08x\t"
                "reported_size=%08x\t"
                "offset=%lx\n",
                stats->channel,
                stats->index,
                raw_report_buffer[stats->index].calc_event_size,
                raw_report_buffer[stats->index].reported_event_size,
                raw_report_buffer[stats->index].offset
            );

            // increment the number of bytes received
            stats->bytes_received +=
                (raw_report_buffer[stats->index].calc_event_size<<2);

            // save new EBOffset
            event_buffer_offset = raw_report_buffer[stats->index].offset;

            // increment reportbuffer offset
            report_buffer_offset =
                    ((stats->index) * sizeof(librorc_event_descriptor)) % rbuf->getPhysicalSize();

            // wrap RB index if necessary
            if( stats->index < (rbuf->getMaxRBEntries()-1) )
            { stats->index++; }
            else
            { stats->index=0; }

            //increment total number of events received
            stats->n_events++;

            //increment number of events processed in this while-loop
            events_per_iteration++;
        }

        // clear processed reportbuffer entries
        memset(&raw_report_buffer[start_index], 0, events_per_iteration*sizeof(librorc_event_descriptor));

        // update min/max statistics on how many events have been received
        // in the above while-loop
        if(events_per_iteration > stats->max_epi)
        { stats->max_epi = events_per_iteration; }

        if(events_per_iteration < stats->min_epi)
        { stats->min_epi = events_per_iteration; }

        events_per_iteration = 0;
        stats->set_offset_count++;

        channel->setBufferOffsetsOnDevice(event_buffer_offset, report_buffer_offset);

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
            stats->channel,
            report_buffer_offset,
            event_buffer_offset
        );
    }

    return events_processed;
}


