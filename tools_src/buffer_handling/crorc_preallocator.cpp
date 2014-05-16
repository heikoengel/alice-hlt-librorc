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

#include <librorc.h>

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

using namespace std;

/** maximum channel number allowed **/
#define MAX_CHANNEL 11

void
alloc_channel
(
    uint32_t         ChannelID,
    librorc::device *Dev,
    uint64_t         size
);

int main( int argc, char *argv[])
{

    for(uint16_t DeviceId=0; DeviceId<UINT16_MAX; DeviceId++)
    {
        librorc::device *Dev;
        try{ Dev = new librorc::device(DeviceId); }
        catch(...){ return(0); }

        for( uint32_t ChannelId = 0; ChannelId<=MAX_CHANNEL; ChannelId++ )
        { alloc_channel(ChannelId, Dev, EBUFSIZE); }

    }

    return(0);
}



void
alloc_channel
(
    uint32_t         ChannelID,
    librorc::device *Dev,
    uint64_t         size
)
{
    if( !(Dev->DMAChannelIsImplemented(ChannelID)) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
            "firmware - exiting\n", ChannelID);
        return;
    }

    librorc::event_stream *eventStream = NULL;
    try
    {
        eventStream =
            new librorc::event_stream
                (Dev->getDeviceId(), ChannelID, LIBRORC_ES_TO_HOST, size);
    }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream. "
             << "Buffer allocation failed" << endl;
        return;
    }

    delete eventStream;

//    /** create a new DMA event buffer */
//    librorc::buffer *ebuf;
//    try
//    { ebuf = new librorc::buffer(Dev, EBUFSIZE, 2*ChannelID, 1, LIBRORC_DMA_FROM_DEVICE); }
//    catch(...)
//    {
//        perror("ERROR: ebuf->allocate");
//        abort();
//    }
//
//    /** create new DMA report buffer */
//    librorc::buffer *rbuf;
//    try
//    { rbuf = new librorc::buffer(Dev, RBUFSIZE, 2*ChannelID+1, 1, LIBRORC_DMA_FROM_DEVICE); }
//    catch(...)
//    {
//        perror("ERROR: rbuf->allocate");
//        abort();
//    }
//
//    delete ebuf;
//    delete rbuf;

}
