/**
 * @file
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
 * Allocate memory for all crorcs in the system. This is especially
 * needed to allocate during the early boot phase to get a low amount
 * of SG-List entries.
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

#include <librorc.h>

#define UINT16_MAX 65535

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
    uint32_t         ChannelID,
    librorc::bar    *Bar,
    librorc::device *Dev
);



int main( int argc, char *argv[])
{
    /** Iterate all Devices*/
    for( uint16_t i=0; i<UINT16_MAX; i++)
    {
        /** create new device instance */
        librorc::device *Dev;
        try{ Dev = new librorc::device(i); }
        catch(...){ break; }

        /** bind to BAR1 */
        #ifdef SIM
            librorc::bar *Bar = new librorc::sim_bar(Dev, 1);
        #else
            librorc::bar *Bar = new librorc::rorc_bar(Dev, 1);
        #endif
        if( Bar->init() == -1 )
        {
            printf("ERROR: failed to initialize BAR1.\n");
            abort();
        }

        for( uint32_t ChannelId = 0; ChannelId<=MAX_CHANNEL; ChannelId++ )
            { alloc_channel(ChannelId, Bar, Dev); }

    }

    return 0;
}



int16_t
alloc_channel
(
    uint32_t         ChannelID,
    librorc::bar    *Bar,
    librorc::device *Dev
)
{
    /** check if requested channel is implemented in firmware */
    if( ChannelID >= (Bar->get(RORC_REG_TYPE_CHANNELS) & 0xffff) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
            "firmware - exiting\n", ChannelID);
        return -1;
    }

    /** create a new DMA event buffer */
    try
    {
        librorc::buffer *ebuf
            = new librorc::buffer(Dev, EBUFSIZE, 2*ChannelID, 1, LIBRORC_DMA_FROM_DEVICE);
    }
    catch(...)
    {
        perror("ERROR: ebuf->allocate");
        abort();
    }

    /** create new DMA report buffer */
    try
    {
        librorc::buffer *rbuf
            = new librorc::buffer(Dev, RBUFSIZE, 2*ChannelID+1, 1, LIBRORC_DMA_FROM_DEVICE);
    }
    catch(...)
    {
        perror("ERROR: rbuf->allocate");
        abort();
    }

    return 0;
}