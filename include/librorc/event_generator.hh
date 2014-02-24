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

#include "librorc/include_ext.hh"
#include "defines.hh"

#define MAX_EVENTS_PER_ITERATION 0x0

namespace LIBRARY_NAME
{
class buffer;
class dma_channel;

    class event_generator
    {
        public:

            event_generator()
            { }

            event_generator
            (
                librorc::buffer      *report_buffer,
                librorc::buffer      *event_buffer,
                librorc::dma_channel *channel
            );

            ~event_generator()
            { }

            uint64_t fillEventBuffer(uint32_t event_size);

        protected:

            uint64_t     m_last_event_buffer_offset;
            uint64_t     m_event_generation_offset;
            uint64_t     m_event_id;
            buffer      *m_report_buffer;
            buffer      *m_event_buffer;
            dma_channel *m_channel;


            /**
            * Create event
            * @param dest uint32_t* pointer to destination memory
            * @param event_id Event ID
            * @param length event length
            **/
            void
            createEvent
            (
                volatile uint32_t *dest,
                uint64_t event_id,
                uint32_t length,
                uint32_t fragment_size
            );

            /**
             * Get the available event buffer space in bytes between the current
             * generation offset and the last offset written to the channel
             **/
            uint64_t
            availableBufferSpace(uint64_t event_generation_offset);

            /**
             * check how many events can be put into the available space
             * note: EventSize is in DWs and events have to be aligned to
             * MaxReadReq boundaries fragment_size is in bytes
             **/
            uint32_t
            fragmentSize
            (
                 uint32_t event_size,
                 uint32_t max_read_req
            );

            uint64_t
            numberOfEvents
            (
                uint64_t available_buffer_space,
                uint32_t event_size,
                uint32_t fragment_size
            );

            bool isSufficientFifoSpaceAvailable();

            /**
             * reduce the number of events to the maximum the
             * EL_FIFO can handle a.t.m.
             */
            uint64_t
            maximumElfifoCanHandle(uint64_t number_of_events);

            uint64_t
            reduceNumberOfEventsToCustomMaximum(uint64_t number_of_events);

            uint64_t
            numberOfEventsThatFitIntoBuffer
            (
                uint64_t available_buffer_space,
                uint32_t event_size,
                uint32_t fragment_size
            );

            void
            packEventsIntoMemory
            (
                uint64_t number_of_events,
                uint32_t event_size,
                uint32_t fragment_size
            );

            void pushEventSizeIntoELFifo(uint32_t event_size);
            void iterateEventBufferFillState(uint32_t fragment_size);
            void wrapFillStateIfNecessary();
    };

}
