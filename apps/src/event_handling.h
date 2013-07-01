#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

/** limit the number of corrupted events to be written to disk **/
#define MAX_FILES_TO_DISK 100

/** Shared mem key offset **/
#define SHM_KEY_OFFSET 2048

/** Pattern Generator Mode: Ramp **/
#define PG_PATTERN_RAMP (1<<0)

/** sanity checks **/
#define CHK_SIZES (1<<0)
#define CHK_PATTERN (1<<1)
#define CHK_SOE (1<<2)
#define CHK_EOE (1<<3)
#define CHK_ID (1<<4)
#define CHK_FILE (1<<8)

/** struct to store statistics on received data for a single channel **/
struct ch_stats
{
    uint64_t n_events;
    uint64_t bytes_received;
    uint64_t min_epi;
    uint64_t max_epi;
    uint64_t index;
    uint64_t set_offset_count;
    uint64_t error_count;
    int64_t  last_id;
    uint32_t channel;
};

#include "helper_functions.h"

/**
 * Sanity checks on received data
 * @param reportbuffer pointer to struct rorcfs_event_descriptor
 * @param eventbuffer pointer to eventbuffer
 * @param i current reportbuffer index
 * @param ch DMA channel number
 * @param pattern_mode pattern to check data against
 * @param check_mask mask of checks to be done on the recived data
 * @return (int64_t)(-1) or error, (int64_t)EventID on success
 **/
int event_sanity_check
(
    struct rorcfs_event_descriptor *reportbuffer,
    volatile uint32_t *eventbuffer,
    uint64_t i,
    uint32_t ch,
    int64_t last_id,
    uint32_t pattern_mode,
    uint32_t check_mask,
    uint32_t *ddlref,
    uint64_t ddlref_size
)
{
  uint64_t offset;
  uint32_t j;
  uint32_t *eb;
  int64_t EventID;

  // compare calculated and reported event sizes
  if ( (check_mask & CHK_SIZES) &&
      (reportbuffer->calc_event_size != reportbuffer->reported_event_size) ) {
#ifdef DEBUG
    printf("CH%2d ERROR: Event[%ld] sizes do not match: \n"
        "calculated: 0x%x, reported: 0x%x\n"
        "offset=0x%lx, rbdm_offset=0x%lx\n", ch, i,
        reportbuffer->calc_event_size,
        reportbuffer->reported_event_size,
        reportbuffer->offset,
        i*sizeof(struct rorcfs_event_descriptor) );
#endif
    return -1;
  }

  /// reportbuffer->offset is a multiple of bytes
  offset = reportbuffer->offset/4;
  eb = (uint32_t*)malloc(
      (reportbuffer->reported_event_size+4)*sizeof(uint32_t));
  if ( eb==NULL ) {
    perror("Malloc EB");
    return -1;
  }
  // local copy of the event.
  // TODO: replace this with something more meaningful
  memcpy(eb, (uint8_t *)eventbuffer + reportbuffer->offset,
      (reportbuffer->reported_event_size+4)*sizeof(uint32_t));

  // sanity check on SOF
  if( (check_mask & CHK_SOE) &&
      ((uint32_t)*(eb)!=0xffffffff) ) {
#ifdef DEBUG
    printf("ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
        "offset=%ld, rbdm_offset=%ld\n",
        i, (uint32_t)*(eb),
        reportbuffer->offset,
        i*sizeof(struct rorcfs_event_descriptor) );
    dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
    dump_rb(reportbuffer, i, ch);
#endif
    free(eb);
    return -1;
  }

  if ( check_mask & CHK_PATTERN )
  {
    switch (pattern_mode)
    {
      /* Data pattern is a ramp */
      case PG_PATTERN_RAMP:

        // check PG pattern
        for (j=8;j<reportbuffer->calc_event_size;j++) {
          if ( (uint32_t)*(eb + j) != j-8 ) {
#ifdef DEBUG
            printf("ERROR: Event[%ld][%d] expected %08x read %08x\n",
                i, j, j-8, (uint32_t)*(eventbuffer + offset + j));
            dump_event(eventbuffer, offset,
                reportbuffer->reported_event_size);
            dump_rb(reportbuffer, i, ch);
#endif
            free(eb);
            return -1;
          }
        }
        break;

      default:
        printf("ERROR: specified unknown pattern matching algorithm\n");
        return -1;
    }
  }


  // compare with reference DDL file
  if ( check_mask & CHK_FILE )
  {
    if ( ((uint64_t)reportbuffer->calc_event_size<<2) != ddlref_size )
    {
#ifdef DEBUG
        printf("ERROR: Eventsize %lx does not match "
            "reference DDL file size %lx\n",
            ((uint64_t)reportbuffer->calc_event_size<<2),
            ddlref_size);
        dump_event(eventbuffer, offset,
            reportbuffer->reported_event_size);
        dump_rb(reportbuffer, i, ch);
#endif
        free(eb);
        return -1;
    }



    for (j=0;j<reportbuffer->calc_event_size;j++) {
      if ( eb[j] != ddlref[j] ) {
#ifdef DEBUG
        printf("ERROR: Event[%ld][%d] expected %08x read %08x\n",
            i, j, ddlref[j], eb[j]);
        dump_event(eventbuffer, offset,
            reportbuffer->reported_event_size);
        dump_rb(reportbuffer, i, ch);
#endif
        free(eb);
        return -1;
      }
    }
  }




  if ( check_mask & CHK_EOE )
  {
    if (DMA_MODE==128)
    {
      /**
       * 128 bit DMA mode
       *
       * DMA data is written in multiples of 128 bit
       * if an EOE occured the EOE words starts at the next 128bit boundary
       * the EOE actually is also a 128bit word consisting of
       * {32bit calc_event_size, 32bit reported_event_size, 64bit dummy}
       **/

      // jump to EOE words
      j = reportbuffer->calc_event_size;
      j &= (uint32_t)0xfffffffc; // cut to 128bit boundary
      if ( reportbuffer->calc_event_size & 0x3 )
        j+=4;

      // first EOE DW is calculated event size
      if ( (uint32_t)*(eb + j) !=
          reportbuffer->calc_event_size ) {
#ifdef DEBUG
        printf("ERROR: could not find matching calculated event size "
            "at Event[%d], expected %08x found %08x\n",
            j, reportbuffer->calc_event_size,
            (uint32_t)*(eb + j) );
        dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
        dump_rb(reportbuffer, i, ch);
#endif
        free(eb);
        return -1;
      }

      // advance 1DW
      j++;

      // second EOE DW is reported event size
      if ( (uint32_t)*(eb + j) !=
          reportbuffer->reported_event_size ) {
#ifdef DEBUG
        printf("ERROR: could not find matching reported event size "
            "at Event[%d] expected %08x found %08x\n",
            j, reportbuffer->reported_event_size,
            (uint32_t)*(eb + j) );
        dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
        dump_rb(reportbuffer, i, ch);
#endif
        free(eb);
        return -1;
      }
    } // DMA_MODE

    else if (DMA_MODE==32)
    {
      /**
       * 32 bit DMA mode
       *
       * DMA data is written in multiples of 32 bit. A 32bit EOE word
       * is directly attached after the last event data word.
       * The EOE word contains the EOE status word received from the DIU
       **/

      if ( (uint32_t)*(eb + reportbuffer->calc_event_size) !=
          reportbuffer->reported_event_size ) {
#ifdef DEBUG
        printf("ERROR: could not find matching reported event size "
            "at Event[%d] expected %08x found %08x\n",
            j, reportbuffer->calc_event_size,
            (uint32_t)*(eb + j) );
        dump_event(eventbuffer, offset, reportbuffer->calc_event_size);
        dump_rb(reportbuffer, i, ch);
#endif
        free(eb);
        return -1;
      }

    } //DMA_MODE
  } // CHK_EOE


  // get EventID from CDH:
  // lower 12 bits in CHD[1][11:0]
  // upper 24 bits in CDH[2][23:0]
  EventID = (uint32_t)*(eventbuffer + offset + 2) & 0x00ffffff;
  EventID<<=12;
  EventID |= (uint32_t)*(eventbuffer + offset + 1) & 0x00000fff;

  // make sure EventIDs increment with each event.
  // missing EventIDs are an indication of lost event data
  if ( (check_mask & CHK_ID) &&
      last_id!=-1 &&
      (EventID & 0xfffffffff) !=
      ((last_id +1) & 0xfffffffff) ) {
#ifdef DEBUG
    printf("ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
        "current ID: %ld\n", ch, last_id, EventID);
    dump_event(eventbuffer, offset, reportbuffer->calc_event_size);
    dump_rb(reportbuffer, i, ch);
#endif
    free(eb);
    return -1;
  }

  free(eb);
  return EventID;
}




/**
 * handle incoming data
 *
 * check if there is a reportbuffer entry at the current polling index
 * if yes, handle all available reportbuffer entries
 * @param rbuf pointer to ReportBuffer's struct rorcfs_buffer
 * @param eventbuffer pointer to EventBuffer Memory
 * @param ch pointer to struct rorcfs_dma_channel
 * @param ch_stats pointer to channel stats struct
 * @param do_sanity_check mask of sanity checks to be done on the
 * received data. See CHK_* defines above.
 * @return number of events processed
 **/
int handle_channel_data
(
    librorc::buffer *rbuf,
    librorc::buffer *ebuf,
    librorc::rorcfs_dma_channel *channel,
    struct    ch_stats *stats,
    int       do_sanity_check,
    uint32_t *ddlref,
    uint64_t  ddlref_size
)
{
  uint64_t events_per_iteration = 0;
  int events_processed = 0;
  uint64_t eboffset = 0;
  uint64_t rboffset = 0;
  uint64_t starting_index, entrysize;
  struct rorcfs_event_descriptor rb;
  int64_t EventID;
  char    basedir[] = "/tmp";

  struct rorcfs_event_descriptor *reportbuffer =
    (struct rorcfs_event_descriptor *)rbuf->getMem();
  volatile uint32_t *eventbuffer = (uint32_t *)ebuf->getMem();

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
        EventID = event_sanity_check(
            &rb,
            eventbuffer,
            stats->index,
            stats->channel,
            stats->last_id,
            PG_PATTERN_RAMP,
            do_sanity_check,
            ddlref,
            ddlref_size);


        if ( EventID<0 ) {
          stats->error_count++;
          if (stats->error_count < MAX_FILES_TO_DISK)
            dump_to_file(
                basedir, // base dir
                stats->channel, // channel index
                stats->index, // reportbuffer index
                stats->error_count, // file index
                reportbuffer, // Report Buffer
                ebuf // Event Buffer
                );
#ifdef SIM
          return -1;
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
          sizeof(struct rorcfs_event_descriptor)) % rbuf->getPhysicalSize();

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
    entrysize = sizeof(struct rorcfs_event_descriptor);
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
    channel->setEBOffset(eboffset);
    channel->setRBOffset(rboffset);

#ifdef DEBUG
    printf("CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
        stats->channel, rboffset, eboffset);
#endif
  }

  return events_processed;
}

#endif
