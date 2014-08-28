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
