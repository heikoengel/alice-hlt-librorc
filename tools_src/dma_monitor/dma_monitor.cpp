/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2013-01-11i
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

#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/shm.h>
#include <getopt.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <librorc.h>
#include <event_handling.h>

#define STAT_INTERVAL 1

using namespace std;

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
    int32_t DeviceId = -1;

    // command line arguments
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
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
                break;;
            case 'h':
                cout << HELP_TEXT;
                exit(0);
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
    uint64_t           last_bytes_received[LIBRORC_MAX_DMA_CHANNELS];
    uint64_t           last_events_received[LIBRORC_MAX_DMA_CHANNELS];
    uint64_t           channel_bytes[LIBRORC_MAX_DMA_CHANNELS];
    struct   ch_stats *chstats[LIBRORC_MAX_DMA_CHANNELS];
    int32_t            shID[LIBRORC_MAX_DMA_CHANNELS];
    char              *shm[LIBRORC_MAX_DMA_CHANNELS];

    for(int32_t i=0; i<LIBRORC_MAX_DMA_CHANNELS; i++)
    {
        last_bytes_received[i] = 0;
        last_events_received[i] = 0;
        channel_bytes[i] = 0;

        shID[i] = shmget(SHM_KEY_OFFSET + DeviceId*SHM_DEV_OFFSET + i, 
                sizeof(struct ch_stats), IPC_CREAT | 0666);
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
        chstats[i] = (struct ch_stats*)shm[i];
    }

    /** capture starting time */
    timeval cur_time;
    gettimeofday(&cur_time, 0);
    timeval last_time = cur_time;

    while( !done )
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
                    (double)(channel_bytes[i])/gettimeofday_diff(last_time, cur_time)/(double)(1<<20)
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
                        gettimeofday_diff(last_time, cur_time)/1000.0 << " kHz";
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
                << (double)((sum_of_bytes_diff)/gettimeofday_diff(last_time, cur_time)/(double)(1<<20))
                << " MB/s";

        }
        else
        {
            cout << " Combined Data-Rate: -";
        }

        cout << " ========" << endl;

        last_time = cur_time;

        sleep(STAT_INTERVAL);
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
