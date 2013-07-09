/**
 * @file hlt_out_writer.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-06-20
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
#include <sys/shm.h>

#include <librorc.h>
#include <event_handling.h>
#include <event_generation.h>

using namespace std;


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
    unsigned long last_bytes_received;
    unsigned long last_events_received;
    uint64_t EventSize;
    uint32_t ChannelId;
    uint64_t ebuf_fill_state;
    uint64_t nevents;


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
    if (argc!=3 ) {
        printf("wrong argument count %d\n"
                "usage: %s [ChannelId] [EventSize]\n"
                "where [EventSize] is specified as number of DWs "
                "and has to be >=8 DWs\n",
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

    // get EventSize
    EventSize = strtoul(argv[2], NULL, 0);
    if ((errno == ERANGE && EventSize == ULONG_MAX)
            || (errno != 0 && EventSize== 0)
            || EventSize<8 ) {
        perror("illegal EventSize");
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
    try{ dev = new librorc::device(0); }
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
                1, LIBRORC_DMA_TO_DEVICE)!=0 ) {
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
    printf("EventBuffer size: 0x%lx bytes\n", EBUFSIZE);

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
    printf("ReportBuffer size: 0x%lx bytes\n", RBUFSIZE);


    memset(chstats, 0, sizeof(struct ch_stats));
    chstats->index = 0;
    chstats->last_id = -1;
    chstats->channel = (uint32_t)ChannelId;


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
    result = ch->configureChannel(ebuf, rbuf, 512);
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

    //TODO: all SIU interface handling

    // capture starting time
    bar1->gettime(&start_time, 0);
    last_time = start_time;
    cur_time = start_time;

    last_bytes_received = 0;
    last_events_received = 0;

    // no event in EB now
    ebuf_fill_state = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    // wait for RB entry
    while(!done)
    {
        nevents = fill_eventbuffer(
                rbuf, //report buffer instance
                ebuf, //event buffer instance
                ch, //channel instance
                &ebuf_fill_state, // event buffer fill state
                EventSize // event size to be used for event generation
                );
        if ( nevents > 0 )
        {
            printf("Pushed %ld events into EB\n", nevents);
        }

        result = handle_channel_data(
                rbuf, 
                ebuf, 
                ch, // channe struct
                chstats, // stats struct
                0, // do sanity check
                NULL, // no DDL reference file
                0); //DDL reference size

        if (result<0)
        {
            printf("handle_channel_data failed for channel %d\n", 
                    ChannelId);
        } else if (result==0)
        {
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

    if (shm) 
    {
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