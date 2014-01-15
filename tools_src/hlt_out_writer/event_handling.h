#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

#include <librorc.h>
#include <pda.h>



/**
 * Dump reportbuffer entry
 * @param reportbuffer pointer to reportbuffer
 * @param i index of current librorc_event_descriptor within
 * reportbuffer
 * @param ch DMA channel number
 * */
void
dump_rb
(
    librorc_event_descriptor *reportbuffer,
    uint64_t i,
    uint32_t ch
)
{
    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
            "CH%2d - RB[%3ld]: calc_size=%08x\t"
            "reported_size=%08x\t"
            "offset=%lx\n",
            ch, i, reportbuffer->calc_event_size,
            reportbuffer->reported_event_size,
            reportbuffer->offset);
}

/**
 * gettimeofday_diff
 * @param time1 earlier timestamp
 * @param time2 later timestamp
 * @return time difference in seconds as double
 * */
double gettimeofday_diff(timeval time1, timeval time2) {
  timeval diff;
  diff.tv_sec = time2.tv_sec - time1.tv_sec;
  diff.tv_usec = time2.tv_usec - time1.tv_usec;
  while(diff.tv_usec < 0) {
    diff.tv_usec += 1000000;
    diff.tv_sec -= 1;
  }

  return (double)((double)diff.tv_sec +
      (double)((double)diff.tv_usec / 1000000));
}

/**
 * Sanity checks on received data
 * @param reportbuffer pointer to librorc_event_descriptor
 * @param eventbuffer pointer to eventbuffer
 * @param i current reportbuffer index
 * @param ch DMA channel number
 * @param pattern_mode pattern to check data against
 * @param check_mask mask of checks to be done on the recived data
 * @param event_id pointer to uint64_t, used to return event ID
 * @return !=0 on error, 0 on sucess
 **/

void
event_sanity_check
(
    librorcChannelStatus     *channel_status,
    librorc_event_descriptor *report_buffer,
    librorc::buffer          *event_buffer,
    uint32_t                  pattern_mode,
    uint32_t                       sanity_check_mask,
    uint64_t                      *event_id,
    bool                           ddl_reference_is_enabled,
    char                          *ddl_path,
    librorc::event_sanity_checker *checker
)
{
    //char log_directory_path[] = "/tmp";

    try
    {
//        librorc::event_sanity_checker checker =
//            ddl_reference_is_enabled
//            ?
//                librorc::event_sanity_checker
//                (
//                    event_buffer,
//                    channel_status->channel,
//                    pattern_mode,
//                    sanity_check_mask,
//                    log_directory_path,
//                    ddl_path
//                )
//            :
//                librorc::event_sanity_checker
//                (
//                    event_buffer,
//                    channel_status->channel,
//                    pattern_mode,
//                    sanity_check_mask,
//                    log_directory_path
//                )
//            ;

        *event_id = checker->check(report_buffer, channel_status);
    }
    catch( int error )
    { abort(); }
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
  int events_processed = 0;
  uint64_t eboffset = 0;
  uint64_t rboffset = 0;
  uint64_t starting_index, entrysize;
  librorc_event_descriptor rb;
  uint64_t EventID = 0;

  librorc_event_descriptor *raw_report_buffer =
    (librorc_event_descriptor *)(rbuf->getMem());

  // new event received
  if( raw_report_buffer[stats->index].calc_event_size!=0 ) {

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
        event_sanity_check
        (
            stats,
            raw_report_buffer,
            ebuf,
            PG_PATTERN_INC,
            do_sanity_check,
            &EventID,
            ddl_reference_is_enabled,
            ddl_path,
            checker
        );

        stats->last_id = EventID;
      }

#ifdef DEBUG
      dump_rb(&raw_report_buffer[stats->index], stats->index, stats->channel);
#endif

      // increment the number of bytes received
      stats->bytes_received +=
        (raw_report_buffer[stats->index].calc_event_size<<2);

      // save new EBOffset
      eboffset = raw_report_buffer[stats->index].offset;

      // increment reportbuffer offset
      rboffset = ((stats->index)*
          sizeof(librorc_event_descriptor)) % rbuf->getPhysicalSize();

      // wrap RB index if necessary
      if( stats->index < rbuf->getMaxRBEntries()-1 )
        stats->index++;
      else
        stats->index=0;

      //increment total number of events received
      stats->n_events++;

      //increment number of events processed in this while-loop
      events_per_iteration++;
    }

    // clear processed reportbuffer entries
    entrysize = sizeof(librorc_event_descriptor);
    memset(&raw_report_buffer[starting_index], 0,
        events_per_iteration*entrysize);


    // update min/max statistics on how many events have been received
    // in the above while-loop
    if(events_per_iteration > stats->max_epi)
      stats->max_epi = events_per_iteration;
    if(events_per_iteration < stats->min_epi)
      stats->min_epi = events_per_iteration;
    events_per_iteration = 0;
    stats->set_offset_count++;

    // actually update the offset pointers in the firmware
    //channel->setEBOffset(eboffset);
    //channel->setRBOffset(rboffset);

    channel->setBufferOffsetsOnDevice(eboffset, rboffset);

    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
    "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
        stats->channel, rboffset, eboffset);
  }

  return events_processed;
}

#endif
