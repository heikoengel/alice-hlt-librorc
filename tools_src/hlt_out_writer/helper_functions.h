/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-03-18
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

#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <fcntl.h>
#include <pda.h>



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
 * TODO: dump to file?
 * Dump Event
 * @param eventbuffer pointer to eventbuffer
 * @param offset offset of the current event within the eventbuffer
 * @param len size in DWs of the event
 * */
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
 * ==========================================================
 * print summary statistics
 * ==========================================================
 * @param n number of channels
 * @param chstats pointer to struct ch_stats
 * @param ch_last_bytes_received pointer to array of bytescounts
 * fromt the last iteration
 * @param timediff time passed since the last iteration
 * */
void print_summary_stats
(
    uint32_t              n,
    librorcChannelStatus *chstats[],
    uint64_t             *ch_last_bytes_received,
    double                timediff
)
{
  librorcChannelStatus statsum;
  uint64_t last_bytes_received = 0;
  uint32_t i;
  //sum up all channels
  memset(&statsum, 0, sizeof(statsum));
  for(i=0;i<n;i++) {
    statsum.bytes_received += chstats[i]->bytes_received;
    statsum.error_count += chstats[i]->error_count;
    statsum.n_events += chstats[i]->n_events;
    statsum.set_offset_count += chstats[i]->set_offset_count;
    last_bytes_received += ch_last_bytes_received[i];
  }
  // summary stats
  printf("SUM:  Events: %10ld, DataSize: %8.3f GB ",
      statsum.n_events,
      (double)statsum.bytes_received/(double)(1<<30));

  if ( statsum.bytes_received-last_bytes_received)
  {
    printf(" DataRate: %9.3f MB/s",
        (double)(statsum.bytes_received-last_bytes_received)/
        timediff/(double)(1<<20));
  } else {
    printf(" DataRate: -");
  }
  printf(" Errors: %ld\n", statsum.error_count);
}


/**
 * ==========================================================
 * print channel statistics
 * ==========================================================
 * @param n number of channels
 * @param chstats pointer to struct ch_stats
 * @param ch_last_bytes_received pointer to array of bytescounts
 * fromt the last iteration
 * @param timediff time passed since the last iteration
 * */
void print_channel_stats
(
    uint32_t              n,
    librorcChannelStatus *chstats[],
    uint64_t             *ch_last_bytes_received,
    double                timediff
)
{
  librorcChannelStatus statsum;
  uint64_t last_bytes_received = 0;
  uint32_t i;
  //sum up all channels
  memset(&statsum, 0, sizeof(statsum));
  for(i=0;i<n;i++) {
    statsum.bytes_received += chstats[i]->bytes_received;
    statsum.error_count += chstats[i]->error_count;
    statsum.n_events += chstats[i]->n_events;
    statsum.set_offset_count += chstats[i]->set_offset_count;
    last_bytes_received += ch_last_bytes_received[i];
  }

  // summary stats
  printf("\n********Overall********\n");
  print_summary_stats(n, chstats, ch_last_bytes_received, timediff);
  printf("-------------------------\n");

  // per channel stats
  for(i=0;i<n;i++) {
    printf("CH%2d: Events: %10ld, DataSize: %8.3f GB ",
        i, chstats[i]->n_events,
        (double)chstats[i]->bytes_received/(double)(1<<30));

    if ( chstats[i]->bytes_received-ch_last_bytes_received[i])
    {
      printf(" DataRate: %9.3f MB/s",
          (double)(chstats[i]->bytes_received-
            ch_last_bytes_received[i])/
          timediff/(double)(1<<20));
    } else {
      printf(" DataRate: -");
    }
    printf(" Errors: %ld\n", chstats[i]->error_count);
    ch_last_bytes_received[i] = chstats[i]->bytes_received;
  }
}


void
dump_diu_state
(
    librorc::dma_channel *ch
)
{
  uint32_t status;
  status = ch->getLink()->GTX(RORC_REG_DDL_CTRL);
  printf("\nDIU_IF: ");
  if ( status & 1 ) printf("DIU_ON ");
  else printf("DIU_OFF ");
  if ( status>>1 & 1) printf("FC_ON ");
  else printf("FC_OFF ");
  if ( !(status>>4 & 1) ) printf("LF ");
  if ( !(status>>5 & 1) ) printf("LD ");
  if ( !(status>>30 & 1) ) printf("BSY ");
  if ( status>>8 & 1) printf("PG_ON");
  else printf("PG_OFF");

  if ( !(status>>8 & 1) ) { // PG disabled
    printf("CTSTW:%08x ", ch->getLink()->GTX(RORC_REG_DDL_CTSTW));
    printf("DEADTIME:%08x ", ch->getLink()->GTX(RORC_REG_DDL_DEADTIME));
    printf("EC:%08x ", ch->getLink()->GTX(RORC_REG_DDL_EC));
  }
}


/**
 * dump event to file(s)
 * @param base_dir pointer to destination directory string
 * @param pointer to stats channel stats struct
 * @param file_index index of according file, appears in file name
 * @param reportbuffer pointer to reportbuffer
 * @param ebuf pointer to librorc::buffer
 * @return 0 on sucess, -1 on error
 * */
int dump_to_file
(
    char                     *base_dir,
    librorcChannelStatus     *stats,
    uint64_t                  EventID,
    uint32_t                  file_index,
    librorc_event_descriptor *reportbuffer,
    librorc::buffer          *ebuf,
    uint32_t                  error_flags
)
{
  char *ddlname = NULL;
  char *logname = NULL;
  int len, result;
  FILE *fd_ddl, *fd_log;
  uint32_t i;
  uint32_t *eventbuffer = (uint32_t *)ebuf->getMem();

  // get length of destination file string
  len = snprintf(NULL, 0, "%s/ch%d_%d.ddl",
      base_dir, stats->channel, file_index);
  if (len<0) {
    perror("dump_to_file::snprintf failed");
    return -1;
  }

  // allocate memory for destination file string
  ddlname = (char *)malloc(len+1);
  if (ddlname==NULL) {
    perror("dump_to_file::malloc(ddlname) failed");
    return -1;
  }
  logname = (char *)malloc(len+1);
  if (logname==NULL) {
    perror("dump_to_file::malloc(logname) failed");
    return -1;
  }

  // fill destination file string
  snprintf(ddlname, len+1, "%s/ch%d_%d.ddl",
      base_dir, stats->channel, file_index);
  snprintf(logname, len+1, "%s/ch%d_%d.log",
      base_dir, stats->channel, file_index);

  // open DDL file
  fd_ddl = fopen(ddlname, "w");
  if ( fd_ddl < 0 ) {
    perror("failed to open destination DDL file");
    return -1;
  }

  // open log file
  fd_log =  fopen(logname, "w");
  if ( fd_ddl == NULL )
  {
    printf("failed to open destination LOG file : %s\n", ddlname);
    return -1;
  }

  // dump RB entry to log
  fprintf(fd_log, "CH%2d - RB[%3ld]: \ncalc_size=%08x\n"
      "reported_size=%08x\n"
      "offset=%lx\n"
      "EventID=%lx\n"
      "LastID=%lx\n",
      stats->channel, stats->index, 
      reportbuffer[stats->index].calc_event_size,
      reportbuffer[stats->index].reported_event_size,
      reportbuffer[stats->index].offset,
      EventID,
      stats->last_id);

  /** dump error type */
  fprintf(fd_log, "Check Failed: ");
  if ( error_flags & CHK_SIZES ) fprintf(fd_log, " CHK_SIZES");
  if ( error_flags & CHK_PATTERN ) fprintf(fd_log, " CHK_PATTERN");
  if ( error_flags & CHK_SOE ) fprintf(fd_log, " CHK_SOE");
  if ( error_flags & CHK_EOE ) fprintf(fd_log, " CHK_EOE");
  if ( error_flags & CHK_ID ) fprintf(fd_log, " CHK_ID");
  if ( error_flags & CHK_FILE ) fprintf(fd_log, " CHK_FILE");
  fprintf(fd_log, "\n\n");

  // check for reasonable calculated event size
  if (reportbuffer[stats->index].calc_event_size > (ebuf->getPhysicalSize()>>2))
  {
    fprintf(fd_log, "calc_event_size (0x%x DWs) is larger than physical "
        "buffer size (0x%lx DWs) - not dumping event.\n",
        reportbuffer[stats->index].calc_event_size,
        (ebuf->getPhysicalSize()>>2) );
  }
  // check for reasonable offset
  else if (reportbuffer[stats->index].offset > ebuf->getPhysicalSize())
  {
    fprintf(fd_log, "offset (0x%lx) is larger than physical "
        "buffer size (0x%lx) - not dumping event.\n",
        reportbuffer[stats->index].offset,
        ebuf->getPhysicalSize() );
  }
  else
  {
    // dump event to log
    for(i=0;i<reportbuffer[stats->index].calc_event_size;i++)
    {
        uint32_t ebword = (uint32_t)*(eventbuffer +
                (reportbuffer[stats->index].offset>>2) + i);

        fprintf(fd_log, "%03d: %08x", i, ebword);
        if ( (error_flags & CHK_PATTERN) &&
                (i>7) && (ebword != i-8) )
        {
            fprintf(fd_log, " expected %08x", i-8);
        }

        fprintf(fd_log, "\n");
    }

    fprintf(fd_log, "%03d: EOE reported_event_size: %08x\n", i,
        (uint32_t)*(eventbuffer + (reportbuffer[stats->index].offset>>2) + i));

    //dump event to DDL file
    result = fwrite(eventbuffer + (reportbuffer[stats->index].offset>>2), 4,
        reportbuffer[stats->index].calc_event_size, fd_ddl);
    if( result<0 ) {
      perror("failed to copy event data into DDL file");
      return -1;
    }

  }

  fclose(fd_log);
  fclose(fd_ddl);

  return 0;
}


#endif
