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
#include "event_handling.h"

using namespace std;


int handle_channel_data
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    librorcChannelStatus *stats,
    int                   do_sanity_check,
    uint32_t             *ddlref,
    uint64_t              ddlref_size
);


DMA_ABORT_HANDLER



int
event_sanity_check
(
    volatile librorc_event_descriptor *reportbuffer,
    volatile uint32_t                 *eventbuffer,
             uint64_t                  report_buffer_index,
             uint32_t                  channel_id,
             int64_t                   last_id,
             uint32_t                  pattern_mode,
             uint32_t                  check_mask,
             uint32_t                 *ddl_reference,
             uint64_t                  ddl_reference_size,
             uint64_t                 *event_id
)
{

    event_sanity_checker
        checker(eventbuffer, channel_id, pattern_mode, check_mask,
                ddl_reference, ddl_reference_size);

    try
    {
        *event_id
            = checker.eventSanityCheck
              (
                  reportbuffer,
                  report_buffer_index,
                  last_id
              );
    }
    catch( int error )
    { return error; }

    return 0;
}



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

    librorcChannelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    DDLRefFile ddlref;
    if(opts.esType == LIBRORC_ES_DDL)
        { ddlref = getDDLReferenceFile(opts); }
    else if(opts.esType == LIBRORC_ES_PG)
    {
        ddlref.map  = NULL;
        ddlref.size = 0;
    }
    else
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
    int32_t sanity_checks = 0xff; /** no checks defaults */
    if(ddlref.map && ddlref.size)
        {sanity_checks = CHK_FILE;}
    else if(opts.esType == LIBRORC_ES_DDL)
        {sanity_checks = CHK_SIZES;}

    /** Event loop */
    while( !done )
    {
        result = handle_channel_data
        (
            eventStream->m_reportBuffer,
            eventStream->m_eventBuffer,
            eventStream->m_channel,
            chstats,
            sanity_checks,
            ddlref.map,
            ddlref.size
        );

        if(result < 0)
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            return result;
        }
//        else if(result==0)
//        { usleep(200); } /** no events available */

        eventStream->m_bar1->gettime(&cur_time, 0);

        last_time =
            printStatusLine
                (last_time, cur_time, chstats,
                    &last_events_received, &last_bytes_received);
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(chstats, opts, start_time, end_time);

    /** Cleanup */
    delete eventStream;
    deleteDDLReferenceFile(ddlref);
    shmdt(chstats);

    return result;
}


/** limit the number of corrupted events to be written to disk **/
#define MAX_FILES_TO_DISK 100

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
int
handle_channel_data
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    librorcChannelStatus *stats,
    int                   do_sanity_check,
    uint32_t             *ddlref,
    uint64_t              ddlref_size
)
{
    uint64_t events_per_iteration = 0;
    int      events_processed = 0;
    uint64_t eboffset = 0;
    uint64_t rboffset = 0;
    uint64_t starting_index, entrysize;
    librorc_event_descriptor rb;
    uint64_t EventID;
    char     basedir[] = "/tmp";
    int      retval;

    librorc_event_descriptor *reportbuffer =
            (librorc_event_descriptor *)(rbuf->getMem());
    volatile uint32_t *eventbuffer = (uint32_t *)ebuf->getMem();

    // new event received
    if( reportbuffer[stats->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        starting_index = stats->index;

        // handle all following entries
        while( reportbuffer[stats->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform validity tests on the received data (if enabled)
            if(do_sanity_check)
            {
                rb = reportbuffer[stats->index];
                retval =
                    event_sanity_check
                    (
                        &rb,
                        eventbuffer,
                        stats->index,
                        stats->channel,
                        stats->last_id,
                        PG_PATTERN_RAMP,
                        do_sanity_check,
                        ddlref,
                        ddlref_size,
                        &EventID
                    );


                if ( retval!=0 )
                {
                stats->error_count++;

                    if (stats->error_count < MAX_FILES_TO_DISK)
                    {
                        dump_to_file
                        (
                            basedir, // base dir
                            stats, // channel stats
                            EventID, // current EventID
                            stats->error_count, // file index
                            reportbuffer, // Report Buffer
                            ebuf, // Event Buffer
                            retval // Error flags
                        );
                    }
                }

                stats->last_id = EventID;
            }

//            #ifdef DEBUG
//            dump_rb(&reportbuffer[stats->index], stats->index, stats->channel);
//            #endif

            // increment the number of bytes received
            stats->bytes_received +=
                (reportbuffer[stats->index].calc_event_size<<2);

            // save new EBOffset
            eboffset = reportbuffer[stats->index].offset;

            // increment reportbuffer offset
            rboffset =
                ((stats->index)*sizeof(librorc_event_descriptor)) % (rbuf->getPhysicalSize());

            // wrap RB index if necessary
            if( stats->index < rbuf->getMaxRBEntries()-1 )
            {
                stats->index++;
            }
            else
            {
                stats->index=0;
            }

            //increment total number of events received
            stats->n_events++;

            //increment number of events processed in this while-loop
            events_per_iteration++;
        }

        // clear processed reportbuffer entries
        entrysize = sizeof(librorc_event_descriptor);
        memset(&reportbuffer[starting_index], 0, (events_per_iteration*entrysize) );


        // update min/max statistics on how many events have been received
        // in the above while-loop
        if(events_per_iteration > stats->max_epi)
        {
            stats->max_epi = events_per_iteration;
        }

        if(events_per_iteration < stats->min_epi)
        {
            stats->min_epi = events_per_iteration;
        }
        events_per_iteration = 0;
        stats->set_offset_count++;

        channel->setBufferOffsetsOnDevice(eboffset, rboffset);

        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
        "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
        stats->channel, rboffset, eboffset);
    }

    return events_processed;
}
