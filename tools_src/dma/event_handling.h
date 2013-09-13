#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

/** limit the number of corrupted events to be written to disk **/
#define MAX_FILES_TO_DISK 100

/** Pattern Generator Mode: Ramp **/
#define PG_PATTERN_RAMP (1<<0)

/** sanity checks **/
#define CHK_SIZES   (1<<0)
#define CHK_PATTERN (1<<1)
#define CHK_SOE     (1<<2)
#define CHK_EOE     (1<<3)
#define CHK_ID      (1<<4)
#define CHK_FILE    (1<<8)

#include <pda.h>
#include <librorc.h>
#include <fcntl.h>


/**
 * TODO: dump to file?
 * Dump Event
 * @param eventbuffer pointer to eventbuffer
 * @param offset offset of the current event within the eventbuffer
 * @param len size in DWs of the event
 * */
void
dump_event
(
    volatile uint32_t *eventbuffer,
    uint64_t offset,
    uint64_t len
)
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
 * ==========================================================
 * print summary statistics
 * ==========================================================
 * @param n number of channels
 * @param chstats pointer to struct ch_stats
 * @param ch_last_bytes_received pointer to array of bytescounts
 * fromt the last iteration
 * @param timediff time passed since the last iteration
 * */
void
print_summary_stats
(
    uint32_t       n,
    librorcChannelStatus *chstats[],
    uint64_t      *ch_last_bytes_received,
    double         timediff
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
 * Dump reportbuffer entry
 * @param reportbuffer pointer to reportbuffer
 * @param i index of current struct librorc_event_descriptor within
 * reportbuffer
 * @param ch DMA channel number
 * */
void
dump_rb
(
    struct librorc_event_descriptor *reportbuffer,
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


//TODO: this looks like redundant with the dma monitor
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
void
print_channel_stats
(
    uint32_t n,
    librorcChannelStatus *chstats[],
    uint64_t *ch_last_bytes_received,
    double timediff
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


//TODO: put this into the dma channel class!
void
dump_diu_state
(
    librorc::dma_channel *ch
)
{
    uint32_t status = ch->getGTX(RORC_REG_DDL_CTRL);

    printf("\nDIU_IF: ");

     (status & 1)       ? printf("DIU_ON ") : printf("DIU_OFF ");
     ((status>>1) & 1)  ? printf("FC_ON ")  : printf("FC_OFF ");
    !((status>>4) & 1)  ? printf("LF ")     : printf("");
    !((status>>5) & 1)  ? printf("LD ")     : printf("");
    !((status>>30) & 1) ? printf("BSY ")    : printf("");

    /** PG disabled */
    ((status>>8) & 1) ? printf("PG_ON") : printf("PG_OFF");

    if ( !(status>>8 & 1) )
    {
        printf("CTSTW:%08x ", ch->getGTX(RORC_REG_DDL_CTSTW));
        printf("DEADTIME:%08x ", ch->getGTX(RORC_REG_DDL_DEADTIME));
        printf("EC:%08x ", ch->getGTX(RORC_REG_DDL_EC));
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
    char            *base_dir,
    librorcChannelStatus   *stats,
    uint64_t        EventID,
    uint32_t         file_index,
    struct librorc_event_descriptor *reportbuffer,
    librorc::buffer *ebuf,
    uint32_t        error_flags
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



//TODO : this going to be refactored into a class
/**
 * Sanity checks on received data
 * @param reportbuffer pointer to struct librorc_event_descriptor
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
    struct librorc_event_descriptor *reportbuffer,
    volatile uint32_t *eventbuffer,
    uint64_t i,
    uint32_t ch,
    int64_t last_id,
    uint32_t pattern_mode,
    uint32_t check_mask,
    uint32_t *ddlref,
    uint64_t ddlref_size,
    uint64_t *event_id
)
{
  uint64_t offset;
  uint32_t j;
  uint32_t *eb = NULL;
  uint64_t cur_event_id;
  int retval = 0;

  /** upper two bits of the sizes are reserved for flags */
  uint32_t reported_event_size = (reportbuffer->reported_event_size & 0x3fffffff);
  uint32_t calc_event_size = (reportbuffer->calc_event_size & 0x3fffffff);

  /** Bit31 of calc_event_size is read completion timeout flag */
  uint32_t timeout_flag = (reportbuffer->calc_event_size>>31);

  // compare calculated and reported event sizes
  if (check_mask & CHK_SIZES)
  {
      if (timeout_flag)
      {
          DEBUG_PRINTF(PDADEBUG_ERROR,
                  "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
                  ch, i);
          retval |= CHK_SIZES;
      } else if (calc_event_size != reported_event_size)
      {
          DEBUG_PRINTF(PDADEBUG_ERROR,
                  "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                  "calculated: 0x%x, reported: 0x%x\n"
                  "offset=0x%lx, rbdm_offset=0x%lx\n", ch, i,
                  calc_event_size,reported_event_size,
                  reportbuffer->offset,
                  i*sizeof(struct librorc_event_descriptor) );
          retval |= CHK_SIZES;
      }
  }

  /// reportbuffer->offset is a multiple of bytes
  offset = reportbuffer->offset/4;
  eb = (uint32_t*)malloc((reported_event_size+4)*sizeof(uint32_t));
  if ( eb==NULL ) {
    perror("Malloc EB");
    return -1;
  }
  // local copy of the event.
  // TODO: replace this with something more meaningful
  memcpy(eb, (uint8_t *)eventbuffer + reportbuffer->offset,
      (reported_event_size+4)*sizeof(uint32_t));

  // sanity check on SOF
  if( (check_mask & CHK_SOE) &&
          ((uint32_t)*(eb)!=0xffffffff) ) {
      DEBUG_PRINTF(PDADEBUG_ERROR,
              "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
              "offset=%ld, rbdm_offset=%ld\n",
              i, (uint32_t)*(eb),
              reportbuffer->offset,
              i*sizeof(struct librorc_event_descriptor) );
      dump_event(eventbuffer, offset, reported_event_size);
      dump_rb(reportbuffer, i, ch);
      retval |= CHK_SOE;
  }

  if ( check_mask & CHK_PATTERN )
  {
    switch (pattern_mode)
    {
      /* Data pattern is a ramp */
      case PG_PATTERN_RAMP:

        // check PG pattern
        for (j=8;j<calc_event_size;j++) {
          if ( (uint32_t)*(eb + j) != j-8 ) {
              DEBUG_PRINTF(PDADEBUG_ERROR,
                      "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                      i, j, j-8, (uint32_t)*(eventbuffer + offset + j));
              dump_event(eventbuffer, offset, reported_event_size);
              dump_rb(reportbuffer, i, ch);
              retval |= CHK_PATTERN;
          }
        }
        break;

      default:
        printf("ERROR: specified unknown pattern matching algorithm\n");
        retval |= CHK_PATTERN;
    }
  }

  // compare with reference DDL file
  if ( check_mask & CHK_FILE )
  {
    if ( ((uint64_t)calc_event_size<<2) != ddlref_size )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: Eventsize %lx does not match "
                "reference DDL file size %lx\n",
                ((uint64_t)calc_event_size<<2),
                ddlref_size);
        dump_event(eventbuffer, offset, reported_event_size);
        dump_rb(reportbuffer, i, ch);
        retval |= CHK_FILE;
    }

    for (j=0;j<calc_event_size;j++) {
        if ( eb[j] != ddlref[j] ) {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                    i, j, ddlref[j], eb[j]);
            dump_event(eventbuffer, offset, reported_event_size);
            dump_rb(reportbuffer, i, ch);
            retval |= CHK_FILE;
      }
    }
  }

  if ( check_mask & CHK_EOE )
  {
      /**
       * 32 bit DMA mode
       *
       * DMA data is written in multiples of 32 bit. A 32bit EOE word
       * is directly attached after the last event data word.
       * The EOE word contains the EOE status word received from the DIU
       **/

      if ( (uint32_t)*(eb + calc_event_size) != reported_event_size ) {
          DEBUG_PRINTF(PDADEBUG_ERROR,
                  "ERROR: could not find matching reported event size "
                  "at Event[%d] expected %08x found %08x\n",
                  j, calc_event_size,
                  (uint32_t)*(eb + j) );
          dump_event(eventbuffer, offset, calc_event_size);
          dump_rb(reportbuffer, i, ch);
          retval |= CHK_EOE;
      }

  } // CHK_EOE

  // get EventID from CDH:
  // lower 12 bits in CHD[1][11:0]
  // upper 24 bits in CDH[2][23:0]
  cur_event_id = (uint32_t)*(eventbuffer + offset + 2) & 0x00ffffff;
  cur_event_id <<= 12;
  cur_event_id |= (uint32_t)*(eventbuffer + offset + 1) & 0x00000fff;

  // make sure EventIDs increment with each event.
  // missing EventIDs are an indication of lost event data
  if ( (check_mask & CHK_ID) &&
          last_id!=-1 &&
          (cur_event_id & 0xfffffffff) !=
          ((last_id +1) & 0xfffffffff) ) {
      DEBUG_PRINTF(PDADEBUG_ERROR,
              "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
              "current ID: %ld\n", ch, last_id, cur_event_id);
      dump_event(eventbuffer, offset, calc_event_size);
      dump_rb(reportbuffer, i, ch);
      retval |= CHK_EOE;
  }

  /** return event ID to caller */
  *event_id = cur_event_id;

  if(eb)
  {
      free(eb);
  }
  return retval;
}

#endif
