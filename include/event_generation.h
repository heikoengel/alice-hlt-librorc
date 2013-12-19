/**
 * @file event_generation.h
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-02-04
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

#define MAX_EVENTS_PER_ITERATION 0x0
#include <math.h>

/**
 * create event
 * @param dest uint32_t* pointer to destination memory
 * @param event_id Event ID
 * @param length event length
 * */
void 
create_event
(
    volatile uint32_t *dest,
    uint64_t event_id,
    uint32_t length
)
{
    uint32_t i;

    if(length <= 8)
    { throw 0; }

    /** First 8 DWs are CDH */
    dest[0] = 0xffffffff;
    dest[1] = event_id & 0xfff;
    dest[2] = ((event_id>>12) & 0x00ffffff);
    dest[3] = 0x00000000; // PGMode / participating subdetectors
    dest[4] = 0x00000000; // mini event id, error flags, MBZ
    dest[5] = 0xaffeaffe; // trigger classes low
    dest[6] = 0x00000000; // trigger classes high, MBZ, ROI
    dest[7] = 0xdeadbeaf; // ROI high

    for(i=0; i<length-8; i++)
    { dest[8+i] = i; }
}


uint64_t
fill_eventbuffer
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    uint64_t             *event_generation_offset,
    uint64_t             *EventID,
    uint32_t             EventSize
)
{
    // get last EB Offset written to channel
    uint64_t last_eb_offset = channel->getLastEBOffset();
    uint64_t buf_space_avail;
    uint32_t fragment_size;

    // get the available event buffer space in bytes between the current
    // generation offset and the last offset written to the channel
    if ( *event_generation_offset < last_eb_offset )
    {
        // generation offset is smaller than last offset -> no wrap
        buf_space_avail = last_eb_offset - *event_generation_offset;
    } else 
    {
        // generation offset is larger than last offset -> wrap in between
        buf_space_avail = last_eb_offset + ebuf->getSize() - 
            *event_generation_offset;
    }

    // check how many events can be put into the available space
    // note: EventSize is in DWs and events have to be aligned to
    // MaxReadReq boundaries
    // fragment_size is in bytes
    uint32_t max_read_req = channel->pciePacketSize();
    if ( (EventSize<<2) % max_read_req )
    {
        // EventSize is not a multiple of max_read_req
        fragment_size = (trunc((EventSize<<2) / max_read_req) + 1) * 
            max_read_req;
    }
    else
    { fragment_size = (EventSize<<2); }

    uint64_t nevents = (uint64_t)(buf_space_avail / fragment_size);

    // never use full buf_space_avail to avoid the situation where
    // event_generation_offset==last_eb_offset because this will break
    // buf_space_avail calculation above
    if ( (buf_space_avail - EventSize) <= fragment_size )
    { nevents = 0; }
    else
    { nevents = (uint64_t)(buf_space_avail / fragment_size) - 1;}

    // get current EL FIFO fill state consisting of:
    // el_fifo_state[31:16] = FIFO write limit
    // el_fifo_state[15:0]  = FIFO write count
    uint32_t el_fifo_state   = channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO);
    uint32_t el_fifo_wrlimit = ((el_fifo_state>>16) & 0x0000ffff);
    uint32_t el_fifo_wrcount = (el_fifo_state & 0x0000ffff);

    // break if no sufficient FIFO space available
    // margin of 10 is chosen arbitrarily here
    if ( el_fifo_wrcount + 10 >= el_fifo_wrlimit )
    { return 0; }

    // reduce nevents to the maximum the EL_FIFO can handle a.t.m.
    if ( el_fifo_wrlimit - el_fifo_wrcount < nevents )
    { nevents = el_fifo_wrlimit - el_fifo_wrcount; }

    // reduce nevents to a custom maximum
    if( MAX_EVENTS_PER_ITERATION && nevents > MAX_EVENTS_PER_ITERATION )
    { nevents = MAX_EVENTS_PER_ITERATION; }
    
    volatile uint32_t *eventbuffer = ebuf->getMem();

    for ( uint64_t i=0; i < nevents; i++ )
    {
        // byte offset of next event
        uint64_t offset = *event_generation_offset;

        volatile uint32_t *destination = eventbuffer + (offset>>2);
        create_event(destination, *EventID, EventSize );

        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "create_event(%lx, %lx, %x)\n",
                offset, *EventID, EventSize);

        // push event size into EL FIFO
        channel->getLink()->setPacketizer(RORC_REG_DMA_ELFIFO, EventSize);

        // adjust event buffer fill state
        *event_generation_offset += fragment_size;
        *EventID += 1;

        // wrap fill state if neccessary
        if( *event_generation_offset >= ebuf->getSize() )
        { *event_generation_offset -= ebuf->getSize(); }

    }

    return nevents;
}
