#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "librorc.h"
#include "event_handling.h"

using namespace std;

#ifndef SIM

// EventBuffer size in bytes
#define EBUFSIZE (((unsigned long)1) << 30)
// ReportBuffer size in bytes
#define RBUFSIZE (((unsigned long)1) << 28)
// number of bytes to transfer
#define N_BYTES_TRANSFER (((unsigned long)1) << 35)

#else

// EventBuffer size in bytes
#define EBUFSIZE (((unsigned long)1) << 19)
// ReportBuffer size in bytes
#define RBUFSIZE (((unsigned long)1) << 17)
// number of bytes to transfer
#define N_BYTES_TRANSFER (((unsigned long)1) << 17)

#endif

#define CHANNELS 1


int main( int argc, char *argv[])
{
  int result = 0;
  unsigned long i;
  rorcfs_device *dev = NULL;
  rorcfs_bar *bar1 = NULL;
  rorcfs_buffer *ebuf[CHANNELS];
  rorcfs_buffer *rbuf[CHANNELS];
  rorcfs_dma_channel *ch[CHANNELS];

  struct rorcfs_event_descriptor *reportbuffer[CHANNELS];
  unsigned int *eventbuffer[CHANNELS];
  timeval start_time, end_time;
  struct ch_stats *chstats[CHANNELS];
  unsigned long bytes_received, events_received;
  unsigned long EventSize;

  if (argc!=2 ) {
    printf("wrong argument count %d\n"
        "usage: %s [EventSize]\n"
        "where [EventSize] is the number of DWs sent after"
        "the CommonDataHeader (CDH). The CDH itself consists of"
        "8 DWs\n",
        argc, argv[0]);
    result = -1;
    exit(-1);
  }

  EventSize = strtoul(argv[1], NULL, 0);
  if ((errno == ERANGE && EventSize == ULONG_MAX)
      || (errno != 0 && EventSize== 0)) {
    perror("illegal EventSize");
    result = -1;
    exit(-1);
  }



  for(i=0;i<CHANNELS;i++) {
    ebuf[i]=NULL;
    rbuf[i]=NULL;
    ch[i]=NULL;
    chstats[i]=NULL;
  }

  // create new device class
  dev = new rorcfs_device();
  if ( dev->init(0) == -1 ) {
    cout << "failed to initialize device 0" << endl;
    goto out;
  }

  printf("Bus %x, Slot %x, Func %x\n", dev->getBus(),
      dev->getSlot(),dev->getFunc());

  // bind to BAR1
  bar1 = new rorcfs_bar(dev, 1);
  if ( bar1->init() == -1 ) {
    cout<<"BAR1 init failed\n";
    goto out;
  }

  printf("FirmwareDate: %08x\n", bar1->get(RORC_REG_FIRMWARE_DATE));

  for(i=0;i<CHANNELS;i++) {

    // create new DMA event buffer
    ebuf[i] = new rorcfs_buffer();
    if ( ebuf[i]->allocate(dev, EBUFSIZE, 2*i,
          1, RORCFS_DMA_FROM_DEVICE)!=0 ) {
      if ( errno == EEXIST ) {
        if ( ebuf[i]->connect(dev, 2*i) != 0 ) {
          perror("ERROR: ebuf->connect");
          goto out;
        }
      } else {
        perror("ERROR: ebuf->allocate");
        goto out;
      }
    }

    // create new DMA report buffer
    rbuf[i] = new rorcfs_buffer();;
    if ( rbuf[i]->allocate(dev, RBUFSIZE, 2*i+1,
          1, RORCFS_DMA_FROM_DEVICE)!=0 ) {
      if ( errno == EEXIST ) {
        //printf("INFO: Buffer already exists, trying to connect...\n");
        if ( rbuf[i]->connect(dev, 2*i+1) != 0 ) {
          perror("ERROR: rbuf->connect");
          goto out;
        }
      } else {
        perror("ERROR: rbuf->allocate");
        goto out;
      }
    }

    chstats[i] = (struct ch_stats*)malloc(sizeof(struct ch_stats));
    if ( chstats[i]==NULL ){
      perror("alloc chstats");
      result=-1;
      goto out;
    }

    memset(chstats[i], 0, sizeof(struct ch_stats));

    chstats[i]->index = 0;
    chstats[i]->last_id = -1;
    chstats[i]->channel = (unsigned int)i;


    // create DMA channel
    ch[i] = new rorcfs_dma_channel();

    // bind channel to BAR1, channel offset 0
    ch[i]->init(bar1, (i+1)*RORC_CHANNEL_OFFSET);

    // prepare EventBufferDescriptorManager
    // with scatter-gather list
    result = ch[i]->prepareEB( ebuf[i] );
    if (result < 0) {
      perror("prepareEB()");
      result = -1;
      goto out;
    }

    // prepare ReportBufferDescriptorManager
    // with scatter-gather list
    result = ch[i]->prepareRB( rbuf[i] );
    if (result < 0) {
      perror("prepareRB()");
      result = -1;
      goto out;
    }

    result = ch[i]->configureChannel(ebuf[i], rbuf[i], 128, 256);
    if (result < 0) {
      perror("configureChannel()");
      result = -1;
      goto out;
    }

    /* clear report buffer */
    reportbuffer[i] = (struct rorcfs_event_descriptor *)rbuf[i]->getMem();
    printf("pRBUF=%p, MappingSize=%ld\n",
        rbuf[i]->getMem(), rbuf[i]->getMappingSize() );
    memset(reportbuffer[i], 0, rbuf[i]->getMappingSize());

    eventbuffer[i] = (unsigned int *)ebuf[i]->getMem();

  }

  // create DIU interface

  // wait for GTX domain to be ready:
  for(i=0;i<CHANNELS;i++) {
    // read asynchronous GTX status
    // wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
    // & !gtx_in_rst
    while( (ch[i]->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
      usleep(100);

    ch[i]->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, EventSize);
    ch[i]->setGTX(RORC_REG_DDL_CTRL,
        ch[i]->getGTX(RORC_REG_DDL_CTRL) | 0x600); //set PG mode
    ch[i]->setGTX(RORC_REG_DDL_CTRL,
        ch[i]->getGTX(RORC_REG_DDL_CTRL) | 0x100); //enable PG

    ch[i]->setEnableEB(1);
    ch[i]->setEnableRB(1);
  }

  //bar1->gettime(&start_time, 0);
  gettimeofday(&start_time, 0);

  for(i=0;i<CHANNELS;i++) {
    // enable DMA channel
    ch[i]->setDMAConfig( ch[i]->getDMAConfig() | 0x01 );
  }


  i = 0;

  while( 1 ) {

    for(i=0;i<CHANNELS;i++) {
      result = handle_channel_data(
          reportbuffer[i],
          eventbuffer[i],
          ch[i],
          chstats[i],
          rbuf[i]->getPhysicalSize(),
          rbuf[i]->getMaxRBEntries(),
          1); // do sanity checks
      if ( result < 0 ) {
        printf("handle_channel_data failed for channel %ld\n", i);
        goto out;
      }
    }


    // sum stats from all channels
    bytes_received = 0;
    events_received = 0;
    for(i=0;i<CHANNELS;i++) {
      bytes_received += chstats[i]->bytes_received;
      events_received += chstats[i]->n_events;
    }

    // check break condition
    if(bytes_received > N_BYTES_TRANSFER) {
      //bar1->gettime(&end_time, 0);
      gettimeofday(&end_time, 0);

      printf("%ld Byte / %ld events in %.2f sec"
          "-> %.1f MB/s.\n",
          (bytes_received), events_received,
          gettimeofday_diff(start_time, end_time),
          ((float)bytes_received/
           gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

      for(i=0;i<CHANNELS;i++) {
        if(!chstats[i]->set_offset_count) //avoid DivByZero Exception
          printf("CH%ld: No Events\n", i);
        else
          printf("CH%ld: Events %ld, max_epi=%ld, min_epi=%ld, "
              "avg_epi=%ld, set_offset_count=%ld\n", i,
              chstats[i]->n_events, chstats[i]->max_epi,
              chstats[i]->min_epi,
              chstats[i]->n_events/chstats[i]->set_offset_count,
              chstats[i]->set_offset_count);
      }
      printf("\n");

      break;
    } else
      usleep(100);

  }

  for(i=0;i<CHANNELS;i++) {
    //disable PG
    ch[i]->setGTX(RORC_REG_DDL_CTRL, 0x0); //disable PG
    // disable DMA Engine
    ch[i]->setEnableEB(0);
  }

  for(i=0;i<CHANNELS;i++) {

    // wait for pending transfers to complete (dma_busy->0)
    while( ch[i]->getDMABusy() )
      usleep(100);

    // disable RBDM
    ch[i]->setEnableRB(0);

    // reset DFIFO, disable DMA PKT
    ch[i]->setDMAConfig(0X00000002);

    if (chstats[i]) {
      free(chstats[i]);
      chstats[i] = NULL;
    }
    if (ch[i]) {
      delete ch[i];
      ch[i] = NULL;
    }
    if (ebuf[i]) {
      delete ebuf[i];
      ebuf[i] = NULL;
    }
    if (rbuf[i]) {
      delete rbuf[i];
      rbuf[i] = NULL;
    }
  }

out:

  for(i=0;i<CHANNELS;i++) {
    if (chstats[i])
      free(chstats[i]);
    if (ch[i])
      delete ch[i];
    if (ebuf[i])
      delete ebuf[i];
    if (rbuf[i])
      delete rbuf[i];
  }

  if (bar1)
    delete bar1;
  if (dev)
    delete dev;

  return result;
}
