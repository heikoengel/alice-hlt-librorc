/**
 * @file pgdma_continuous.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2012-11-14#include <iostream>
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

#include "librorc.h"
#include "event_handling.h"

using namespace std;

#ifndef SIM
// EventBuffer size in bytes
#define EBUFSIZE (((unsigned long)1) << 28)
// ReportBuffer size in bytes
#define RBUFSIZE (((unsigned long)1) << 26)
#else
// EventBuffer size in bytes
#define EBUFSIZE (((unsigned long)1) << 19)
// ReportBuffer size in bytes
#define RBUFSIZE (((unsigned long)1) << 17)
#endif


/** maximum channel number allowed **/
#define MAX_CHANNEL 11

int done = 0;


void abort_handler( int s )
{
  printf("Caught signal %d\n", s);
  if (done==1)
    exit(-1);
  else
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
  struct ch_stats *chstats = NULL;
  unsigned long last_bytes_received;
  unsigned long EventSize, ChannelId;

  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = abort_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);


  // charg argument count
  if (argc!=3 ) {
    printf("wrong argument count %d\n"
        "usage: %s [ChannelId] [EventSize]\n"
        "where [EventSize] is the number of DWs sent after"
        "the CommonDataHeader (CDH). The CDH itself consists of"
        "8 DWs\n",
        argc, argv[0]);
    result = -1;
    exit(-1);
  }

  // get ChannelID
  ChannelId= strtoul(argv[1], NULL, 0);	
  if ((errno == ERANGE && ChannelId == ULONG_MAX)
      || (errno != 0 && ChannelId>MAX_CHANNEL)) {
    perror("illegal ChannelID");
    result = -1;
    exit(-1);
  }	

  // get EventSize
  EventSize = strtoul(argv[2], NULL, 0);	
  if ((errno == ERANGE && EventSize == ULONG_MAX)
      || (errno != 0 && EventSize== 0)) {
    perror("illegal EventSize");
    result = -1;
    exit(-1);
  }	

  // create new device class
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

  chstats = (struct ch_stats*)malloc(sizeof(struct ch_stats));
  if ( chstats==NULL ){
    perror("alloc chstats");
    result=-1;
    goto out;
  }

  // initialize stats struct
  memset(chstats, 0, sizeof(struct ch_stats));
  chstats->index = 0;
  chstats->last_id = -1;
  chstats->channel = (unsigned int)ChannelId;


  // create DMA channel
  ch = new rorcfs_dma_channel();

  // bind channel to BAR1
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
  result = ch->configureChannel(ebuf, rbuf, 128, 256);
  if (result < 0) {
    perror("configureChannel()");
    result = -1;
    goto out;
  }


  /* clear report buffer */
  reportbuffer = (struct rorcfs_event_descriptor *)rbuf->getMem();
  printf("pRBUF=%p, MappingSize=%ld\n", 
      rbuf->getMem(), rbuf->getMappingSize() );
  memset(reportbuffer, 0, rbuf->getMappingSize());

  eventbuffer = (unsigned int *)ebuf->getMem();


  // wait for GTX domain to be ready
  // read asynchronous GTX status
  // wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
  // & !gtx_in_rst
  printf("Waiting for GTX to be ready...\n");
  while( (ch->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
    usleep(100);

  // Configure Pattern Generator
  ch->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, EventSize);
  ch->setGTX(RORC_REG_DDL_CTRL, 
      ch->getGTX(RORC_REG_DDL_CTRL) | 0x600); //set PG mode
  ch->setGTX(RORC_REG_DDL_CTRL, 
      ch->getGTX(RORC_REG_DDL_CTRL) | 0x100); //enable PG

  ch->setEnableEB(1);
  ch->setEnableRB(1);

  // capture starting time
  bar1->gettime(&start_time, 0);
  last_time = start_time;
  cur_time = start_time;

  // enable DMA channel
  ch->setDMAConfig( ch->getDMAConfig() | 0x01 );


  last_bytes_received = 0;

  while( !done ) {

    // this can be aborted by abort_handler(),
    // triggered from CTRL+C

    result =  handle_channel_data(
        rbuf, 
        eventbuffer, 
        ch, // channe struct
        chstats, // stats struct
        0xff, // do sanity check
        NULL, // no reference DDL
        0); //reference DDL size

    if ( result < 0 ) {
      printf("handle_channel_data failed for channel %ld\n", ChannelId);
      goto out;
    } else if ( result==0 ) {
      // no events available
      usleep(100);
    }

    bar1->gettime(&cur_time, 0);

    // print status line each second
    if(gettimeofday_diff(last_time, cur_time)>1.0) {
      printf("Events: %10ld, DataSize: %8.3f GB",
          chstats->n_events, 
          (double)chstats->bytes_received/(double)(1<<30));

      if ( chstats->bytes_received-last_bytes_received)
      {
        printf(" \tDataRate: %9.3f MB/s",
            (double)(chstats->bytes_received-last_bytes_received)/
            gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
      } else {
        printf(" \tDataRate: -");
        dump_dma_state(ch);
      }
      printf("\tErrors: %ld\n", chstats->error_count);
      last_time = cur_time;
      last_bytes_received = chstats->bytes_received;
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
    printf("CH%ld: No Events\n", ChannelId);
  else 
    printf("CH%ld: Events %ld, max_epi=%ld, min_epi=%ld, "
        "avg_epi=%ld, set_offset_count=%ld\n", ChannelId, 
        chstats->n_events, chstats->max_epi, 
        chstats->min_epi,
        chstats->n_events/chstats->set_offset_count, 
        chstats->set_offset_count);


  //disable PG
  ch->setGTX(RORC_REG_DDL_CTRL, 0x0); //disable PG
  // disable DMA Engine
  ch->setEnableEB(0);


  // wait for pending transfers to complete (dma_busy->0)
  while( ch->getDMABusy() )
    usleep(100);

  // disable RBDM
  ch->setEnableRB(0);

  // reset DFIFO, disable DMA PKT
  ch->setDMAConfig(0X00000002);

  if (chstats) {
    free(chstats);
    chstats = NULL;
  }
  if (ch) {
    delete ch;
    ch = NULL;
  }
  if (ebuf) {
    delete ebuf;
    ebuf = NULL;
  }
  if (rbuf) {
    delete rbuf;
    rbuf = NULL;
  }

out:

  if (chstats)
    free(chstats);
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
