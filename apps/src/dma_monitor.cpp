/**
 * @file dma_monitor.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
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

#include "librorc.h"
#include "event_handling.h"

#define STAT_INTERVAL 1.0
#define N_CHANNELS 12

int done = 0;

void abort_handler( int s )
{
  printf("Caught signal %d\n", s);
  done = 1;
}


int main( int argc, char *argv[] )
{
  int shID[N_CHANNELS];
  char *shm[N_CHANNELS];
  struct ch_stats *chstats[N_CHANNELS];
  int i;

  timeval start_time;
  timeval last_time, cur_time;
  unsigned long last_bytes_received[N_CHANNELS];
  unsigned long last_events_received[N_CHANNELS];
  unsigned long channel_bytes[N_CHANNELS];
  unsigned long sum_of_bytes, sum_of_bytes_diff;

  // catch CTRL+C for abort
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = abort_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  for(i=0;i<N_CHANNELS;i++)
  {
    shm[i] = NULL;
    chstats[i] = NULL;
    last_bytes_received[i] = 0;
    last_events_received[i] = 0;
    channel_bytes[i] = 0;
  }

  for(i=0;i<N_CHANNELS;i++)
  {
    shID[i] = shmget(SHM_KEY_OFFSET + i, sizeof(struct ch_stats), 
        IPC_CREAT | 0666);
    if ( shID[i]==-1) {
      perror("shmget");
      goto out;
    }

    shm[i] = (char *)shmat(shID[i], 0, 0);
    if(shm[i]==(char*)-1) {
      perror("shmat");
      goto out;
    }
    chstats[i] = (struct ch_stats*)shm[i];
  }

  // capture starting time
  gettimeofday(&start_time, 0);
  last_time = start_time;
  cur_time = start_time;

  sigaction(SIGINT, &sigIntHandler, NULL);

  while( !done ) {
    gettimeofday(&cur_time, 0);

    // print status line each second
    if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL) {

      for(i=0;i<N_CHANNELS;i++)
      {

        printf("CH%2d - Events: %10ld, DataSize: %8.3f GB ",
            i, chstats[i]->n_events, 
            (double)chstats[i]->bytes_received/(double)(1<<30));

        channel_bytes[i] = chstats[i]->bytes_received -
          last_bytes_received[i];

        if ( last_bytes_received[i] && channel_bytes[i])
        {
          printf(" DataRate: %9.3f MB/s",
              (double)(channel_bytes[i])/
              gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
        } else {
          printf(" DataRate: -");
        }

        if ( last_events_received[i] && 
            chstats[i]->n_events - last_events_received[i])
        {
          printf(" EventRate: %9.3f kHz",
              (double)(chstats[i]->n_events-last_events_received[i])/
              gettimeofday_diff(last_time, cur_time)/1000.0);
        } else {
          printf(" EventRate: -");
        }
        printf(" Errors: %ld\n", chstats[i]->error_count);
        last_bytes_received[i] = chstats[i]->bytes_received;
        last_events_received[i] = chstats[i]->n_events;
      }
      printf("======== ");
      sum_of_bytes=0;
      sum_of_bytes_diff=0;
      for(i=0;i<N_CHANNELS;i++) {
        sum_of_bytes+=chstats[i]->bytes_received;
        sum_of_bytes_diff+=channel_bytes[i];
      }
      if(sum_of_bytes_diff)
        printf("DataSize: %8.3f TB, DataRate: %9.3f MB/s",
            (double)sum_of_bytes/((uint64_t)1<<40),
            (double)(sum_of_bytes_diff)/
            gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
      else
        printf(" DataRate: -");
      printf(" ========\n");


      last_time = cur_time;
    }
    usleep(100);
  }


out:
  for(i=0;i<N_CHANNELS;i++)
  {
    if (shm[i]) {
      shmdt(shm[i]);
      shm[i] = NULL;
    }
  }

  return 0;
}
