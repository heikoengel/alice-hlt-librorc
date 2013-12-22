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

#include <math.h>
#define MAX_EVENTS_PER_ITERATION 0x0


class event_generator
{
	public:

         event_generator()
         {

         };

		 event_generator
		 (
		     librorc::buffer      *report_buffer,
			 librorc::buffer      *event_buffer,
			 librorc::dma_channel *channel
		 )
		 {
			 m_report_buffer           = report_buffer;
			 m_event_buffer            = event_buffer;
			 m_channel                 = channel;
			 m_event_generation_offset = 0;
			 m_event_id                = 0;
		 };

		~event_generator()
		 {

		 };

		uint64_t
		fillEventBuffer(uint32_t event_size)
		{
			m_last_event_buffer_offset
			    = m_channel->getLastEBOffset();

			uint32_t max_read_req
				= m_channel->pciePacketSize();

			uint64_t available_buffer_space
			    = availableBufferSpace(m_event_generation_offset);

			uint32_t fragment_size
				= fragmentSize(event_size, max_read_req);

			uint64_t number_of_events
				= numberOfEvents(available_buffer_space, event_size, fragment_size);

			packEventsIntoMemory(number_of_events, event_size, fragment_size);

			return number_of_events;
		};

	protected:

		uint64_t              m_last_event_buffer_offset;
		uint64_t              m_event_generation_offset;
		uint64_t              m_event_id;
		librorc::buffer      *m_report_buffer;
		librorc::buffer      *m_event_buffer;
		librorc::dma_channel *m_channel;


		/**
		 * Create event
		 * @param dest uint32_t* pointer to destination memory
		 * @param event_id Event ID
		 * @param length event length
		 **/
		//TODO: since this needs to be extended it should better refactored into a class!
		void
		createEvent
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
		};


        /**
         * Get the available event buffer space in bytes between the current
         * generation offset and the last offset written to the channel
         **/
		uint64_t
		availableBufferSpace(uint64_t event_generation_offset)
		{
			return   (event_generation_offset < m_last_event_buffer_offset)
				   ? m_last_event_buffer_offset - event_generation_offset
				   : m_last_event_buffer_offset + m_event_buffer->getSize()
					  - event_generation_offset; /** wrap in between */
		};

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
	    )
		{
			return   ((event_size << 2) % max_read_req)
				   ? (trunc((event_size << 2) / max_read_req) + 1) * max_read_req
				   : (event_size << 2);
		}



		uint64_t
		numberOfEvents
		(
		    uint64_t available_buffer_space,
			uint32_t event_size,
			uint32_t fragment_size)
		{

			if(!isSufficientFifoSpaceAvailable())
			{ return 0; }

			uint64_t
			number_of_events
				= numberOfEventsThatFitIntoBuffer
					(available_buffer_space, event_size, fragment_size);

			number_of_events
				= maximumElfifoCanHandle(number_of_events);

			number_of_events
				= reduceNumberOfEventsToCustomMaximum(number_of_events);

			return number_of_events;
		}

		bool
		isSufficientFifoSpaceAvailable()
		{
			uint32_t el_fifo_state       = m_channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO);
			uint32_t el_fifo_write_limit = ((el_fifo_state >> 16) & 0x0000ffff);
			uint32_t el_fifo_write_count = (el_fifo_state & 0x0000ffff);
			return !(el_fifo_write_count + 10 >= el_fifo_write_limit);
		}

		/**
		 * reduce the number of events to the maximum the
		 * EL_FIFO can handle a.t.m.
		 */
		uint64_t
		maximumElfifoCanHandle(uint64_t number_of_events)
		{
		    uint32_t el_fifo_state       = m_channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO);
		    uint32_t el_fifo_write_limit = ((el_fifo_state >> 16) & 0x0000ffff);
		    uint32_t el_fifo_write_count = (el_fifo_state & 0x0000ffff);

		    return
		    (el_fifo_write_limit - el_fifo_write_count < number_of_events)
		    ? (el_fifo_write_limit - el_fifo_write_count) : number_of_events;
		}

		uint64_t
		reduceNumberOfEventsToCustomMaximum(uint64_t number_of_events)
		{
		    return
		    (MAX_EVENTS_PER_ITERATION && number_of_events > MAX_EVENTS_PER_ITERATION)
			? MAX_EVENTS_PER_ITERATION : number_of_events;
		}

		uint64_t
		numberOfEventsThatFitIntoBuffer
		(
		    uint64_t available_buffer_space,
			uint32_t event_size,
			uint32_t fragment_size
		)
		{
			return
			((available_buffer_space - event_size) <= fragment_size)
			? 0 : ((uint64_t)(available_buffer_space / fragment_size) - 1);
		}



		void
		packEventsIntoMemory
		(
		    uint64_t number_of_events,
		    uint32_t event_size,
			uint32_t fragment_size
		)
		{
			volatile uint32_t* eventbuffer = m_event_buffer->getMem();
			for(uint64_t i = 0; i < number_of_events; i++)
			{
				createEvent((eventbuffer + (m_event_generation_offset >> 2)), m_event_id, event_size);

				DEBUG_PRINTF
				(
				    PDADEBUG_CONTROL_FLOW,
				    "create_event(%lx, %lx, %x)\n",
					m_event_generation_offset,
					m_event_id,
					event_size
				);

				pushEventSizeIntoELFifo(event_size);
				iterateEventBufferFillState(fragment_size);
				wrapFillStateIfNecessary();
			}
		}

		void
		pushEventSizeIntoELFifo(uint32_t event_size)
		{
			m_channel->getLink()->setPacketizer(RORC_REG_DMA_ELFIFO, event_size);
		}

		void
		iterateEventBufferFillState(uint32_t fragment_size)
		{
			m_event_generation_offset += fragment_size;
			m_event_id += 1;
		}

		void
		wrapFillStateIfNecessary()
		{
			m_event_generation_offset
				= (m_event_generation_offset >= m_event_buffer->getSize())
				? (m_event_generation_offset - m_event_buffer->getSize())
				: m_event_generation_offset;
		}
};
