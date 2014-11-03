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
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef LIBRORC_HIGH_LEVEL_EVENT_STREAM_HH
#define LIBRORC_HIGH_LEVEL_EVENT_STREAM_HH

#include <librorc/event_stream.hh>

namespace LIBRARY_NAME
{
#ifndef MODELSIM
    #define STAT_INTERVAL 1.0
#else
    #define STAT_INTERVAL 0.00001
#endif

    typedef uint64_t (*event_callback)
    (
         void*,
         EventDescriptor,
         const uint32_t*,
         ChannelStatus*
    );

    typedef uint64_t (*status_callback)
    (
         timeval,
         timeval,
         ChannelStatus*,
         uint64_t,
         uint64_t
    );

    class device;
    class bar;
    class event_stream;

    class high_level_event_stream : public event_stream
    {
        public:
            high_level_event_stream
            (
                 int32_t       deviceId,
                 int32_t       channelId,
                 LibrorcEsType esType
            );

#ifdef LIBRORC_INTERNAL
             high_level_event_stream
             (
                device          *dev,
                bar             *bar,
                int32_t          channelId,
                LibrorcEsType    esType
             );
#endif
            ~high_level_event_stream();

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
            setEventCallback(event_callback event_callback)
            { m_event_callback = event_callback; }

            /**
             * Set the status callback. This callback is basically used to gather statistics
             * to the user of the library. The callback does not need to be set (it is optional)
             * @param [in] status_callback
             *        Callback function pointer (see event_stream.hh for the function pointer
             *        layout).
             */
            void
            setStatusCallback(status_callback status_callback)
            { m_status_callback = status_callback; }

            /**
             * @internal
             */
            uint64_t handleChannelData(void *user_data);

            /** Member Variables */
            bool         m_done;
            uint64_t     m_last_bytes_received;
            uint64_t     m_last_events_received;

            timeval      m_start_time;
            timeval      m_end_time;
            timeval      m_last_time;
            timeval      m_current_time;

        protected:
            event_callback m_event_callback;
            status_callback m_status_callback;

            uint64_t dwordOffset(EventDescriptor report_entry);
            uint64_t getEventIdFromCdh(uint64_t offset);

            uint64_t
            handleEvent
            (
                uint64_t         events_processed,
                void            *user_data,
                EventDescriptor *report,
                const uint32_t  *event,
                uint64_t        *events_per_iteration
            );

            const
            uint32_t* getRawEvent(EventDescriptor report);


    };


}

#endif
