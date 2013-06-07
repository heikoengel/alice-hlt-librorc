/**
 * @file pgdma_continuous.cpp
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.1
 * @date 2013-06-07
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
#include <sys/mman.h>
#include <fcntl.h>

#include "librorc.h"
#include "event_handling.h"

using namespace std;

/** Buffer Sizes (in Bytes) **/
#ifndef SIM
#define EBUFSIZE (((unsigned long)1) << 28)
#define RBUFSIZE (((unsigned long)1) << 26)
#define STAT_INTERVAL 1.0
#else
#define EBUFSIZE (((unsigned long)1) << 19)
#define RBUFSIZE (((unsigned long)1) << 17)
#define STAT_INTERVAL 0.00001
#endif


/** maximum channel number allowed **/
#define MAX_CHANNEL 11

int16_t
alloc_channel
(
    uint32_t     ChannelId,
    librorc_bar *Bar;
)
{
    /** check if requested channel is implemented in firmware */
    if( ChannelId >= (Bar->get(RORC_REG_TYPE_CHANNELS) & 0xffff) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
            "firmware - exiting\n", ChannelId);
        return -1;
    }

    /** create a new DMA event buffer */
    rorcfs_buffer *ebuf = new rorcfs_buffer();
    if ( ebuf->allocate(dev, EBUFSIZE, 2*ChannelId, 1, RORCFS_DMA_FROM_DEVICE)!=0 )
    {
        if ( errno == EEXIST )
        {
            if ( ebuf->connect(dev, 2*ChannelId) != 0 )
            {
                perror("ERROR: ebuf->connect");
                goto out;
            }
        }
        else
        {
            perror("ERROR: ebuf->allocate");
            goto out;
        }
    }

    /** */ create new DMA report buffer
    rorcfs_buffer *rbuf = new rorcfs_buffer();
    if ( rbuf->allocate(dev, RBUFSIZE, 2*ChannelId+1, 1, RORCFS_DMA_FROM_DEVICE)!=0 )
    {
        if ( errno == EEXIST )
        {
            if ( rbuf->connect(dev, 2*ChannelId+1) != 0 )
            {
                perror("ERROR: rbuf->connect");
                goto out;
            }
        }
        else
        {
            perror("ERROR: rbuf->allocate");
            goto out;
        }
    }
}


int main( int argc, char *argv[])
{
    /** Iterate all Devices*/
    for( uint16_t i=0; i<UINT16_MAX; i++)
    {
        /** create new device instance */
        dev = new rorcfs_device();
        if( dev->init(i) == -1 )
        { break; }

        /** bind to BAR1 */
        librorc_bar *bar1;
        #ifdef SIM
            bar1 = new sim_bar(dev, 1);
        #else
            bar1 = new rorc_bar(dev, 1);
        #endif
        if( bar1->init() == -1 )
        {
            printf("ERROR: failed to initialize BAR1.\n");
            goto out;
        }

        for( uint32_t ChannelId = 0; ChannelId<=MAX_CHANNEL; ChannelId++ )
        {
            int16_t alloc_channel(uint32_t ChannelId, Bar)
        }

    }

    return 0;
}
