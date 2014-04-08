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

namespace LIBRARY_NAME
{
class buffer;
class dma_channel;
class event_stream;

    class event_generator
    {
        public:

            event_generator(){ }

            event_generator(event_stream *eventStream );

            ~event_generator(){ }

            uint64_t fillEventBuffer(uint32_t event_size);

        protected:
            uint64_t      m_event_id;
            event_stream *m_event_stream;

            void
            packEventsIntoMemory
            (
                uint64_t number_of_events,
                uint32_t event_size
            );

            /**
             * Create an event
             * @param Event ID
             * @param event length
            **/
            void
            createEvent
            (
                uint64_t event_id,
                uint32_t event_size
            );

    };

}
