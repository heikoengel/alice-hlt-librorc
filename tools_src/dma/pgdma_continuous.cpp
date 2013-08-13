/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
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

#include <librorc.h>
#include <event_handling.h>

#include "dma_handling.hh"

using namespace std;

int done = 0;

void abort_handler( int s )
{
    printf("Caught signal %d\n", s);
    if( done==1 )
    { exit(-1); }
    else
    { done = 1; }
}



int main( int argc, char *argv[])
{
    int result = 0;

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
                printf(HELP_TEXT, argv[0]);
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
        printf(HELP_TEXT, argv[0]);
        exit(-1);
    }

    if( ChannelId < 0 || ChannelId > MAX_CHANNEL )
    {
        cout << "ChannelId invalid or not set: " << ChannelId << endl;
        printf(HELP_TEXT, argv[0]);
        exit(-1);
    }

    if( EventSize == 0 )
    {
        cout << "EventSize invalid or not set: 0x" << hex
             << EventSize << endl;
        printf(HELP_TEXT, argv[0]);
        exit(-1);
    }


    /** catch CTRL+C for abort */
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = abort_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;


    /** Shared memory for DMA monitoring */
    /** Allocate shared mem */
    int shID =
        shmget( (SHM_KEY_OFFSET + DeviceId*SHM_DEV_OFFSET + ChannelId),
                sizeof(struct ch_stats), IPC_CREAT | 0666);
    if(shID==-1)
    {
        perror("shmget");
        abort();
    }

    /** Attach to shared memory */
    char *shm = (char*)shmat(shID, 0, 0);
    if (shm==(char*)-1)
    {
        perror("shmat");
        abort();
    }
    struct ch_stats *chstats
        = (struct ch_stats*)shm;

    /** Wipe SHM */
    memset(chstats, 0, sizeof(struct ch_stats));
    chstats->index = 0;
    chstats->last_id = -1;
    chstats->channel = (unsigned int)ChannelId;


    /** Create new device instance */
    librorc::device *dev;
    try{ dev = new librorc::device(DeviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }

    printf("Bus %x, Slot %x, Func %x\n", dev->getBus(), dev->getSlot(),dev->getFunc());

    /** bind to BAR1 */
    librorc::bar *bar1 = NULL;
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
        abort();
    }

    printf("FirmwareDate: %08x\n", bar1->get32(RORC_REG_FIRMWARE_DATE));

    /** Check if requested channel is implemented in firmware */
    if( ChannelId >= (int32_t)(bar1->get32(RORC_REG_TYPE_CHANNELS) & 0xffff) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
               "firmware - exiting\n", ChannelId);
        abort();
    }


    /** Create new DMA event buffer */
    librorc::buffer *ebuf;
    try
    { ebuf = new librorc::buffer(dev, EBUFSIZE, 2*ChannelId, 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: ebuf->allocate");
        abort();
    }


    /** Create new DMA report buffer */
    librorc::buffer *rbuf;
    try
    { rbuf = new librorc::buffer(dev, RBUFSIZE, 2*ChannelId+1, 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: rbuf->allocate");
        abort();
    }

    /* clear report buffer */
    struct rorcfs_event_descriptor *reportbuffer
        = (struct rorcfs_event_descriptor *)rbuf->getMem();
    memset(reportbuffer, 0, rbuf->getMappingSize());


    /** Create DMA channel and bind channel to BAR1 */
    librorc::dma_channel *ch = new librorc::dma_channel();
    ch->init(bar1, ChannelId);

    /* Prepare EventBufferDescriptorManager with scatter-gather list */
    result = ch->prepareEB( ebuf );
    if(result < 0)
    {
        perror("prepareEB()");
        result = -1;
        abort();
    }

    /* Prepare ReportBufferDescriptorManager with scatter-gather list */
    result = ch->prepareRB( rbuf );
    if(result < 0)
    {
        perror("prepareRB()");
        result = -1;
        abort();
    }

    /* Aet MAX_PAYLOAD, buffer sizes, #sgEntries, ... */
    result = ch->configureChannel(ebuf, rbuf, 128);
    if (result < 0)
    {
        perror("configureChannel()");
        result = -1;
        abort();
    }


    /** Enable Buffer Description Managers (BDMs) */
    ch->setEnableEB(1);
    ch->setEnableRB(1);

    /** Enable DMA channel */
    ch->setDMAConfig( ch->getDMAConfig() | 0x01 );


    /**
     * wait for GTX domain to be ready
     * read asynchronous GTX status
     * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
     * & !gtx_in_rst
     **/
    printf("Waiting for GTX to be ready...\n");
    while( (ch->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
    {
        usleep(100);
    }

    /** Configure Pattern Generator */
    //TODO: refactor this into sepparate methods
    ch->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, EventSize);
    //set PG mode
    ch->setGTX(RORC_REG_DDL_CTRL, ch->getGTX(RORC_REG_DDL_CTRL) | 0x600);
    //enable PG
    ch->setGTX(RORC_REG_DDL_CTRL, ch->getGTX(RORC_REG_DDL_CTRL) | 0x100);


    /** capture starting time */
    timeval start_time;
        bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;

    /**
     *  This can be aborted by abort_handler() ... */
    sigaction(SIGINT, &sigIntHandler, NULL);
    while( !done )
    {
    /** ... triggered from CTRL+C                  */

        result = handle_channel_data
        (
            rbuf,
            ebuf,
            ch,      /** channel struct     */
            chstats, /** stats struct       */
            0xff,    /** do sanity check    */
            NULL,    /** no reference DDL   */
            0        /** reference DDL size */
        );

        if( result < 0 )
        {
            printf("handle_channel_data failed for channel %d\n", ChannelId);
            abort();
        }
        else if( result==0 )
        {
            /** no events available */
            usleep(100);
        }

        bar1->gettime(&cur_time, 0);

        /** print status line each second */
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL)
        {
            printf
            (
                "Events: %10ld, DataSize: %8.3f GB",
                chstats->n_events,
                (double)chstats->bytes_received/(double)(1<<30)
            );

            if( chstats->bytes_received-last_bytes_received )
            {
                printf
                (
                    " DataRate: %9.3f MB/s",
                    (double)(chstats->bytes_received-last_bytes_received)/
                    gettimeofday_diff(last_time, cur_time)/(double)(1<<20)
                );
            }
            else
            {
                printf(" DataRate: -");
            }

            if( chstats->n_events - last_events_received)
            {
                printf
                (
                    " EventRate: %9.3f kHz/s",
                    (double)(chstats->n_events-last_events_received)/
                    gettimeofday_diff(last_time, cur_time)/1000.0
                );
            }
            else
            {
                printf(" EventRate: -");
            }

            printf(" Errors: %ld\n", chstats->error_count);
            last_time = cur_time;
            last_bytes_received = chstats->bytes_received;
            last_events_received = chstats->n_events;
        }
    }

    timeval end_time;
    bar1->gettime(&end_time, 0);
    printf
    (
        "%ld Byte / %ld events in %.2f sec -> %.1f MB/s.\n",
        chstats->bytes_received,
        chstats->n_events,
        gettimeofday_diff(start_time, end_time),
        ((float)chstats->bytes_received/gettimeofday_diff(start_time, end_time))/(float)(1<<20)
    );

    if(!chstats->set_offset_count)
    {
        printf("CH%d: No Events\n", ChannelId);
    }
    else
    {
        printf
        (
            "CH%d: Events %ld, max_epi=%ld, min_epi=%ld, avg_epi=%ld, set_offset_count=%ld\n",
            ChannelId,
            chstats->n_events,
            chstats->max_epi,
            chstats->min_epi,
            chstats->n_events/chstats->set_offset_count,
            chstats->set_offset_count
        );
    }

    /** Disable PG */
    ch->setGTX(RORC_REG_DDL_CTRL, 0x0);

    /** Disable DMA Engine */
    ch->setEnableEB(0);

    /** Wait for pending transfers to complete (dma_busy->0) */
    while( ch->getDMABusy() )
    {
        usleep(100);
    }

    /** Disable RBDM */
    ch->setEnableRB(0);

    /** Reset DFIFO, disable DMA PKT */
    ch->setDMAConfig(0X00000002);

    /** Clear reportbuffer */
    memset(reportbuffer, 0, rbuf->getMappingSize());


    return result;
}
