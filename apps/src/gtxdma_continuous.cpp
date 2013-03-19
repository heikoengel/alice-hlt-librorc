/**
 * @file gtxdma_continuous.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2012-12-17
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
 * Open DMA Channel sourced by optical DDL
 *
 * */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/shm.h>

#include "librorc.h"
#include "event_handling.h"

using namespace std;

/** Buffer Sizes (in Bytes) **/
#ifndef SIM
#define EBUFSIZE (((unsigned long)1) << 28)
#define RBUFSIZE (((unsigned long)1) << 26)
#define STAT_INTERVAL 1.0
#else
#define EBUFSIZE (((unsigned long)1) << 19)
#define RBUFSIZE (((unsigned long)1) << 17)
#define STAT_INTERVAL 0.00001
#endif


/** maximum channel number allowed **/
#define MAX_CHANNEL 11

int done = 0;


void abort_handler( int s )
{
  printf("Caught signal %d\n", s);
  done = 1;
}


int main( int argc, char *argv[])
{
  int result = 0;
  rorcfs_device *dev = NULL;
  rorcfs_bar *bar1 = NULL;
  rorcfs_buffer *ebuf = NULL;
  rorcfs_buffer *rbuf = NULL;
  rorcfs_dma_channel *ch = NULL;

  struct rorcfs_event_descriptor *reportbuffer = NULL;
  unsigned int *eventbuffer = NULL;
  timeval start_time, end_time;
  timeval last_time, cur_time;
  unsigned long last_bytes_received;
  unsigned long last_events_received;
  unsigned int ChannelId;

  // catch CTRL+C for abort
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = abort_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  //shared memory
  int shID;
  struct ch_stats *chstats = NULL;
  char *shm = NULL;

  // charg argument count
  if (argc!=2 ) {
    printf("wrong argument count %d\n"
        "usage: %s [ChannelId]\n",
        argc, argv[0]);
    result = -1;
    exit(-1);
  }

  // get ChannelID
  ChannelId= strtoul(argv[1], NULL, 0);	
  if ( errno || ChannelId>MAX_CHANNEL) {
    perror("illegal ChannelId");
    result = -1;
    exit(-1);
  }	

  //allocate shared mem
  shID = shmget(SHM_KEY_OFFSET + ChannelId, 
      sizeof(struct ch_stats), IPC_CREAT | 0666);
  if(shID==-1) {
    perror("shmget");
    goto out;
  }
  //attach to shared memory
  shm = (char*)shmat(shID, 0, 0);
  if (shm==(char*)-1) {
    perror("shmat");
    goto out;
  }
  chstats = (struct ch_stats*)shm;


  // create new device instance
  dev = new rorcfs_device();	
  if ( dev->init(0) == -1 ) {
    printf("ERROR: failed to initialize device.\n");
    goto out;
  }

  printf("Bus %x, Slot %x, Func %x\n", dev->getBus(),
      dev->getSlot(),dev->getFunc());

  // bind to BAR1
  bar1 = new rorcfs_bar(dev, 1);
  if ( bar1->init() == -1 ) {
    printf("ERROR: failed to initialize BAR1.\n");
    goto out;
  }

  printf("FirmwareDate: %08x\n", bar1->get(RORC_REG_FIRMWARE_DATE));

  // check if requested channel is implemented in firmware
  if ( ChannelId >= (bar1->get(RORC_REG_TYPE_CHANNELS) & 0xffff)) {
    printf("ERROR: Requsted channel %d is not implemented in "
        "firmware - exiting\n", ChannelId);
    goto out;
  }

  // create new DMA event buffer
  ebuf = new rorcfs_buffer();			
  if ( ebuf->allocate(dev, EBUFSIZE, 2*ChannelId, 
        1, RORCFS_DMA_FROM_DEVICE)!=0 ) {
    if ( errno == EEXIST ) {
      if ( ebuf->connect(dev, 2*ChannelId) != 0 ) {
        perror("ERROR: ebuf->connect");
        goto out;
      }
    } else {
      perror("ERROR: ebuf->allocate");
      goto out;
    }
  }

  // create new DMA report buffer
  rbuf = new rorcfs_buffer();;
  if ( rbuf->allocate(dev, RBUFSIZE, 2*ChannelId+1, 
        1, RORCFS_DMA_FROM_DEVICE)!=0 ) {
    if ( errno == EEXIST ) {
      //printf("INFO: Buffer already exists, trying to connect...\n");
      if ( rbuf->connect(dev, 2*ChannelId+1) != 0 ) {
        perror("ERROR: rbuf->connect");
        goto out;
      }
    } else {
      perror("ERROR: rbuf->allocate");
      goto out;
    }
  }

  // initialize channel stats struct
  /*chstats = (struct ch_stats*)malloc(sizeof(struct ch_stats));
    if ( chstats==NULL ){
    perror("alloc chstats");
    result=-1;
    goto out;
    }*/

  memset(chstats, 0, sizeof(struct ch_stats));
  chstats->index = 0;
  chstats->last_id = -1;
  chstats->channel = (unsigned int)ChannelId;


  // create DMA channel
  ch = new rorcfs_dma_channel();

  // bind channel to BAR1, channel offset 0
  ch->init(bar1, ChannelId);

  // prepare EventBufferDescriptorManager
  // with scatter-gather list
  result = ch->prepareEB( ebuf );
  if (result < 0) {
    perror("prepareEB()");
    result = -1;
    goto out;
  }

  // prepare ReportBufferDescriptorManager
  // with scatter-gather list
  result = ch->prepareRB( rbuf );
  if (result < 0) {
    perror("prepareRB()");
    result = -1;
    goto out;
  }

  // set MAX_PAYLOAD, buffer sizes, #sgEntries, ...
  result = ch->configureChannel(ebuf, rbuf, 256, 512);
  if (result < 0) {
    perror("configureChannel()");
    result = -1;
    goto out;
  }


  /* clear report buffer */
  reportbuffer = (struct rorcfs_event_descriptor *)rbuf->getMem();
  memset(reportbuffer, 0, rbuf->getMappingSize());

  eventbuffer = (unsigned int *)ebuf->getMem();

  // enable BDMs
  ch->setEnableEB(1);
  ch->setEnableRB(1);

  // enable DMA channel
  ch->setDMAConfig( ch->getDMAConfig() | 0x01 );


  // wait for GTX domain to be ready
  // read asynchronous GTX status
  // wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
  // & !gtx_in_rst
  printf("Waiting for GTX to be ready...\n");
  while( (ch->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
    usleep(100);

  // set ENABLE, activate flow control (DIU_IF:busy)
  ch->setGTX(RORC_REG_DDL_CTRL, 0x00000003);

  // wait for riLD_N='1'
  printf("Waiting for LD_N to deassert...\n");
  while( (ch->getGTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20 )
    usleep(100);

  // send RdyRx to SIU
  ch->setGTX(RORC_REG_DDL_CMD, 0x00000014); //RdyRX

  // TODO: wait for DIU status???

  // capture starting time
  bar1->gettime(&start_time, 0);
  last_time = start_time;
  cur_time = start_time;

  last_bytes_received = 0;
  last_events_received = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
  while( !done ) {

    // this can be aborted by abort_handler(),
    // triggered from CTRL+C

    result =  handle_channel_data(
        reportbuffer, 
        eventbuffer, 
        ch, // channe struct
        chstats, // stats struct
        rbuf->getPhysicalSize(), //rBuf size
        rbuf->getMaxRBEntries(), //rBuf #entries
        1); // do sanity check

    if ( result < 0 ) {
      printf("handle_channel_data failed for channel %d\n", ChannelId);
      goto out;
    } else if ( result==0 ) {
      // no events available
      usleep(100);
    }

    bar1->gettime(&cur_time, 0);

    // print status line each second
    if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL) {
      printf("Events: %10ld, DataSize: %8.3f GB ",
          chstats->n_events, 
          (double)chstats->bytes_received/(double)(1<<30));

      if ( chstats->bytes_received-last_bytes_received)
      {
        printf(" DataRate: %9.3f MB/s",
            (double)(chstats->bytes_received-last_bytes_received)/
            gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
      } else {
        printf(" DataRate: -");
        dump_dma_state(ch);
        dump_diu_state(ch);
      }

      if ( chstats->n_events - last_events_received)
      {
        printf(" EventRate: %9.3f kHz/s",
            (double)(chstats->n_events-last_events_received)/
            gettimeofday_diff(last_time, cur_time)/1000.0);
      } else {
        printf(" EventRate: -");
      }
      printf(" Errors: %ld\n", chstats->error_count);
      last_time = cur_time;
      last_bytes_received = chstats->bytes_received;
      last_events_received = chstats->n_events;
    }

  }

  // EOR
  bar1->gettime(&end_time, 0);

  // print summary
  printf("%ld Byte / %ld events in %.2f sec"
      "-> %.1f MB/s.\n", 
      (chstats->bytes_received), chstats->n_events, 
      gettimeofday_diff(start_time, end_time),
      ((float)chstats->bytes_received/
       gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

  if(!chstats->set_offset_count) //avoid DivByZero Exception
    printf("CH%d: No Events\n", ChannelId);
  else 
    printf("CH%d: Events %ld, max_epi=%ld, min_epi=%ld, "
        "avg_epi=%ld, set_offset_count=%ld\n", ChannelId, 
        chstats->n_events, chstats->max_epi, 
        chstats->min_epi,
        chstats->n_events/chstats->set_offset_count, 
        chstats->set_offset_count);

  // check if link is still up: LD_N == 1
  if ( ch->getGTX(RORC_REG_DDL_CTRL) & (1<<5) ) {

    // disable BUSY -> drop current data in chain
    ch->setGTX(RORC_REG_DDL_CTRL, 0x00000001);

    // wait for LF_N to go high
    while(!(ch->getGTX(RORC_REG_DDL_CTRL) & (1<<4)))
      usleep(100);

    // Send EOBTR command
    ch->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

    // wait for command transmission status word (CTST)
    // in response to the EOBTR:
    // STS[7:4]="0000"
    while(ch->getGTX(RORC_REG_DDL_STS0) & 0xf0)
      usleep(100);

    // disable DIU_IF
    ch->setGTX(RORC_REG_DDL_CTRL, 0x00000000);
  } 
  else 
  { //link is down -> unable to send EOBTR
    printf("Link is down - unable to send EOBTR\n");
  }

  // disable EBDM -> no further sg-entries to PKT
  ch->setEnableEB(0);

  // wait for pending transfers to complete (dma_busy->0)
  while( ch->getDMABusy() )
    usleep(100);

  // disable RBDM
  ch->setEnableRB(0);

  // reset DFIFO, disable DMA PKT
  ch->setDMAConfig(0X00000002);

  // clear reportbuffer
  memset(reportbuffer, 0, rbuf->getMappingSize());


out:

  if (shm) {
    //free(chstats);
    shmdt(shm);
    shm = NULL;
  }
  if (ch)
    delete ch;
  if (ebuf)
    delete ebuf;
  if (rbuf)
    delete rbuf;

  if (bar1)
    delete bar1;
  if (dev)
    delete dev;

  return result;
}
