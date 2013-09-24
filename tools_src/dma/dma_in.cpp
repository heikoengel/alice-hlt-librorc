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
 * Open a DMA Channel and read out data
 *
 **/

#include <librorc.h>

#include "dma_handling.hh"

using namespace std;



int handle_channel_data
(
    librorc::event_stream *eventStream,
    librorcChannelStatus  *channel_status,
    int                    sanity_check_mask,
    bool                   ddl_reference_is_enabled,
    char                  *ddl_path
);



DMA_ABORT_HANDLER



int main(int argc, char *argv[])
{
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    if
    (
        !checkEventSize(opts.eventSize, argv[0]) &&
        (opts.esType == LIBRORC_ES_PG)
    )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    librorcChannelStatus *channel_status
        = prepareSharedMemory(opts);
    if(channel_status == NULL)
    { exit(-1); }

    /** Create event stream */
    librorc::event_stream *eventStream = NULL;
    if( !(eventStream = prepareEventStream(opts)) )
    { exit(-1); }

    eventStream->printDeviceStatus();

    /** Capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;

    /** make clear what will be checked*/
    int     result        = 0;
    int32_t sanity_checks = 0xff; /** all checks defaults */
    if(opts.esType == LIBRORC_ES_DDL)
    { sanity_checks = CHK_FILE | CHK_SIZES; }

    /** Event loop */
    while( !done )
    {
        result = handle_channel_data
        (
            eventStream,
            channel_status,
            sanity_checks,
            (opts.esType==LIBRORC_ES_DDL),
            opts.refname
        );

        if(result < 0)
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            return result;
        }
        else if(result==0)
        { usleep(200); } /** no events available */

        eventStream->m_bar1->gettime(&cur_time, 0);

        last_time =
            printStatusLine
                (last_time, cur_time, channel_status,
                    &last_events_received, &last_bytes_received);
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(channel_status, opts, start_time, end_time);

    /** Cleanup */
    delete eventStream;
    shmdt(channel_status);

    return result;
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
//TODO: refactor this into a class and merge it with event stream afterwards
int handle_channel_data
(
    librorc::event_stream *eventStream,
    librorcChannelStatus  *channel_status,
    int                    sanity_check_mask,//checker does not change
    bool                   ddl_reference_is_enabled,//checker does not change
    char                  *ddl_path //checker does not change
)
{

    librorc::buffer      *report_buffer = eventStream->m_reportBuffer;
    librorc::buffer      *event_buffer  = eventStream->m_eventBuffer;
    librorc::dma_channel *channel       = eventStream->m_channel;

    uint64_t    event_id             = 0;
    char        log_directory_path[] = "/tmp";

    librorc_event_descriptor *reports
        = (librorc_event_descriptor *)(report_buffer->getMem());

    librorc::event_sanity_checker checker =
        ddl_reference_is_enabled
        ?
            librorc::event_sanity_checker
            (
                event_buffer,
                channel_status->channel,
                PG_PATTERN_INC, /** TODO */
                sanity_check_mask,
                log_directory_path,
                ddl_path
            )
        :
            librorc::event_sanity_checker
            (
                event_buffer,
                channel_status->channel,
                PG_PATTERN_INC,
                sanity_check_mask,
                log_directory_path
            )
        ;


    int events_processed = 0;
    /** new event received */
    if( reports[channel_status->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        uint64_t starting_index       = channel_status->index;
        uint64_t events_per_iteration = 0;
        uint64_t event_buffer_offset  = 0;
        uint64_t report_buffer_offset = 0;

        // handle all following entries
        while( reports[channel_status->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform selected validity tests on the received data
            // dump stuff if errors happen
            try
            { event_id = checker.check(reports, channel_status); }
            catch(...){ abort(); }

            channel_status->last_id = event_id;

            // increment the number of bytes received
            channel_status->bytes_received +=
                (reports[channel_status->index].calc_event_size<<2);

            // save new EBOffset
            event_buffer_offset = reports[channel_status->index].offset;

            // increment reportbuffer offset
            report_buffer_offset = ((channel_status->index)*sizeof(librorc_event_descriptor)) % report_buffer->getPhysicalSize();

            // wrap RB index if necessary
            channel_status->index
                = (channel_status->index < report_buffer->getMaxRBEntries()-1)
                ? (channel_status->index+1) : 0;

            //increment total number of events received
            channel_status->n_events++;

            //increment number of events processed in this while-loop
            events_per_iteration++;
        }

        // clear processed reportbuffer entries
        memset(&reports[starting_index], 0, events_per_iteration*sizeof(librorc_event_descriptor) );


        // update min/max statistics on how many events have been received
        // in the above while-loop
        if(events_per_iteration > channel_status->max_epi)
        { channel_status->max_epi = events_per_iteration; }

        if(events_per_iteration < channel_status->min_epi)
        { channel_status->min_epi = events_per_iteration; }

        events_per_iteration = 0;
        channel_status->set_offset_count++;

        // actually update the offset pointers in the firmware
        channel->setBufferOffsetsOnDevice(event_buffer_offset, report_buffer_offset);

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
            channel_status->channel,
            report_buffer_offset,
            event_buffer_offset
        );
    }

    return events_processed;
}





