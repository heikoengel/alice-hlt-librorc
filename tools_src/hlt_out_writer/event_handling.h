#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

#include <librorc.h>
#include <pda.h>



/**
 * gettimeofday_diff
 * @param time1 earlier timestamp
 * @param time2 later timestamp
 * @return time difference in seconds as double
 * */
double gettimeofday_diff(timeval time1, timeval time2)
{
    timeval diff;
    diff.tv_sec = time2.tv_sec - time1.tv_sec;
    diff.tv_usec = time2.tv_usec - time1.tv_usec;
    while(diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec -= 1;
    }

    return (double)((double)diff.tv_sec +
        (double)((double)diff.tv_usec / 1000000));
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
int handle_channel_data
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    librorcChannelStatus *stats,
    int                   do_sanity_check,
    bool                  ddl_reference_is_enabled,
    char                 *ddl_path,
    librorc::event_sanity_checker *checker
)
{
    uint64_t events_per_iteration = 0;
    int      events_processed     = 0;
    uint64_t eboffset             = 0;
    uint64_t rboffset             = 0;
    uint64_t starting_index, entrysize;
    librorc_event_descriptor rb;
    uint64_t EventID = 0;

    librorc_event_descriptor *raw_report_buffer =
        (librorc_event_descriptor *)(rbuf->getMem());

    // new event received
    if( raw_report_buffer[stats->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        starting_index = stats->index;

        // handle all following entries
        while( raw_report_buffer[stats->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform validity tests on the received data (if enabled)
            if (do_sanity_check)
            {
                rb = raw_report_buffer[stats->index];

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
            eboffset = raw_report_buffer[stats->index].offset;

            // increment reportbuffer offset
            rboffset = ((stats->index)*
                sizeof(librorc_event_descriptor)) % rbuf->getPhysicalSize();

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
        entrysize = sizeof(librorc_event_descriptor);
        memset(&raw_report_buffer[starting_index], 0, events_per_iteration*entrysize);

        // update min/max statistics on how many events have been received
        // in the above while-loop
        if(events_per_iteration > stats->max_epi)
        { stats->max_epi = events_per_iteration; }

        if(events_per_iteration < stats->min_epi)
        { stats->min_epi = events_per_iteration; }

        events_per_iteration = 0;
        stats->set_offset_count++;

        channel->setBufferOffsetsOnDevice(eboffset, rboffset);

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
            stats->channel,
            rboffset,
            eboffset
        );
    }

    return events_processed;
}

#endif
