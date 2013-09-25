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



int
handle_channel_data
(
    librorc::event_stream *eventStream,
    librorc::event_sanity_checker *checker
);

int
eventLoop
(
    librorc::event_sanity_checker checker,
    librorc::event_stream* eventStream
);


DMA_ABORT_HANDLER



int main(int argc, char *argv[])
{
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])    &&
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

    librorc::event_stream *eventStream = NULL;
    if( !(eventStream = prepareEventStream(opts)) )
    { exit(-1); }

    eventStream->printDeviceStatus();

    /** make clear what will be checked*/
    int32_t sanity_check_mask = 0xff; /** all checks defaults */
    if(opts.esType == LIBRORC_ES_DDL)
    { sanity_check_mask = CHK_FILE | CHK_SIZES; }

    librorc::event_sanity_checker checker =
        (opts.esType==LIBRORC_ES_DDL) /** is DDL reference file enabled? */
        ?   librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC, /** TODO */
                sanity_check_mask,
                "/tmp",
                opts.refname
            )
        :   librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC,
                sanity_check_mask,
                "/tmp"
            ) ;

    int result = eventLoop(checker, eventStream);

    /** Cleanup */
    delete eventStream;

    return result;
}



int
eventLoop
(
    librorc::event_sanity_checker checker,
    librorc::event_stream* eventStream
)
{
    uint64_t m_last_bytes_received = 0;
    uint64_t m_last_events_received = 0;

    /** Capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    int result = 0;
    while( !done )
    {
        result = handle_channel_data(eventStream, &checker);
        if (result < 0)
        {
            printf("handle_channel_data failed for channel %d\n",
                    eventStream->m_channel_status->channel);
            return result;
        }
        else if (result == 0)
        { usleep(200); } /** no events available */

        eventStream->m_bar1->gettime(&cur_time, 0);
        last_time = printStatusLine(last_time, cur_time,
                eventStream->m_channel_status, &m_last_events_received,
                &m_last_bytes_received);
    }
    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);
    printFinalStatusLine(eventStream->m_channel_status, start_time, end_time);
    return result;
}




int handle_channel_data
(
    librorc::event_stream         *eventStream,
    librorc::event_sanity_checker *checker
)
{
    librorcChannelStatus *m_channel_status = eventStream->m_channel_status;
    librorc::buffer      *m_reportBuffer   = eventStream->m_reportBuffer;
    librorc::dma_channel *m_channel        = eventStream->m_channel;


    librorc_event_descriptor *reports
        = (librorc_event_descriptor *)(m_reportBuffer->getMem());
    int events_processed = 0;
    /** new event received */
    if( reports[m_channel_status->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        uint64_t starting_index       = m_channel_status->index;
        uint64_t events_per_iteration = 0;
        uint64_t event_buffer_offset  = 0;
        uint64_t report_buffer_offset = 0;

        // handle all following entries
        while( reports[m_channel_status->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform selected validity tests on the received data
            // dump stuff if errors happen
            //___THIS_IS_CALLBACK_CODE__//
            uint64_t event_id = 0;
            try
            { event_id = checker->check(reports, m_channel_status); }
            catch(...){ abort(); }
            //___THIS_IS_CALLBACK_CODE__//

            m_channel_status->last_id = event_id;

            // increment the number of bytes received
            m_channel_status->bytes_received +=
                (reports[m_channel_status->index].calc_event_size<<2);

            // save new EBOffset
            event_buffer_offset = reports[m_channel_status->index].offset;

            // increment reportbuffer offset
            report_buffer_offset
                = ((m_channel_status->index)*sizeof(librorc_event_descriptor))
                % m_reportBuffer->getPhysicalSize();

            // wrap RB index if necessary
            m_channel_status->index
                = (m_channel_status->index < m_reportBuffer->getMaxRBEntries()-1)
                ? (m_channel_status->index+1) : 0;

            //increment total number of events received
            m_channel_status->n_events++;

            //increment number of events processed in this while-loop
            events_per_iteration++;
        }

        // clear processed reportbuffer entries
        memset(&reports[starting_index], 0, events_per_iteration*sizeof(librorc_event_descriptor) );


        // update min/max statistics on how many events have been received
        // in the above while-loop
        if(events_per_iteration > m_channel_status->max_epi)
        { m_channel_status->max_epi = events_per_iteration; }

        if(events_per_iteration < m_channel_status->min_epi)
        { m_channel_status->min_epi = events_per_iteration; }

        events_per_iteration = 0;
        m_channel_status->set_offset_count++;

        // actually update the offset pointers in the firmware
        m_channel->setBufferOffsetsOnDevice(event_buffer_offset, report_buffer_offset);

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
            m_channel_status->channel,
            report_buffer_offset,
            event_buffer_offset
        );
    }

    return events_processed;
}





