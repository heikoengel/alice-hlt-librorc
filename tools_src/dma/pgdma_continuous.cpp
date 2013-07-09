/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2012-11-14
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
 * Open DMA Channel sourced by PatternGenerator
 *
 **/

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
#include <sys/mman.h>
#include <fcntl.h>
#include <getopt.h>

#include <librorc.h>
#include <event_handling.h>

using namespace std;

#define HELP_TEXT "pgdma_continuous usage: \n\
        pgdma_continuous [parameters] \n\
parameters: \n\
        --device [0..255] Source device ID \n\
        --channel [0..11] Source DMA channel \n\
        --size [value]    PatternGenerator Event Size in DWs \n"


/** Buffer Sizes (in Bytes) **/
#ifndef SIM
#define EBUFSIZE (((uint64_t)1) << 28)
#define RBUFSIZE (((uint64_t)1) << 26)
#define STAT_INTERVAL 1.0
#else
#define EBUFSIZE (((uint64_t)1) << 19)
#define RBUFSIZE (((uint64_t)1) << 17)
#define STAT_INTERVAL 0.00001
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


/**
 * Command line arguments
 * argv[1]: Channel Number
 * argv[2]: Event Fragment Size
 * */
int main( int argc, char *argv[])
{
  int result = 0;
  librorc::device      *dev  = NULL;
  librorc::bar         *bar1 = NULL;
  librorc::buffer      *ebuf = NULL;
  librorc::buffer      *rbuf = NULL;
  librorc::dma_channel *ch   = NULL;

  struct rorcfs_event_descriptor *reportbuffer = NULL;
  timeval start_time, end_time;
  timeval last_time, cur_time;
  uint64_t last_bytes_received;
  uint64_t last_events_received;

  int32_t DeviceId = -1;
  int32_t ChannelId = -1;
  uint32_t EventSize = 0;

  // command line arguments
  static struct option long_options[] = {
      {"device", required_argument, 0, 'd'},
      {"channel", required_argument, 0, 'c'},
      {"size", required_argument, 0, 's'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
  };

  /** parse command line arguments **/
  while (1)
  {
      int opt = getopt_long(argc, argv, "", long_options, NULL);
      if ( opt == -1 )
      {
          break;
      }

      switch(opt)
      {
          case 'd':
              DeviceId = strtol(optarg, NULL, 0);
              break;
          case 'c':
              ChannelId = strtol(optarg, NULL, 0);
              break;
          case 's':
              EventSize = strtol(optarg, NULL, 0);
              break;
          case 'h':
              cout << HELP_TEXT;
              break;
          default:
              break;
      }
  }

  /** sanity checks on command line arguments **/
  if ( DeviceId < 0 || DeviceId > 255 )
  {
      cout << "DeviceId invalid or not set: " << DeviceId << endl;
      cout << HELP_TEXT;
      exit(-1);
  }

  if ( ChannelId < 0 || ChannelId > MAX_CHANNEL)
  {
      cout << "ChannelId invalid or not set: " << ChannelId << endl;
      cout << HELP_TEXT;
      exit(-1);
  }

  if ( EventSize == 0)
  {
      cout << "EventSize invalid or not set: 0x" << hex 
           << EventSize << endl;
      cout << HELP_TEXT;
      exit(-1);
  }



  // catch CTRL+C for abort
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = abort_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  //shared memory
  int shID;
  struct ch_stats *chstats = NULL;
  char *shm = NULL;

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
  try{ dev = new librorc::device(DeviceId); }
  catch(...)
  {
    printf("ERROR: failed to initialize device.\n");
    goto out;
  }

  printf("Bus %x, Slot %x, Func %x\n", dev->getBus(),
      dev->getSlot(),dev->getFunc());

  // bind to BAR1
  #ifdef SIM
    bar1 = new librorc::sim_bar(dev, 1);
  #else
    bar1 = new librorc::rorc_bar(dev, 1);
  #endif
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
  ebuf = new librorc::buffer();
  if ( ebuf->allocate(dev, EBUFSIZE, 2*ChannelId,
        1, LIBRORC_DMA_FROM_DEVICE)!=0 ) {
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
  printf("EventBuffer:\n");
  //dump_sglist(ebuf);

  // create new DMA report buffer
  rbuf = new librorc::buffer();;
  if ( rbuf->allocate(dev, RBUFSIZE, 2*ChannelId+1,
        1, LIBRORC_DMA_FROM_DEVICE)!=0 ) {
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
  printf("ReportBuffer:\n");
  //dump_sglist(rbuf);


  memset(chstats, 0, sizeof(struct ch_stats));
  chstats->index = 0;
  chstats->last_id = -1;
  chstats->channel = (unsigned int)ChannelId;


  // create DMA channel
  ch = new librorc::dma_channel();

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
  result = ch->configureChannel(ebuf, rbuf, 128);
  if (result < 0) {
    perror("configureChannel()");
    result = -1;
    goto out;
  }


  /* clear report buffer */
  reportbuffer = (struct rorcfs_event_descriptor *)rbuf->getMem();
  memset(reportbuffer, 0, rbuf->getMappingSize());

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

  // Configure Pattern Generator
  ch->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, EventSize);
  ch->setGTX(RORC_REG_DDL_CTRL,
      ch->getGTX(RORC_REG_DDL_CTRL) | 0x600); //set PG mode
  ch->setGTX(RORC_REG_DDL_CTRL,
      ch->getGTX(RORC_REG_DDL_CTRL) | 0x100); //enable PG


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
        rbuf,
        ebuf,
        ch, // channe struct
        chstats, // stats struct
        0xff, // do sanity check
        NULL, // no reference DDL
        0); //reference DDL size

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
      printf("Events: %10ld, DataSize: %8.3f GB",
          chstats->n_events,
          (double)chstats->bytes_received/(double)(1<<30));

      if ( chstats->bytes_received-last_bytes_received)
      {
        printf(" DataRate: %9.3f MB/s",
            (double)(chstats->bytes_received-last_bytes_received)/
            gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
      } else {
        printf(" DataRate: -");
        //dump_dma_state(ch);
        //dump_diu_state(ch);
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
