/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
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

#include <librorc/event_generator.hh>

#include <librorc/buffer.hh>
#include <librorc/dma_channel.hh>
#include <librorc/event_stream.hh>
#include <librorc/registers.h>

#include <pda.h>

using namespace std;

namespace LIBRARY_NAME
{
    event_generator::event_generator(event_stream *eventStream)
    {
        m_event_stream = eventStream;
        m_event_id     = 0;
    }

    uint64_t
    event_generator::fillEventBuffer(uint32_t event_size)
    {
        uint64_t number_of_events
            = m_event_stream->numberOfEvents( event_size );

        packEventsIntoMemory(number_of_events, event_size);

        return number_of_events;
    }

    void
    event_generator::packEventsIntoMemory
    (
        uint64_t number_of_events,
        uint32_t event_size
    )
    {
        for(uint64_t i = 0; i < number_of_events; i++)
        {
            createEvent(m_event_id, event_size);
            m_event_id += 1;
        }
    }

    void
    event_generator::createEvent
    (
        uint64_t event_id,
        uint32_t event_size
    )
    {
        uint32_t i;

        if(event_size <= 8)
        { throw 0; }

        uint32_t tmp_buffer[event_size];

        /** First 8 DWs are CDH */
        tmp_buffer[0] = 0xffffffff;
        tmp_buffer[1] = event_id & 0xfff;
        tmp_buffer[2] = ((event_id>>12) & 0x00ffffff);
        tmp_buffer[3] = 0x00000000; // PGMode / participating subdetectors
        tmp_buffer[4] = 0x00000000; // mini event id, error flags, MBZ
        tmp_buffer[5] = 0xaffeaffe; // trigger classes low
        tmp_buffer[6] = 0x00000000; // trigger classes high, MBZ, ROI
        tmp_buffer[7] = 0xdeadbeaf; // ROI high

        for(i=0; i<event_size-8; i++)
        { tmp_buffer[8+i] = i; }

        m_event_stream->packEventIntoBuffer(tmp_buffer, event_size);
    }

}
