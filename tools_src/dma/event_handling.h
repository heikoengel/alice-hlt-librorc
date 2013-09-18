#ifndef EVENT_HANDLING_H
#define EVENT_HANDLING_H

#include <pda.h>
#include <librorc.h>
#include <fcntl.h>




/**
 * ==========================================================
 * print summary statistics
 * ==========================================================
 * @param n number of channels
 * @param chstats pointer to struct ch_stats
 * @param ch_last_bytes_received pointer to array of bytescounts
 * fromt the last iteration
 * @param timediff time passed since the last iteration
 * */
void
print_summary_stats
(
    uint32_t       n,
    librorcChannelStatus *chstats[],
    uint64_t      *ch_last_bytes_received,
    double         timediff
)
{
  librorcChannelStatus statsum;
  uint64_t last_bytes_received = 0;
  uint32_t i;
  //sum up all channels
  memset(&statsum, 0, sizeof(statsum));
  for(i=0;i<n;i++) {
    statsum.bytes_received += chstats[i]->bytes_received;
    statsum.error_count += chstats[i]->error_count;
    statsum.n_events += chstats[i]->n_events;
    statsum.set_offset_count += chstats[i]->set_offset_count;
    last_bytes_received += ch_last_bytes_received[i];
  }
  // summary stats
  printf("SUM:  Events: %10ld, DataSize: %8.3f GB ",
      statsum.n_events,
      (double)statsum.bytes_received/(double)(1<<30));

  if ( statsum.bytes_received-last_bytes_received)
  {
    printf(" DataRate: %9.3f MB/s",
        (double)(statsum.bytes_received-last_bytes_received)/
        timediff/(double)(1<<20));
  } else {
    printf(" DataRate: -");
  }
  printf(" Errors: %ld\n", statsum.error_count);
}



//TODO: this looks like redundant with the dma monitor
/**
 * ==========================================================
 * print channel statistics
 * ==========================================================
 * @param n number of channels
 * @param chstats pointer to struct ch_stats
 * @param ch_last_bytes_received pointer to array of bytescounts
 * fromt the last iteration
 * @param timediff time passed since the last iteration
 * */
void
print_channel_stats
(
    uint32_t n,
    librorcChannelStatus *chstats[],
    uint64_t *ch_last_bytes_received,
    double timediff
)
{
  librorcChannelStatus statsum;
  uint64_t last_bytes_received = 0;
  uint32_t i;
  //sum up all channels
  memset(&statsum, 0, sizeof(statsum));
  for(i=0;i<n;i++) {
    statsum.bytes_received += chstats[i]->bytes_received;
    statsum.error_count += chstats[i]->error_count;
    statsum.n_events += chstats[i]->n_events;
    statsum.set_offset_count += chstats[i]->set_offset_count;
    last_bytes_received += ch_last_bytes_received[i];
  }

  // summary stats
  printf("\n********Overall********\n");
  print_summary_stats(n, chstats, ch_last_bytes_received, timediff);
  printf("-------------------------\n");

  // per channel stats
  for(i=0;i<n;i++) {
    printf("CH%2d: Events: %10ld, DataSize: %8.3f GB ",
        i, chstats[i]->n_events,
        (double)chstats[i]->bytes_received/(double)(1<<30));

    if ( chstats[i]->bytes_received-ch_last_bytes_received[i])
    {
      printf(" DataRate: %9.3f MB/s",
          (double)(chstats[i]->bytes_received-
            ch_last_bytes_received[i])/
          timediff/(double)(1<<20));
    } else {
      printf(" DataRate: -");
    }
    printf(" Errors: %ld\n", chstats[i]->error_count);
    ch_last_bytes_received[i] = chstats[i]->bytes_received;
  }
}






#endif
