/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

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
#define LIBRORC_MAX_DMA_CHANNELS 12


typedef struct {
    librorc::ChannelStatus chstats[LIBRORC_MAX_DMA_CHANNELS];
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
    current_link->setDdlReg(RORC_REG_DDL_PG_EVENT_LENGTH, EventSize-1);

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
    librorc::ChannelStatus *chstats[LIBRORC_MAX_DMA_CHANNELS]
)
{
    tChannelSnapshot chss;

    // capture current time
    gettimeofday(&(chss.time), 0);

    for ( int i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++ )
    {
        memcpy(&(chss.chstats[i]), chstats[i], 
                sizeof(librorc::ChannelStatus));
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
    sd.time = librorc::gettimeofdayDiff(last.time, current.time);
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
    librorc::ChannelStatus *chstats[LIBRORC_MAX_DMA_CHANNELS];
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
                sizeof(librorc::ChannelStatus), IPC_CREAT | 0666);
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
        chstats[chID] = (librorc::ChannelStatus*)shm[chID];
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
    #ifdef MODELSIM
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

    librorc::sysmon *sm = NULL;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize Sysmon." << endl;
        delete bar;
        delete dev;
        abort();
    }

    /** make sure FW is HLT_IN */
    if ( !sm->firmwareIsHltIn() && !sm->firmwareIsHltHardwareTest() )
    {
        cout << "ERROR: No HLT_IN firmware detected - aborting." << endl;
        delete sm;
        delete bar;
        delete dev;
        abort();
    }
        
    uint32_t startChannel = 0;
    uint32_t endChannel = sm->numberOfChannels() - 1;

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

    delete sm;
    delete bar;
    delete dev;

    return 0;
}


