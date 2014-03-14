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
    uint64_t shadow_index;
    uint64_t set_offset_count;
    uint64_t error_count;
    uint64_t last_id;
    uint32_t channel;
    uint32_t device;
}librorcChannelStatus;

typedef uint64_t (*librorc_event_callback)
(
    void*,
    uint64_t,
    librorc_event_descriptor,
    const uint32_t*,
    librorcChannelStatus*
);

typedef uint64_t (*librorc_status_callback)
(
    timeval,
    timeval,
    librorcChannelStatus*,
    uint64_t,
    uint64_t
);


namespace LIBRARY_NAME
{

class dma_channel;
class bar;
class buffer;
class device;
class event_sanity_checker;

    /**
     * @class librorc::event_stream
     * @brief This class glues everything together to receive or send events
     *        with a CRORC. It manages report as well as event buffer and
     *        configures the DMA channels etc. It also features an API to handle
     *        incoming and send outgoing events.
     *
     * This class manages the DMA buffers (report- and event buffer).
     **/
    class event_stream
    {
        public:

             event_stream
             (
                int32_t       deviceId,
                int32_t       channelId,
                LibrorcEsType esType
             );

             event_stream
             (
                int32_t       deviceId,
                int32_t       channelId,
                uint32_t      eventSize,
                LibrorcEsType esType
             );

#ifdef LIBRORC_INTERNAL
             event_stream
             (
                librorc::device *dev,
                librorc::bar    *bar,
                int32_t          channelId,
                LibrorcEsType    esType
             );

             event_stream
             (
                librorc::device *dev,
                librorc::bar    *bar,
                int32_t          channelId,
                uint32_t         eventSize,
                LibrorcEsType    esType
             );
#endif

            ~event_stream();

            /**
             * Check the firmware type (in, out, etc.)
             * @param [in] esType
             *        Event stream type (look in defines.h for possible options).
             */
            void checkFirmware(LibrorcEsType esType);

            /**
             * Print the current device status.
             */
            void printDeviceStatus();

            /**
             * High level interface to read out an event stream. Calls an event_callback
             * for each new event in the buffer.  setEventCallback must be called before
             * this one.
             * @param [in] user_data
             *        Free form pointer to some userdata which needs to be used inside the
             *        callback.
             */
            uint64_t eventLoop(void *user_data);

            /**
             * Set event callback which is called by eventLoop(void *user_data). This callback
             * is basically called every time when a new event arrives in the event buffer.
             * @param [in] event_callback
             *        Callback function pointer (see event_stream.hh for the function pointer
             *        layout).
             */
            void
            setEventCallback(librorc_event_callback event_callback)
            { m_event_callback = event_callback; }

            /**
             * Set the status callback. This callback is basically used to gather statistics
             * to the user of the library. The callback does not need to be set (it is optional)
             * @param [in] status_callback
             *        Callback function pointer (see event_stream.hh for the function pointer
             *        layout).
             */
            void
            setStatusCallback(librorc_status_callback status_callback)
            { m_status_callback = status_callback; }

            /**
             * Get the pointers to the next event in the event buffer. This routine cann be
             * used if the eventLoop API is to high level. Any event, which was obtained by
             * this function, needs to be cleared by the releaseEvent() method to release
             * memory resources. Beware, not clearing an event can lead to a deadlock.
             * @param [out] report
             *        Pointer to the event descriptor field inside the report buffer. Please
             *        see buffer.hh for the detailed memory layout.
             * @param [out] event_id
             *        Event ID.
             * @param [out] event
             *        Pointer to the event payload.
             *
             * @return true if there was a new event and false if the buffer was empty
             */
            bool
            getNextEvent
            (
                librorc_event_descriptor **report,
                uint64_t                  *event_id,
                const uint32_t           **event
            );

            /**
             * Release the event which was obtained by getNextEvent().
             * @param [in] report Pointer to the report entry which was obtained with
             *        getNextEvent().
             */
            void releaseEvent(librorc_event_descriptor *report);

            /**
             * @internal
             */
            uint64_t handleChannelData(void *user_data);

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
            uint32_t m_eventSize;
            int32_t  m_deviceId;
            int32_t  m_channelId;
            bool     m_called_with_bar;
            bool     m_release_map[RBUFSIZE/sizeof(librorc_event_descriptor)];

            volatile uint32_t        *m_raw_event_buffer;
            librorc_event_descriptor *m_reports;

            librorc_event_callback  m_event_callback;
            librorc_status_callback m_status_callback;

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
            void     setBufferOffsets();


            const
            uint32_t* getRawEvent(librorc_event_descriptor report);

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
