/**
 * @file hwpgdma_es_sweep.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-10-22
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

#include <sys/shm.h>
#include <getopt.h>
#include <librorc.h>

using namespace std;

#define HELP_TEXT "dma_monitor usage: \n\
        dma_monitor [parameters] \n\
parameters: \n\
        -n [0..255] Source device ID \n\
        -h          Show this text\n"

#define TIME_ES_TO_STAT 1
#define TIME_STAT_TO_STAT 1
#define ITERATIONS_PER_ES 10


typedef struct {
    librorcChannelStatus chstats[LIBRORC_MAX_DMA_CHANNELS];
    timeval time;
} tChannelSnapshot;


typedef struct
{
    uint64_t bytes;
    uint64_t events;
    double time;
} tSnapshotDiff;


/**
 * set EventSize for PatternGenerator DMA Channel
 * */
int
setEventSize
(
    librorc::bar *bar,
    uint32_t ChannelId,
    uint32_t EventSize
)
{ 
    /** get current link */
    librorc::link *current_link
        = new librorc::link(bar, ChannelId);

    /** make sure GTX domain is up and running */
    if ( !current_link->isGtxDomainReady() )
    {
        return -1;
    }
    
    /** set new EventSize
     *  '-1' because EOE is attached to each event. Without '-1' but 
     *  EventSizes aligned to the max payload size boundaries a new full
     *  packet is sent containing only the EOE word. -> would take
     *  bandwidth but would not appear in number of bytes transferred
     *  */
    current_link->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, EventSize-1);

    delete current_link;
    return 0;
}


/**
 * get a snapshot with timestamp of the current
 * channel status struct
 * */
tChannelSnapshot
getSnapshot
(
    librorcChannelStatus *chstats[LIBRORC_MAX_DMA_CHANNELS]
)
{
    tChannelSnapshot chss;

    // capture current time
    gettimeofday(&(chss.time), 0);

    for ( int i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++ )
    {
        memcpy(&(chss.chstats[i]), chstats[i], 
                sizeof(librorcChannelStatus));
    }
    return chss;
}


/**
 * get the difference in bytes, events and time
 * between two snapshots
 * */
tSnapshotDiff
getSnapshotDiff
(
    tChannelSnapshot last,
    tChannelSnapshot current
)
{
    tSnapshotDiff sd;
    sd.bytes = 0;
    sd.events = 0;
    for(uint32_t chID=0; chID<LIBRORC_MAX_DMA_CHANNELS; chID++)
    {
        sd.bytes += (current.chstats[chID].bytes_received - 
                last.chstats[chID].bytes_received);
        sd.events += (current.chstats[chID].n_events -
                last.chstats[chID].n_events);
    }
    sd.time = gettimeofdayDiff(last.time, current.time);
    return sd;
}

uint32_t
nextEventSize
(
    uint32_t EventSize
)
{
    for ( uint32_t i=0; i<32; i++ )
    {
        uint32_t es = (EventSize>>i);
        switch (es)
        {
            case 2:
            case 3:
                return EventSize + (1<<(i-1));
                return EventSize + (1<<(i-1));
            case 5:
                return EventSize + (1<<i);
            case 7:
                return ( 1<<(i+3) );
            default:
                break;
        }
    }
    //TODO
    return 0;
}

int main( int argc, char *argv[])
{
    int32_t DeviceId = -1;
    librorcChannelStatus *chstats[LIBRORC_MAX_DMA_CHANNELS];
    int32_t        shID[LIBRORC_MAX_DMA_CHANNELS];
    char          *shm[LIBRORC_MAX_DMA_CHANNELS];
    uint32_t       chID;

    int arg;
    char *fname = NULL;

    /** parse command line arguments **/
    while ( (arg = getopt(argc, argv, "hn:f:")) != -1 )
    {
        switch(arg)
        {
            case 'n':
                DeviceId = strtol(optarg, NULL, 0);
                break;
            case 'f':
                fname = (char *)malloc(strlen(optarg));
                if ( fname == NULL )
                {
                    perror("malloc failed");
                    abort();
                }
                strcpy(fname, optarg);
                break;
            case 'h':
                cout << HELP_TEXT;
                return 0;
            default:
                cout << "Unknown parameter (" << arg << ")!" << endl;
                cout << HELP_TEXT;
                break;
        }
    }

    /** sanity checks on command line arguments **/
    if ( DeviceId < 0 )
    {
        cout << "DeviceId not set, using default device 0" << endl;
        DeviceId = 0;
    }
    else if ( DeviceId > 255 )
    {
        cout << "DeviceId invalid: " << DeviceId << endl;
        cout << HELP_TEXT;
        exit(-1);
    }

    /** Initialize shm channels */
    for(chID=0; chID<LIBRORC_MAX_DMA_CHANNELS; chID++)
    {

        shID[chID] = shmget(SHM_KEY_OFFSET + DeviceId*SHM_DEV_OFFSET + chID,
                sizeof(librorcChannelStatus), IPC_CREAT | 0666);
        if( shID[chID]==-1)
        {
            perror("shmget");
            abort();
        }

        shm[chID] = NULL;
        shm[chID] = (char *)shmat(shID[chID], NULL, SHM_RDONLY);
        if(shm[chID]==(char*)-1)
        {
            perror("shmat");
            abort();
        }

        chstats[chID] = NULL;
        chstats[chID] = (librorcChannelStatus*)shm[chID];
    }


    FILE *log_fd = NULL;

    /** open dump file for writing */
    if ( fname )
    {
        log_fd = fopen(fname, "a");
        if ( log_fd==NULL )
        {
            perror("Failed to open destination file");
            abort();
        }
    }



    /** Create new device instance */
    librorc::device *dev = NULL;
    try{ dev = new librorc::device(DeviceId); }
    catch(...)
    {
        cout << "ERROR: failed to initialize device " << DeviceId << endl;
        abort();
    }

    /** bind to BAR1 */
    librorc::bar *bar = NULL;
    try
    {
    #ifdef SIM
        bar = new librorc::sim_bar(dev, 1);
    #else
        bar = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    /** get number channels implemented in firmware */
    uint32_t type_channels = bar->get32(RORC_REG_TYPE_CHANNELS);

    /** make sure FW is HLT_IN */
    if ( (type_channels>>16) != RORC_CFG_PROJECT_hlt_in &&
    (type_channels>>16) != RORC_CFG_PROJECT_hwtest )
    {
        cout << "ERROR: No HLT_IN firmware detected - aborting." << endl;
        delete bar;
        delete dev;
        abort();
    }
        
    uint32_t startChannel = 0;
    uint32_t endChannel = (type_channels & 0xffff) - 1;

    for ( uint32_t EventSize=16; 
            EventSize<0x10000; 
            EventSize=nextEventSize(EventSize) )
    {
        /** set new EventSize for all channels */
        for ( chID=startChannel; chID<=endChannel; chID++ )
        {
            if ( setEventSize(bar, chID, EventSize) < 0 )
            {
                cout << "ERROR: Failed to set EventSize for Channel"
                     << chID << endl;
            }
        }

        /** wait some time */
        sleep(TIME_ES_TO_STAT);

        /** get a snapshot with timestamp */
        tChannelSnapshot last_status = getSnapshot(chstats);

        for ( int itCount=0; itCount<ITERATIONS_PER_ES; itCount++ )
        {
            /** wait some time */
            sleep(TIME_STAT_TO_STAT);

            /** get new snapshot */
            tChannelSnapshot cur_status = getSnapshot(chstats);

            /** get diff*/
            tSnapshotDiff diff = getSnapshotDiff(last_status, cur_status);

            /** get rate to EB */
            double ebrate = ((double)diff.bytes/1024.0/1024.0)/diff.time;
            
            /** get rate to RB */
            double rbrate = ((double)diff.events*16.0/1024.0/1024.0)/diff.time;


            /** EventSize is in DWs, multiply by 216 links */
            uint32_t EventSizeTpc = 216 * (EventSize<<2);

            /** output */
            cout << EventSizeTpc << ", " << ebrate  << ", " << rbrate << endl;
            if ( fname )
            {
                fprintf(log_fd, "%d, %f, %f\n", EventSizeTpc, ebrate, rbrate);
            }

            last_status = cur_status;
        }
    } /** for-loop: Event Sizes */


    /** Detach all the shared memory */
    for(int32_t i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++)
    {
        if( shm[i] != NULL )
        {
            shmdt(shm[i]);
            shm[i] = NULL;
        }
    }

    if ( fname )
    {
        fclose(log_fd);
        free(fname);
    }

    delete bar;
    delete dev;

    return 0;
}


