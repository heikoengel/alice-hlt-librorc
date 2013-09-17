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
#include <getopt.h>

#include <librorc.h>
#include "event_handling.h"
#include "event_generation.h"

using namespace std;

#define HELP_TEXT "hlt_out_writer usage: \n\
        hlt_out_writer [parameters] \n\
parameters: \n\
        --device [0..255] Destination device ID \n\
        --channel [0..11] Destination DMA channel \n\
        --size [value]    Event Size in DWs \n\
        --help            Show this text \n"

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
    librorc::device      *dev  = NULL;
    librorc::bar         *bar1 = NULL;
    librorc::buffer      *ebuf = NULL;
    librorc::buffer      *rbuf = NULL;
    librorc::dma_channel *ch   = NULL;

    timeval start_time, end_time;
    timeval last_time, cur_time;
    unsigned long last_bytes_received;
    unsigned long last_events_received;
    uint64_t ebuf_fill_state;
    uint64_t nevents;

    /** command line arguments */
    // TODO : this is bad because it fails if the struct changes
    static struct option long_options[] =
    {
        {"device", required_argument, 0, 'd'},
        {"channel", required_argument, 0, 'c'},
        {"size", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /** parse command line arguments **/
    int32_t  DeviceId  = -1;
    int32_t  ChannelId = -1;
    uint32_t EventSize = 0;
    while(1)
    {
        int opt = getopt_long(argc, argv, "", long_options, NULL);
        if ( opt == -1 )
        { break; }

        switch(opt)
        {
            case 'd':
            {
                DeviceId = strtol(optarg, NULL, 0);
            }
            break;

            case 'c':
            {
                ChannelId = strtol(optarg, NULL, 0);
            }
            break;

            case 's':
            {
                EventSize = strtol(optarg, NULL, 0);
            }
            break;

            case 'h':
            {
                cout << HELP_TEXT;
                exit(0);
            }
            break;

            default:
            {
                break;
            }
        }
    }

    /** sanity checks on command line arguments **/
    if( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "DeviceId invalid or not set: " << DeviceId << endl;
        cout << HELP_TEXT;
        exit(-1);
    }

    if( ChannelId < 0 || ChannelId > MAX_CHANNEL )
    {
        cout << "ChannelId invalid or not set: " << ChannelId << endl;
        cout << HELP_TEXT;
        exit(-1);
    }

    if( EventSize == 0 )
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
    librorcChannelStatus *chstats = NULL;
    char *shm = NULL;

    //allocate shared mem
    shID = shmget(SHM_KEY_OFFSET + DeviceId*SHM_DEV_OFFSET + ChannelId,
            sizeof(librorcChannelStatus), IPC_CREAT | 0666);
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
    chstats = (librorcChannelStatus*)shm;


    // create new device instance
    try{ dev = new librorc::device(DeviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        goto out;
    }

    /** Print some stats */
    printf("Bus %x, Slot %x, Func %x\n",
            dev->getBus(),
            dev->getSlot(),
            dev->getFunc());

    // bind to BAR1
    try
    {
    #ifdef SIM
        bar1 = new librorc::sim_bar(dev, 1);
    #else
        bar1 = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR1.\n");
        goto out;
    }

    bar1->simSetPacketSize(32);

    try
    {
        librorc::sysmon *sm = new librorc::sysmon(bar1);
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setw(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setw(8) << sm->FwBuildDate() << dec << endl;
        delete sm;
    }
    catch(...)
    { cout << "Firmware Rev. and Date not available!" << endl; }

    /** Check if requested channel is implemented in firmware */
    if( !dev->DMAChannelIsImplemented(ChannelId) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
               "firmware - exiting\n", ChannelId);
        return(-1);
    }

    // check if firmware is HLT_OUT
    if ( (bar1->get32(RORC_REG_TYPE_CHANNELS)>>16) != RORC_CFG_PROJECT_hlt_out )
    {
        cout << "Firmware is not HLT_OUT - exiting." << endl;
        goto out;
    }

    /** create new DMA event buffer */
    try
    { ebuf = new librorc::buffer(dev, EBUFSIZE, 2*ChannelId, 1, LIBRORC_DMA_TO_DEVICE); }
    catch(...)
    {
        perror("ERROR: ebuf->allocate");
        goto out;
    }
    printf("EventBuffer size: 0x%lx bytes\n", EBUFSIZE);

    /** create new DMA report buffer */
    try
    { rbuf = new librorc::buffer(dev, RBUFSIZE, 2*ChannelId+1, 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: rbuf->allocate");
        goto out;
    }
    printf("ReportBuffer size: 0x%lx bytes\n", RBUFSIZE);


    memset(chstats, 0, sizeof(librorcChannelStatus));
    chstats->index = 0;
    chstats->last_id = -1;
    chstats->channel = (uint32_t)ChannelId;


    /** Create DMA channel */
    try
    {
        ch = new librorc::dma_channel(ChannelId, 64, dev, bar1, ebuf, rbuf);
        ch->enable();
    }
    catch(...)
    {
        cout << "DMA channel failed!" << endl;
        abort();
    }

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
            DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                    "Pushed %ld events into EB\n", nevents);
        }

        result = handle_channel_data(
                rbuf,
                ebuf,
                ch, // channe struct
                chstats, // stats struct
                CHK_SIZES|CHK_PATTERN|CHK_SOE, // do sanity check
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
    ch->disableEventBuffer();

    // wait for pending transfers to complete (dma_busy->0)
    while( ch->getDMABusy() )
        usleep(100);

    // disable RBDM
    ch->enableReportBuffer();

    // reset DFIFO, disable DMA PKT
    ch->setDMAConfig(0X00000002);

    // clear reportbuffer
    memset(rbuf->getMem(), 0, rbuf->getMappingSize());


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
