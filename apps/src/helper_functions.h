/**
 * @file helper_functions.h
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

using namespace librorc;

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
    uint32_t *eventbuffer,
    uint64_t offset,
    uint64_t len)
{
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
}


/**
 * Dump reportbuffer entry
 * @param reportbuffer pointer to reportbuffer
 * @param i index of current struct rorcfs_event_descriptor within
 * reportbuffer
 * @param ch DMA channel number
 * */
void dump_rb(
    struct rorcfs_event_descriptor *reportbuffer,
    uint64_t i,
    uint32_t ch )
{
  printf("CH%2d - RB[%3ld]: calc_size=%08x\t"
      "reported_size=%08x\t"
      "offset=%lx\n",
      ch, i, reportbuffer->calc_event_size,
      reportbuffer->reported_event_size,
      reportbuffer->offset);
}


/**
 * Dump the state of the DMA engine
 * @param ch pointer to struct rorcfs_dma_channel
 * */
void dump_dma_state(
    struct rorcfs_dma_channel *ch)
{
  uint32_t dma_ctrl;
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
    uint32_t n,
    struct ch_stats *chstats[],
    uint64_t *ch_last_bytes_received,
    double timediff)
{
  struct ch_stats statsum;
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
void print_channel_stats(
    uint32_t n,
    struct ch_stats *chstats[],
    uint64_t *ch_last_bytes_received,
    double timediff)
{
  struct ch_stats statsum;
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


void dump_diu_state ( struct rorcfs_dma_channel *ch )
{
  uint32_t status;
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
    printf("CTSTW:%08x ", ch->getGTX(RORC_REG_DDL_CTSTW));
    printf("DEADTIME:%08x ", ch->getGTX(RORC_REG_DDL_DEADTIME));
    printf("EC:%08x ", ch->getGTX(RORC_REG_DDL_EC));
  }
}


/**
 * dump event to file(s)
 * @param base_dir pointer to destination directory string
 * @param ch_index DMA channel index
 * @param rb_index index of according reportbuffer entry
 * @param file_index index of according file, appears in file name
 * @param reportbuffer pointer to reportbuffer
 * @param ebuf pointer to struct rorcfs_buffer
 * @return 0 on sucess, -1 on error
 * */
int dump_to_file (
    char *base_dir,
    uint32_t ch_index,
    uint64_t rb_index,
    uint32_t file_index,
    struct rorcfs_event_descriptor *reportbuffer,
    struct rorcfs_buffer *ebuf)
{
  char *ddlname = NULL;
  char *logname = NULL;
  int len, result;
  FILE *fd_ddl, *fd_log;
  uint64_t i;
  uint32_t *eventbuffer = (uint32_t *)ebuf->getMem();

  // get length of destination file string
  len = snprintf(NULL, 0, "%s/ch%d_%d.ddl",
      base_dir, ch_index, file_index);
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
      base_dir, ch_index, file_index);
  snprintf(logname, len+1, "%s/ch%d_%d.log",
      base_dir, ch_index, file_index);

  // open DDL file
  fd_ddl = fopen(ddlname, "w");
  if ( fd_ddl < 0 ) {
    perror("failed to open destination DDL file");
    return -1;
  }

  // open log file
  fd_log =  fopen(logname, "w");
  if ( fd_ddl < 0 ) {
    perror("failed to open destination LOG file");
    return -1;
  }

  // dump RB entry to log
  fprintf(fd_log, "CH%2d - RB[%3ld]: \ncalc_size=%08x\n"
      "reported_size=%08x\n"
      "offset=%lx\n\n",
      ch_index, rb_index, reportbuffer[rb_index].calc_event_size,
      reportbuffer[rb_index].reported_event_size,
      reportbuffer[rb_index].offset);

  // check for reasonable calculated event size
  if (reportbuffer[rb_index].calc_event_size > (ebuf->getPhysicalSize()>>2))
  {
    fprintf(fd_log, "calc_event_size (0x%x DWs) is larger than physical "
        "buffer size (0x%lx DWs) - not dumping event.\n",
        reportbuffer[rb_index].calc_event_size,
        (ebuf->getPhysicalSize()>>2) );
  }
  // check for reasonable offset
  else if (reportbuffer[rb_index].offset > ebuf->getPhysicalSize())
  {
    fprintf(fd_log, "offset (0x%lx) is larger than physical "
        "buffer size (0x%lx) - not dumping event.\n",
        reportbuffer[rb_index].offset,
        ebuf->getPhysicalSize() );
  }
  else
  {
    // dump event to log
    for(i=0;i<reportbuffer[rb_index].calc_event_size;i++)
      fprintf(fd_log, "%03ld: %08x\n",
          i, (uint32_t)*(eventbuffer +
            (reportbuffer[rb_index].offset>>2) + i));

    fprintf(fd_log, "%03ld: EOE reported_event_size: %08x\n", i,
        (uint32_t)*(eventbuffer + (reportbuffer[rb_index].offset>>2) + i));

    //dump event to DDL file
    result = fwrite(eventbuffer + (reportbuffer[rb_index].offset>>2), 4,
        reportbuffer[rb_index].calc_event_size, fd_ddl);
    if( result<0 ) {
      perror("failed to copy event data into DDL file");
      return -1;
    }

  }

  fclose(fd_log);
  fclose(fd_ddl);

  return 0;
}


///**
// * Dump ScatterGather List
// * @param buf pointer to the according struct rorcfs_buffer
// * */
//void dump_sglist( struct rorcfs_buffer *buf )
//{
//  char *fname = NULL;
//  int fname_size = 0;
//  int fd, nbytes;
//  unsigned long i;
//  struct rorcfs_dma_desc dma_desc;
//  fname_size = snprintf(NULL, 0, "%s/sglist", buf->getDName()) + 1;
//  fname = (char *)malloc(fname_size);
//  if (!fname) {
//    perror("malloc fname failed");
//    return;
//  }
//  snprintf(fname, fname_size, "%s/sglist", buf->getDName());
//  fd = open(fname, O_RDONLY);
//  free(fname);
//  if (fd==-1) {
//    perror("failed to open sglist file");
//    return;
//  }
//
//  for(i=0;i<buf->getnSGEntries();i++)
//  {
//    nbytes = read(fd, &dma_desc, sizeof(dma_desc));
//    if (nbytes != sizeof(dma_desc)) {
//      perror("failed to read from sglist");
//      close(fd);
//      return;
//    }
//    printf("%04ld: 0x%08x_%08x - 0x%08x_%08x\n",
//        i, (unsigned int)((unsigned long)dma_desc.addr>>32 & 0xffffffff),
//        (unsigned int)(dma_desc.addr & 0xffffffff),
//        (unsigned int)((unsigned long)dma_desc.len>>32 & 0xffffffff),
//        (unsigned int)(dma_desc.len & 0xffffffff) );
//  }
//
//  close(fd);
//}


#endif
