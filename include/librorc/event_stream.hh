/**
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-08-16
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
 */

#ifndef LIBRORC_EVENT_STREAM_H
#define LIBRORC_EVENT_STREAM_H

#include <librorc/include_ext.hh>
#include "defines.hh"
#include <librorc/buffer.hh>

#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED     1
#define LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED   2


typedef enum
{
    LIBRORC_ES_PURE,
    LIBRORC_ES_DDL,
    LIBRORC_ES_PG
} LibrorcEsType;


/** Buffer Sizes (in Bytes) **/
#ifndef SIM
    #define EBUFSIZE (((uint64_t)1) << 28)
    #define RBUFSIZE (((uint64_t)1) << 26)
    #define STAT_INTERVAL 1.0
#else
    #define EBUFSIZE (((uint64_t)1) << 19)
    #define RBUFSIZE (((uint64_t)1) << 17)
    #define STAT_INTERVAL 0.00001
#endif

/** Shared mem key offset **/
#define SHM_KEY_OFFSET 2048
/** Shared mem device offset **/
#define SHM_DEV_OFFSET 32

typedef struct
{
    uint64_t n_events;
    uint64_t bytes_received;
    uint64_t min_epi;
    uint64_t max_epi;
    uint64_t index;
    uint64_t set_offset_count;
    uint64_t error_count;
    uint64_t last_id;
    uint32_t channel;
}librorcChannelStatus;

typedef uint64_t (*librorc_event_callback)
(
    void*,
    uint64_t,
    librorc_event_descriptor,
    const uint32_t*,
    librorcChannelStatus*
);



namespace LIBRARY_NAME
{

class dma_channel;
class bar;
class buffer;
class device;
class event_sanity_checker;

    class event_stream
    {
        public:

             event_stream
             (
                int32_t deviceId,
                int32_t channelId
             );

             event_stream
             (
                int32_t       deviceId,
                int32_t       channelId,
                uint32_t      eventSize,
                LibrorcEsType esType
             );

            ~event_stream();
            void printDeviceStatus();

            uint64_t eventLoop(void *user_data);

            void
            setEventCallback(librorc_event_callback event_callback)
            {
                m_event_callback = event_callback;
            }

            /** Member Variables */
            device      *m_dev;
            bar         *m_bar1;
            buffer      *m_eventBuffer;
            buffer      *m_reportBuffer;
            dma_channel *m_channel;

            bool         m_done;
            uint64_t     m_last_bytes_received;
            uint64_t     m_last_events_received;

            timeval      m_start_time;
            timeval      m_end_time;
            timeval      m_last_time;
            timeval      m_current_time;

            librorcChannelStatus *m_channel_status;



        protected:
            uint32_t  m_eventSize;
            int32_t   m_deviceId;
            int32_t   m_channelId;

            volatile uint32_t *m_raw_event_buffer;

            librorc_event_callback m_event_callback = NULL;

            void
            generateDMAChannel
            (
                int32_t       deviceId,
                int32_t       channelId,
                LibrorcEsType esType
            );

            void     chooseDMAChannel(LibrorcEsType esType);
            void     prepareSharedMemory();
            void     deleteParts();
            uint64_t dwordOffset(librorc_event_descriptor report_entry);
            uint64_t getEventIdFromCdh(uint64_t offset);

            uint64_t handleChannelData(void *user_data);

            const
            uint32_t* getRawEvent(librorc_event_descriptor report);

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
