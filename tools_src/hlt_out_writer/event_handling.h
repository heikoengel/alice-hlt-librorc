#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

#include <librorc.h>
#include <pda.h>
#include "helper_functions.h"

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

int event_sanity_check
(
    librorc_event_descriptor *reportbuffer,
    volatile uint32_t *eventbuffer,
    uint64_t i,
    uint32_t ch,
    uint64_t last_id,
    uint32_t pattern_mode,
    uint32_t check_mask,
    uint32_t *ddlref,
    uint64_t ddlref_size,
    uint64_t *event_id
)
{
    librorc::event_sanity_checker
        checker
        (
            eventbuffer,
            ch,
            pattern_mode,
            check_mask,
            ddlref,
            ddlref_size
        );

    try
    { *event_id = checker.check(reportbuffer, i, last_id); }
    catch( int error )
    { return error; }

    return 0;
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
    librorcChannelStatus        *stats,
    int                   do_sanity_check,
    uint32_t             *ddlref,
    uint64_t              ddlref_size
)
{
  uint64_t events_per_iteration = 0;
  int events_processed = 0;
  uint64_t eboffset = 0;
  uint64_t rboffset = 0;
  uint64_t starting_index, entrysize;
  librorc_event_descriptor rb;
  uint64_t EventID;
  char    basedir[] = "/tmp";
  int retval;

  librorc_event_descriptor *reportbuffer =
    (librorc_event_descriptor *)(rbuf->getMem());
  volatile uint32_t *eventbuffer = (uint32_t *)(ebuf->getMem());

  // new event received
  if( reportbuffer[stats->index].calc_event_size!=0 ) {

    // capture index of the first found reportbuffer entry
    starting_index = stats->index;

    // handle all following entries
    while( reportbuffer[stats->index].calc_event_size!=0 ) {

      // increment number of events processed in this interation
      events_processed++;

      // perform validity tests on the received data (if enabled)
      if (do_sanity_check) {
        rb = reportbuffer[stats->index];
        retval = event_sanity_check(
            &rb,
            eventbuffer,
            stats->index,
            stats->channel,
            stats->last_id,
            PG_PATTERN_INC,
            do_sanity_check,
            ddlref,
            ddlref_size,
            &EventID);


        if ( retval!=0 ) {
          stats->error_count++;
          if (stats->error_count < MAX_FILES_TO_DISK)
            dump_to_file(
                basedir, // base dir
                stats, // channel stats
                EventID, // current EventID
                stats->error_count, // file index
                reportbuffer, // Report Buffer
                ebuf, // Event Buffer
                retval // Error flags
                );
#ifdef SIM
          //return -1;
#endif
        }

        stats->last_id = EventID;
      }

#ifdef DEBUG
      dump_rb(&reportbuffer[stats->index], stats->index, stats->channel);
#endif

      // increment the number of bytes received
      stats->bytes_received +=
        (reportbuffer[stats->index].calc_event_size<<2);

      // save new EBOffset
      eboffset = reportbuffer[stats->index].offset;

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
    memset(&reportbuffer[starting_index], 0,
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
