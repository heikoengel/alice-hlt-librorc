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
    printf("calculated: 0x%x, reported: 0x%x\n", (reportbuffer->calc_event_size & 0x3fffffff),
            (reportbuffer->reported_event_size & 0x3fffffff));

    librorc::event_sanity_checker
        checker
        (
            eventbuffer,
            channel_id,
            pattern_mode,
            check_mask,
            ddl_reference,
            ddl_reference_size
        );

    try
    {
        *event_id
            = checker.check
              (
                  reportbuffer,
                  report_buffer_index,
                  last_id
              );
    }
    catch( int error )
    {
        printf("Error : %d\n", error);
        return error;
    }

    return 0;
}

void dump_event(
    volatile uint32_t *eventbuffer,
    uint64_t offset,
    uint64_t len)
{
#ifdef SIM
  uint64_t i = 0;
  for(i=0;i<len;i++) {
    printf("%03ld: %08x\n",
        i, (uint32_t)*(eventbuffer + offset +i));
  }
  if(len&0x01) {
    printf("%03ld: %08x (dummy)\n", i,
        (uint32_t)*(eventbuffer + offset + i));
    i++;
  }
  printf("%03ld: EOE reported_event_size: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
#if DMA_MODE==128
  i++;
  printf("%03ld: EOE calc_event_size: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
#endif
#endif
}

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

//int event_sanity_check
//(
//    librorc_event_descriptor *reportbuffer,
//    volatile uint32_t *eventbuffer,
//    uint64_t i,
//    uint32_t ch,
//    uint64_t last_id,
//    uint32_t pattern_mode,
//    uint32_t check_mask,
//    uint32_t *ddlref,
//    uint64_t ddlref_size,
//    uint64_t *event_id
//)
//{
//  uint64_t offset;
//  uint32_t j;
//  uint32_t *eb = NULL;
//  uint64_t cur_event_id;
//  int retval = 0;
//
//  /** upper two bits of the sizes are reserved for flags */
//  uint32_t reported_event_size = (reportbuffer->reported_event_size & 0x3fffffff);
//  uint32_t calc_event_size = (reportbuffer->calc_event_size & 0x3fffffff);
//
//  /** Bit31 of calc_event_size is read completion timeout flag */
//  uint32_t timeout_flag = (reportbuffer->calc_event_size>>31);
//
//  // compare calculated and reported event sizes
//  if (check_mask & CHK_SIZES)
//  {
//      if (timeout_flag)
//      {
//          DEBUG_PRINTF(PDADEBUG_ERROR,
//                  "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
//                  ch, i);
//          retval |= CHK_SIZES;
//      } else if (calc_event_size != reported_event_size)
//      {
//          DEBUG_PRINTF(PDADEBUG_ERROR,
//                  "CH%2d ERROR: Event[%ld] sizes do not match: \n"
//                  "calculated: 0x%x, reported: 0x%x\n"
//                  "offset=0x%lx, rbdm_offset=0x%lx\n", ch, i,
//                  calc_event_size,reported_event_size,
//                  reportbuffer->offset,
//                  i*sizeof(librorc_event_descriptor) );
//          retval |= CHK_SIZES;
//      }
//  }
//
//  /// reportbuffer->offset is a multiple of bytes
//  offset = reportbuffer->offset/4;
//  eb = (uint32_t*)malloc((reported_event_size+4)*sizeof(uint32_t));
//  if ( eb==NULL ) {
//    perror("Malloc EB");
//    return -1;
//  }
//  // local copy of the event.
//  // TODO: replace this with something more meaningful
//  memcpy(eb, (uint8_t *)eventbuffer + reportbuffer->offset,
//      (reported_event_size+4)*sizeof(uint32_t));
//
//  // sanity check on SOF
//  if( (check_mask & CHK_SOE) &&
//          ((uint32_t)*(eb)!=0xffffffff) ) {
//      DEBUG_PRINTF(PDADEBUG_ERROR,
//              "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
//              "offset=%ld, rbdm_offset=%ld\n",
//              i, (uint32_t)*(eb),
//              reportbuffer->offset,
//              i*sizeof(librorc_event_descriptor) );
//      dump_event(eventbuffer, offset, reported_event_size);
//      dump_rb(reportbuffer, i, ch);
//      retval |= CHK_SOE;
//  }
//
//  if ( check_mask & CHK_PATTERN )
//  {
//    switch (pattern_mode)
//    {
//      /* Data pattern is a ramp */
//      case PG_PATTERN_RAMP:
//
//        // check PG pattern
//        for (j=8;j<calc_event_size;j++) {
//          if ( (uint32_t)*(eb + j) != j-8 ) {
//              DEBUG_PRINTF(PDADEBUG_ERROR,
//                      "ERROR: Event[%ld][%d] expected %08x read %08x\n",
//                      i, j, j-8, (uint32_t)*(eventbuffer + offset + j));
//              dump_event(eventbuffer, offset, reported_event_size);
//              dump_rb(reportbuffer, i, ch);
//              retval |= CHK_PATTERN;
//          }
//        }
//        break;
//
//      default:
//        printf("ERROR: specified unknown pattern matching algorithm\n");
//        retval |= CHK_PATTERN;
//    }
//  }
//
//
//  // compare with reference DDL file
//  if ( check_mask & CHK_FILE )
//  {
//    if ( ((uint64_t)calc_event_size<<2) != ddlref_size )
//    {
//        DEBUG_PRINTF(PDADEBUG_ERROR,
//                "ERROR: Eventsize %lx does not match "
//                "reference DDL file size %lx\n",
//                ((uint64_t)calc_event_size<<2),
//                ddlref_size);
//        dump_event(eventbuffer, offset, reported_event_size);
//        dump_rb(reportbuffer, i, ch);
//        retval |= CHK_FILE;
//    }
//
//
//
//    for (j=0;j<calc_event_size;j++) {
//        if ( eb[j] != ddlref[j] ) {
//            DEBUG_PRINTF(PDADEBUG_ERROR,
//                    "ERROR: Event[%ld][%d] expected %08x read %08x\n",
//                    i, j, ddlref[j], eb[j]);
//            dump_event(eventbuffer, offset, reported_event_size);
//            dump_rb(reportbuffer, i, ch);
//            retval |= CHK_FILE;
//      }
//    }
//  }
//
//
//
//
//  if ( check_mask & CHK_EOE )
//  {
//      /**
//       * 32 bit DMA mode
//       *
//       * DMA data is written in multiples of 32 bit. A 32bit EOE word
//       * is directly attached after the last event data word.
//       * The EOE word contains the EOE status word received from the DIU
//       **/
//
//      if ( (uint32_t)*(eb + calc_event_size) != reported_event_size ) {
//          DEBUG_PRINTF(PDADEBUG_ERROR,
//                  "ERROR: could not find matching reported event size "
//                  "at Event[%d] expected %08x found %08x\n",
//                  j, calc_event_size,
//                  (uint32_t)*(eb + j) );
//          dump_event(eventbuffer, offset, calc_event_size);
//          dump_rb(reportbuffer, i, ch);
//          retval |= CHK_EOE;
//      }
//
//  } // CHK_EOE
//
//
//  // get EventID from CDH:
//  // lower 12 bits in CHD[1][11:0]
//  // upper 24 bits in CDH[2][23:0]
//  cur_event_id = (uint32_t)*(eventbuffer + offset + 2) & 0x00ffffff;
//  cur_event_id <<= 12;
//  cur_event_id |= (uint32_t)*(eventbuffer + offset + 1) & 0x00000fff;
//
//  // make sure EventIDs increment with each event.
//  // missing EventIDs are an indication of lost event data
//  if ( (check_mask & CHK_ID) &&
//          (cur_event_id & 0xfffffffff) !=
//          ((last_id+1) & 0xfffffffff) ) {
//      DEBUG_PRINTF(PDADEBUG_ERROR,
//              "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
//              "current ID: %ld\n", ch, last_id, cur_event_id);
//      dump_event(eventbuffer, offset, calc_event_size);
//      dump_rb(reportbuffer, i, ch);
//      retval |= CHK_ID;
//  }
//
//  /** return event ID to caller */
//  *event_id = cur_event_id;
//
//  if(eb)
//  {
//      free(eb);
//  }
//  return retval;
//}



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
    int32_t sanity_checks = 0xff; /** all checks defaults */
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
        else if(result==0)
        { usleep(200); } /** no events available */

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
    uint64_t EventID = 0;
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

