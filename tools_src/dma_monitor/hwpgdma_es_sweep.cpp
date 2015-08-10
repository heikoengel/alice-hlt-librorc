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

#include <cstdio>
#include <sys/shm.h>
#include <unistd.h>
#include <math.h>
#include <librorc.h>

using namespace std;

#define HELP_TEXT "dma_monitor usage: \n\
        dma_monitor [parameters] \n\
parameters: \n\
        -n [0..255] Source device ID \n\
        -h          Show this text\n"

#define TIME_ES_TO_STAT 1
#define TIME_STAT_TO_STAT 1
#define ITERATIONS_PER_ES 5
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
 * set EventSizeDW for PatternGenerator DMA Channel
 * */
int
setEventSizeDW
(
    librorc::bar *bar,
    uint32_t ChannelId,
    uint32_t EventSizeDW
)
{ 
    /** get current link */
    librorc::link *current_link
        = new librorc::link(bar, ChannelId);

    /** make sure DDL domain is up and running */
    if ( !current_link->isDdlDomainReady() )
    {
        return -1;
    }
    
    /** set new EventSizeDW
     *  '-1' because EOE is attached to each event. Without '-1' but 
     *  EventSizeDWs aligned to the max payload size boundaries a new full
     *  packet is sent containing only the EOE word. -> would take
     *  bandwidth but would not appear in number of bytes transferred
     *  */
    current_link->setDdlReg(RORC_REG_DDL_PG_EVENT_LENGTH, EventSizeDW-1);

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
nextEventSizeDW
(
    uint32_t EventSizeDW
)
{
    for ( uint32_t i=0; i<32; i++ )
    {
        uint32_t es = (EventSizeDW>>i);
        switch (es)
        {
            case 2:
            case 3:
                return EventSizeDW + (1<<(i-1));
            case 5:
                return EventSizeDW + (1<<i);
            case 7:
                return ( 1<<(i+3) );
            default:
                break;
        }
    }
    //TODO
    return 0;
}

uint32_t nextMpsEventSizeDW( uint32_t EventSizeDW, uint32_t maxPayloadSize ) {
    uint32_t numPkts = (EventSizeDW<<2) / maxPayloadSize;
    uint32_t nextNumPkts = 2 * numPkts;
    if ( nextNumPkts == numPkts ) {
        nextNumPkts++;
    }
    return ((nextNumPkts * maxPayloadSize)>>2);
}

double average( double *values, int num ) {
    double avg = 0.0;
    for (int i=0; i<num; i++) {
        avg += values[i];
    }
    return avg/num;
}

double variance( double *values, double avg, int num) {
    double var = 0;
    for (int i=0; i<num; i++) {
        var += (values[i]-avg)*(values[i]-avg);
    }
    return sqrt(var);
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
        log_fd = fopen(fname, "w+");
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
        bar = new librorc::bar(dev, 1);
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

    uint32_t maxPayloadSize = dev->maxPayloadSize();

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

    if (fname) {
        fprintf(log_fd, "# numPciePkts, EventSizeBytes, ebrate_avg, ebrate_var, rbrate_avg, rbrate_var, totalrate_avg, totalrate_var\n");
    }

    for ( uint32_t EventSizeDW=64;
            EventSizeDW<0x800000;
            EventSizeDW=nextMpsEventSizeDW(EventSizeDW, maxPayloadSize) )
            //EventSizeDW=nextEventSizeDW(EventSizeDW) )
    {
        /** set new EventSizeDW for all channels */
        for ( chID=startChannel; chID<=endChannel; chID++ )
        {
            if ( setEventSizeDW(bar, chID, EventSizeDW) < 0 )
            {
                cout << "ERROR: Failed to set EventSizeDW for Channel"
                     << chID << endl;
            }
        }

        /** wait some time */
        sleep(TIME_ES_TO_STAT);

        /** get a snapshot with timestamp */
        tChannelSnapshot last_status = getSnapshot(chstats);

        double ebrate[ITERATIONS_PER_ES];
        double rbrate[ITERATIONS_PER_ES];
        double totalrate[ITERATIONS_PER_ES];
        uint64_t EventSizeBytes = ((EventSizeDW) << 2);
        uint64_t numPciePkts = (EventSizeBytes % maxPayloadSize) ?
                (EventSizeBytes/maxPayloadSize+1) : (EventSizeBytes/maxPayloadSize);

        for ( int itCount=0; itCount<ITERATIONS_PER_ES; itCount++ )
        {
            sleep(TIME_STAT_TO_STAT);
            tChannelSnapshot cur_status = getSnapshot(chstats);
            tSnapshotDiff diff = getSnapshotDiff(last_status, cur_status);

            ebrate[itCount] = ((double)diff.bytes/1024.0/1024.0)/diff.time;
            rbrate[itCount] = ((double)diff.events*16.0/1024.0/1024.0)/diff.time;
            totalrate[itCount] = ebrate[itCount] + rbrate[itCount];

            /** output */
            //cout << EventSizeTPC << ", " << ebrate  << ", " << rbrate << endl;
            printf("%ld, %ld, %f, %f, %f\n", numPciePkts, EventSizeBytes,
                   ebrate[itCount], rbrate[itCount], totalrate[itCount]);

            last_status = cur_status;
        }
        if ( fname )
        {
            double ebrate_avg = average(ebrate, ITERATIONS_PER_ES);
            double ebrate_var = variance(ebrate, ebrate_avg, ITERATIONS_PER_ES);
            double rbrate_avg = average(rbrate, ITERATIONS_PER_ES);
            double rbrate_var = variance(rbrate, rbrate_avg, ITERATIONS_PER_ES);
            double totalrate_avg = average(totalrate, ITERATIONS_PER_ES);
            double totalrate_var = variance(totalrate, totalrate_avg, ITERATIONS_PER_ES);

            fprintf(log_fd, "%ld, %ld, %f, %f, %f, %f, %f, %f\n", numPciePkts, EventSizeBytes,
                    ebrate_avg, ebrate_var, rbrate_avg, rbrate_var, totalrate_avg, totalrate_var);
            printf("%ld, %ld, %f, %f, %f, %f, %f, %f\n", numPciePkts, EventSizeBytes,
                   ebrate_avg, ebrate_var, rbrate_avg, rbrate_var, totalrate_avg, totalrate_var);

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


