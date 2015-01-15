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

#define __STDC_LIMIT_MACROS
#include <iomanip>
#include <signal.h>
#include <cstdio>

#include <sys/shm.h>
#include <getopt.h>
#include <unistd.h>

#include <librorc.h>

using namespace std;

#define LIBRORC_MAX_DMA_CHANNELS 12

#define HELP_TEXT "dma_monitor usage: \n\
        dma_monitor [parameters] \n\
parameters: \n\
        --device [0..255] Source device ID \n\
        --help            Show this text\n"


uint32_t done = 0;

void abort_handler( int s )
{
    cout << "Caught signal " << s << endl;
    done = 1;
}


int main( int argc, char *argv[])
{
    int32_t DeviceId   = -1;
    int32_t Iterations =  INT32_MAX;

    // command line arguments
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"iterations", required_argument, 0, 'i'},
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

            case 'h':
                cout << HELP_TEXT;
                exit(0);
            break;

            case 'i':
                cout << "Running for " << optarg << " iterations!" << endl;
                Iterations = strtol(optarg, NULL, 0);
            break;

            default:
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


    /** catch CTRL+C for abort */
    struct sigaction sigIntHandler;
    {
        sigIntHandler.sa_handler = abort_handler;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;
    }
    sigaction(SIGINT, &sigIntHandler, NULL);

    /** Innitialize shm channels */
    uint64_t       last_bytes_received[LIBRORC_MAX_DMA_CHANNELS];
    uint64_t       last_events_received[LIBRORC_MAX_DMA_CHANNELS];
    uint64_t       channel_bytes[LIBRORC_MAX_DMA_CHANNELS];
    librorc::ChannelStatus *chstats[LIBRORC_MAX_DMA_CHANNELS];
    int32_t        shID[LIBRORC_MAX_DMA_CHANNELS];
    char          *shm[LIBRORC_MAX_DMA_CHANNELS];

    for(int32_t i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++)
    {
        last_bytes_received[i] = 0;
        last_events_received[i] = 0;
        channel_bytes[i] = 0;

        shID[i] = shmget(SHM_KEY_OFFSET + DeviceId*SHM_DEV_OFFSET + i,
                sizeof(librorc::ChannelStatus), IPC_CREAT | 0666);
        if( shID[i]==-1)
        {
            perror("shmget");
            abort();
        }

        shm[i] = NULL;
        shm[i] = (char *)shmat(shID[i], 0, 0);
        if(shm[i]==(char*)-1)
        {
            perror("shmat");
            abort();
        }

        chstats[i] = NULL;
        chstats[i] = (librorc::ChannelStatus*)shm[i];
    }

    /** capture starting time */
    timeval cur_time;
    gettimeofday(&cur_time, 0);
    timeval last_time = cur_time;

    int32_t iter = 0;
    while( (!done) && (iter < Iterations) )
    {
        gettimeofday(&cur_time, 0);

        /** print status line each second */
        for(int32_t i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++)
        {
            cout << "CH" << setw(2) << i << " - Events: "
                << setw(10) << chstats[i]->n_events << ", DataSize: "
                << setw(10) << (double)chstats[i]->bytes_received/(double)(1<<30) << " GB";

            channel_bytes[i] =
                chstats[i]->bytes_received - last_bytes_received[i];

            if( last_bytes_received[i] && channel_bytes[i] )
            {
                cout << " Data-Rate: " << fixed << setprecision(3) << setw(7) <<
                    (double)(channel_bytes[i])/librorc::gettimeofdayDiff(last_time, cur_time)/(double)(1<<20)
                    << " MB/s";

            }
            else
            {
                cout << " Data-Rate: -";
            }

            if
                (
                 last_events_received[i] &&
                 chstats[i]->n_events - last_events_received[i]
                )
                {
                    cout << " Event Rate: "  << fixed << setprecision(3) << setw(7) <<
                        (double)(chstats[i]->n_events-last_events_received[i])/
                        librorc::gettimeofdayDiff(last_time, cur_time)/1000.0 << " kHz";
                }
            else
            {
                cout << " Event-Rate: -";
            }

            cout << " Errors: " << chstats[i]->error_count << endl;

            last_bytes_received[i] = chstats[i]->bytes_received;
            last_events_received[i] = chstats[i]->n_events;
        }

        cout << "======== ";

        uint64_t sum_of_bytes      = 0;
        uint64_t sum_of_bytes_diff = 0;
        for(int32_t i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++)
        {
            sum_of_bytes      += chstats[i]->bytes_received;
            sum_of_bytes_diff += channel_bytes[i];
        }

        if(sum_of_bytes_diff)
        {
            cout << "Combined Data-Size: " << (double)sum_of_bytes/((uint64_t)1<<40)
                << " TB, Combined Data-Rate: "
                << (double)((sum_of_bytes_diff)/librorc::gettimeofdayDiff(last_time, cur_time)/(double)(1<<20))
                << " MB/s";

        }
        else
        {
            cout << " Combined Data-Rate: -";
        }

        cout << " ========" << endl;

        last_time = cur_time;

        sleep(STAT_INTERVAL);

        iter = (Iterations!=INT32_MAX) ? iter+1 : iter;
    }

    /** Detach all the shared memory */
    for(int32_t i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++)
    {
        if( shm[i] != NULL )
        {
            shmdt(shm[i]);
            shm[i] = NULL;
        }
    }

    return 0;
}
