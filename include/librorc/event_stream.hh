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

#ifndef LIBRORC_EVENT_STREAM_H
#define LIBRORC_EVENT_STREAM_H

#include <librorc/include_ext.hh>
#include "defines.hh"
#include <librorc/buffer.hh>

#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED     1
#define LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED   2
#define LIBRORC_EVENT_STREAM_ERROR_BUSY                   4
#define LIBRORC_EVENT_STREAM_ERROR_INVALID_ES_TYPE        5
#define LIBRORC_EVENT_STREAM_ERROR_INVALID_CHANNEL        6
#define LIBRORC_EVENT_STREAM_ERROR_ES_TYPE_NOT_AVAILABLE  8

#define MAX_EVENTS_PER_ITERATION 0x0

/** Buffer Sizes (in Bytes) **/
#ifndef MODELSIM
    #define EBUFSIZE (((uint64_t)1) << 28)
    #define STAT_INTERVAL 1.0
#else
    #define EBUFSIZE (((uint64_t)1) << 19)
    #define STAT_INTERVAL 0.00001
#endif

/** Shared mem key offset **/
#define SHM_KEY_OFFSET 2048
/** Shared mem device offset **/
#define SHM_DEV_OFFSET 32

#define EVENT_INDEX_UNDEFINED 0xffffffffffffffff

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

class buffer;
class device;
class bar;
class sysmon;
class dma_channel;
class patterngenerator;
class link;
class diu;
class siu;
class ddl;
class fastclusterfinder;

    /**
     * @class event_stream
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

#ifdef LIBRORC_INTERNAL
             event_stream
             (
                device          *dev,
                bar             *bar,
                int32_t          channelId,
                LibrorcEsType    esType
             );
#endif

            ~event_stream();
            void initMembers();

            /**
             * Check if selected channel is available in current
             * firmware and if selected esType is possible for
             * this channel.
             * throws exception if check fails.
             **/
            void checkLinkTypeCompatibility();

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
             * @param [out] event
             *        Pointer to the event payload.
             * @param [out] Reference to the returned event. Used releaseEvent ...
             *
             * @return true if there was a new event and false if the buffer was empty
             */
            bool
            getNextEvent
            (
                librorc_event_descriptor **report,
                const uint32_t           **event,
                uint64_t                  *reference
            );

            /**
             * update channel status after successful getNextEvent.
             * This adjusts bytes_received and n_events.
             * @param report received reportbuffer descriptor
             **/
            void updateChannelStatus( librorc_event_descriptor *report);

            /**
             * Release the event which was obtained by getNextEvent().
             * @param [in] reference to the report entry which was obtained with
             *        getNextEvent().
             * @return 0 on success, -1 on invalid reference
             */
            int releaseEvent(uint64_t reference);

            /**
             * @internal
             */
            uint64_t handleChannelData(void *user_data);


            /**
             * get PatternGenerator instance for current event_stream
             * @return pointer to instance of patterngenerator when
             * available, NULL when PatternGenerator not available for
             * current event_stream
             **/
            patterngenerator* getPatternGenerator();

            /**
             * get DIU instance for current event_stream.
             * @return pointer to instance of diu when available, NULL
             * when not available for current event_stream
             **/
            diu* getDiu();

            /**
             * get RawReadout instance for current event_stream.
             * @return pointer to instance of ddl when available, NULL
             * when not available for current event_stream
             **/
            ddl* getRawReadout();

            /**
             * get SIU instance for current event_stream.
             * @return pointer to instance of siu when available, NULL
             * when not available for current event_stream.
             **/
            siu* getSiu();

            /**
             * get fastclusterfinder instance for current event_stream.
             * @return pointer to instance of fastclusterfinder when
             * available, NULL when not available for current event_stream.
             **/
            fastclusterfinder* getFastClusterFinder();

            /**
             * override the default PCIe packet size from the PCIe
             * subsystem with a custom value
             * @param pciePacketSize packet size in Bytes
             * @return 0 on success, -1 on invalid packet size
             **/
            int overridePciePacketSize( uint32_t pciePacketSize );

            /** Member Variables */


            bar         *m_bar1;
            buffer      *m_eventBuffer;
            buffer      *m_reportBuffer;
            device      *m_dev;
            sysmon      *m_sm;
            dma_channel *m_channel;
            link        *m_link;

            bool         m_done;
            uint64_t     m_last_bytes_received;
            uint64_t     m_last_events_received;

            timeval      m_start_time;
            timeval      m_end_time;
            timeval      m_last_time;
            timeval      m_current_time;

            librorcChannelStatus *m_channel_status;

            int
            initializeDma
            (
                uint64_t      eventBufferId,
                uint64_t      eventBufferSize
            );

            int  initializeDmaBuffers(uint64_t eventBufferId, uint64_t eventBufferSize );
            void deinitializeDmaBuffers();

        protected:
            int32_t          m_deviceId;
            int32_t          m_channelId;
            LibrorcEsType    m_esType;
            uint32_t         m_pciePacketSize;
            bool             m_called_with_bar;
            bool            *m_release_map;
            uint64_t         m_release_map_entries;
            pthread_mutex_t  m_releaseEnable;
            pthread_mutex_t  m_getEventEnable;

            volatile uint32_t        *m_raw_event_buffer;
            librorc_event_descriptor *m_reports;

            uint32_t  m_fwtype;
            uint32_t  m_linktype;

            librorc_event_callback  m_event_callback;
            librorc_status_callback m_status_callback;

            uint64_t        m_last_event_buffer_offset;

            int      initializeDmaChannel();

            void     prepareSharedMemory();
            void     deleteParts();
            uint64_t dwordOffset(librorc_event_descriptor report_entry);
            uint64_t getEventIdFromCdh(uint64_t offset);
            void     setBufferOffsets();

            uint64_t
            handleEvent
            (
                uint64_t                  events_processed,
                void                     *user_data,
                librorc_event_descriptor *report,
                const uint32_t           *event,
                uint64_t                 *events_per_iteration
            );

            void clearSharedMemory();

            const
            uint32_t* getRawEvent(librorc_event_descriptor report);

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
