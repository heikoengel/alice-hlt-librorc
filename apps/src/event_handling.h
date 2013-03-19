#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

/** Shared mem key offset **/
#define SHM_KEY_OFFSET 2048

struct ch_stats {
  unsigned long n_events;
  unsigned long bytes_received;
  unsigned long min_epi;
  unsigned long max_epi;
  unsigned long index;
  unsigned long set_offset_count;
  unsigned long error_count;
  int last_id;
  unsigned int channel;
};


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


void dump_event(
    unsigned int *eventbuffer, 
    unsigned long offset,
    unsigned long len)
{
  unsigned long i = 0;
  for(i=0;i<len;i++) {
    printf("%03ld: %08x\n", 
        i, (unsigned int)*(eventbuffer + offset +i));
  }
  if(len&0x01) {
    printf("%03ld: %08x (dummy)\n", i, 
        (unsigned int)*(eventbuffer + offset + i));
    i++;
  }
  printf("%03ld: EOE reported_event_size: %08x\n", i, 
      (unsigned int)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE calc_event_size: %08x\n", i, 
      (unsigned int)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy %08x\n", i, 
      (unsigned int)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy: %08x\n", i, 
      (unsigned int)*(eventbuffer + offset + i));
}



void dump_rb(
    struct rorcfs_event_descriptor *reportbuffer,
    unsigned long i,
    unsigned int ch )
{
  printf("CH%2d - RB[%3ld]: calc_size=%08x\t"
      "reported_size=%08x\t"
      "offset=%lx\n",
      ch, i, reportbuffer->calc_event_size,
      reportbuffer->reported_event_size,
      reportbuffer->offset);
}


void dump_dma_state(
    struct rorcfs_dma_channel *ch)
{
  unsigned int dma_ctrl;
  printf("\nPKT:\n");
  printf("#Events: 0x%08x; ", ch->getPKT(RORC_REG_DMA_N_EVENTS_PROCESSED));
  printf("#Stall: 0x%08x; ", ch->getPKT(RORC_REG_DMA_STALL_CNT));
  dma_ctrl = ch->getPKT(RORC_REG_DMA_CTRL);
  printf("PKT_EN:%d; FIFO_RST:%d; EOE_IN_FIFO:%d; FIFO_EMPTY:%d; "
      "FIFO_PEMPTY:%d; BUSY:%d; EBDM_EN:%d, RBDM_EN:%d\n",
      dma_ctrl&1, (dma_ctrl>>1)&1, (dma_ctrl>>4)&1, (dma_ctrl>>5)&1,
      (dma_ctrl>>6)&1, (dma_ctrl>>7)&1, (dma_ctrl>>2)&1,
      (dma_ctrl>>3)&1);

  printf("EBDM:\n");
  printf("EBDM rdptr: 0x%08x_%08x; ",
      ch->getPKT(RORC_REG_EBDM_SW_READ_POINTER_L),
      ch->getPKT(RORC_REG_EBDM_SW_READ_POINTER_H));
  printf("EBDM wrptr: 0x%08x_%08x; ",
      ch->getPKT(RORC_REG_EBDM_FPGA_WRITE_POINTER_L),
      ch->getPKT(RORC_REG_EBDM_FPGA_WRITE_POINTER_H));
  printf("\n");

  printf("RBDM:\n");
  printf("RBDM rdptr: 0x%08x_%08x; ",
      ch->getPKT(RORC_REG_RBDM_SW_READ_POINTER_L),
      ch->getPKT(RORC_REG_RBDM_SW_READ_POINTER_H));
  printf("RBDM wrptr: 0x%08x_%08x; ",
      ch->getPKT(RORC_REG_RBDM_FPGA_WRITE_POINTER_L),
      ch->getPKT(RORC_REG_RBDM_FPGA_WRITE_POINTER_H));
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
void print_summary_stats(
    unsigned int n,
    struct ch_stats *chstats[],
    unsigned long *ch_last_bytes_received, 
    double timediff)
{
  struct ch_stats statsum;
  unsigned long last_bytes_received = 0;
  unsigned int i;
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
void print_channel_stats(
    unsigned int n,
    struct ch_stats *chstats[],
    unsigned long *ch_last_bytes_received, 
    double timediff)
{
  struct ch_stats statsum;
  unsigned long last_bytes_received = 0;
  unsigned int i;
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


void dump_diu_state ( struct rorcfs_dma_channel *ch )
{
  unsigned int status;
  status = ch->getGTX(RORC_REG_DDL_CTRL);
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
    printf("STS0:%08x ", ch->getGTX(RORC_REG_DDL_STS0));
    printf("DEADTIME:%08x ", ch->getGTX(RORC_REG_DDL_DEADTIME));
    printf("EC:%08x ", ch->getGTX(RORC_REG_DDL_EC));
  }
}



/**
 * Sanity checks on received data
 **/
int event_sanity_check( 
    struct rorcfs_event_descriptor *reportbuffer, 
    unsigned int *eventbuffer, 
    unsigned long i,
    unsigned int ch)
{
  unsigned long offset;
  unsigned int j;
  unsigned int *eb;
  int EventID;

  // compare calculated and reported event sizes
  if ( reportbuffer->calc_event_size != 
      reportbuffer->reported_event_size ) {
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

  offset = reportbuffer->offset/4;
  eb = (unsigned int*)malloc(
      (reportbuffer->reported_event_size+4)*sizeof(unsigned int));
  if ( eb==NULL ) {
    perror("Malloc EB");
    return -1;
  }
  memcpy(eb, (unsigned char *)eventbuffer + reportbuffer->offset, 
      (reportbuffer->reported_event_size+4)*sizeof(unsigned int));

  // sanity check on SOF
  if((unsigned int)*(eb)!=0xffffffff) {
#ifdef DEBUG
    printf("ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
        "offset=%ld, rbdm_offset=%ld\n", 
        i, (unsigned int)*(eb),
        reportbuffer->offset, 
        i*sizeof(struct rorcfs_event_descriptor) );
    dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
    dump_rb(reportbuffer, i, ch);
#endif
    free(eb);
    return -1;
  }

  // check PG pattern
  for (j=8;j<reportbuffer->calc_event_size;j++) {
    if ( (unsigned int)*(eb + j) != j-8 ) {
#ifdef DEBUG
      printf("ERROR: Event[%ld][%d] expected %08x read %08x\n",
          i, j, j-8, (unsigned int)*(eventbuffer + offset + j));
      dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
      dump_rb(reportbuffer, i, ch);
#endif
      free(eb);
      return -1;
    }
  }

  if (DMA_MODE==128)
  {

    //TODO: EOE pattern is different for 32bit Packetizer
    // check event counts at EOE
    j = reportbuffer->calc_event_size;
    j &= (unsigned int)0xfffffffc;
    if ( reportbuffer->calc_event_size & 0x3 )
      j+=4;

    if ( (unsigned int)*(eb + j) != 
        reportbuffer->calc_event_size ) {
#ifdef DEBUG
      printf("ERROR: could not find matching calculated event size "
          "at Event[%d], expected %08x found %08x\n", 
          j, reportbuffer->calc_event_size,
          (unsigned int)*(eb + j) );
      dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
      dump_rb(reportbuffer, i, ch);
#endif
      free(eb);
      return -1;
    }

    j++;
    if ( (unsigned int)*(eb + j) != 
        reportbuffer->reported_event_size ) {
#ifdef DEBUG
      printf("ERROR: could not find matching reported event size "
          "at Event[%d] expected %08x found %08x\n", 
          j, reportbuffer->reported_event_size,
          (unsigned int)*(eb + j) );
      dump_event(eventbuffer, offset, reportbuffer->reported_event_size);
      dump_rb(reportbuffer, i, ch);
#endif
      free(eb);
      return -1;
    }
  } // DMA_MODE
  else if (DMA_MODE==32) 
  {

    if ( (unsigned int)*(eb + reportbuffer->calc_event_size) != 
        reportbuffer->calc_event_size ) {
#ifdef DEBUG
      printf("ERROR: could not find matching calculated event size "
          "at Event[%d] expected %08x found %08x\n", 
          j, reportbuffer->calc_event_size,
          (unsigned int)*(eb + j) );
      dump_event(eventbuffer, offset, reportbuffer->calc_event_size);
      dump_rb(reportbuffer, i, ch);
#endif
      free(eb);
      return -1;
    }
  } //DMA_MODE


  EventID = (int)*(eventbuffer + offset + 1) & 0x00000fff;
  free(eb);

  return EventID;
}




/**
 * handle incoming data
 **/
int handle_channel_data( 
    struct rorcfs_event_descriptor *reportbuffer,
    unsigned int *eventbuffer,
    struct rorcfs_dma_channel *ch,
    struct ch_stats *stats,
    unsigned long rbsize,
    unsigned long maxrbentries,
    int do_sanity_check)
{
  unsigned long events_per_iteration = 0;
  int events_processed = 0;
  unsigned long eboffset, rboffset;
  unsigned long starting_index, entrysize;
  struct rorcfs_event_descriptor rb;
  int EventID;


  if( reportbuffer[stats->index].calc_event_size!=0 ) { // new event received

    starting_index = stats->index;

    while( reportbuffer[stats->index].calc_event_size!=0 ) {
      events_processed++;


      if (do_sanity_check) {
        rb = reportbuffer[stats->index];
        EventID = event_sanity_check(&rb,
            eventbuffer, 
            stats->index, 
            stats->channel);

        if ( EventID<0) {
          stats->error_count++;
#ifdef SIM
          return -1;
#endif
        }

        if ( stats->last_id!=-1 &&
            (EventID & 0xfff) != ((stats->last_id +1) & 0xfff) ) {
#ifdef DEBUG
          printf("ERROR: CH%d - Invalid Event Sequence: last ID: %d, "
              "current ID: %d\n", stats->channel, stats->last_id, EventID);
          dump_event(eventbuffer, reportbuffer[stats->index].offset>>2,
              reportbuffer[stats->index].reported_event_size);
          dump_rb(&rb, stats->index, stats->channel);
#endif
          stats->error_count++;
#ifdef SIM
          return -1;
#endif
        }
        stats->last_id = EventID;
      }
#ifdef DEBUG
      dump_rb(&reportbuffer[stats->index], stats->index, stats->channel);
#endif

      //stats->bytes_received += reportbuffer[stats->index].length;
      stats->bytes_received += 
        (reportbuffer[stats->index].reported_event_size<<2);

      // set new EBOffset
      eboffset = reportbuffer[stats->index].offset;

      // increment reportbuffer offset
      rboffset = ((stats->index)*
          sizeof(struct rorcfs_event_descriptor)) % rbsize;

      // wrap RB pointer if necessary
      if( stats->index < maxrbentries-1 ) 
        stats->index++;
      else
        stats->index=0;
      stats->n_events++;
      events_per_iteration++;
    }

    // clear processed reportbuffer entries
    entrysize = sizeof(struct rorcfs_event_descriptor);
    //printf("clearing RB: start: %ld entries: %ld, %ldb each\n",
    //	entrysize*starting_index, events_per_iteration, entrysize);

    memset(&reportbuffer[starting_index], 0, 
        events_per_iteration*entrysize);


    if(events_per_iteration > stats->max_epi)
      stats->max_epi = events_per_iteration;
    if(events_per_iteration < stats->min_epi)
      stats->min_epi = events_per_iteration;
    events_per_iteration = 0;
    stats->set_offset_count++;
#ifdef DEBUG
    printf("CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n", 
        stats->channel, rboffset, eboffset);
#endif
    ch->setEBOffset(eboffset);
    ch->setRBOffset(rboffset);
  }

  return events_processed;
}

#endif
