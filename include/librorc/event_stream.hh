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

/** Shared mem key offset **/
#define SHM_KEY_OFFSET 2048
/** Shared mem device offset **/
#define SHM_DEV_OFFSET 32


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
    uint32_t channel;
    uint32_t device;
}ChannelStatus;

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
                uint32_t      deviceId,
                uint32_t      channelId,
                LibrorcEsType esType
             );

#ifdef LIBRORC_INTERNAL
             event_stream
             (
                device          *dev,
                bar             *bar,
                uint32_t         channelId,
                LibrorcEsType    esType
             );
#endif

            ~event_stream();

            /**
             * Check if selected channel is available in current
             * firmware and if selected esType is possible for
             * this channel.
             * throws exception if check fails.
             **/
            void checkLinkTypeCompatibility();

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
                EventDescriptor **report,
                const uint32_t  **event,
                uint64_t         *reference
            );

            /**
             * update channel status after successful getNextEvent.
             * This adjusts bytes_received and n_events.
             * @param report received reportbuffer descriptor
             **/
            void updateChannelStatus( EventDescriptor *report);

            /**
             * Release the event which was obtained by getNextEvent().
             * @param [in] reference to the report entry which was obtained with
             *        getNextEvent().
             * @return 0 on success, -1 on invalid reference
             */
            int releaseEvent(uint64_t reference);

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
            ChannelStatus *m_channel_status;

            int initializeDma(uint64_t eventBufferId, uint64_t eventBufferSize);
            int initializeDmaBuffers(uint64_t eventBufferId, uint64_t eventBufferSize);
            void deinitializeDmaBuffers();

        protected:
            uint32_t         m_deviceId;
            uint32_t         m_channelId;
            uint32_t         m_fwtype;
            uint32_t         m_linktype;
            LibrorcEsType    m_esType;
            uint32_t         m_pciePacketSize;
            bool             m_called_with_bar;
            bool            *m_release_map;
            uint64_t         m_max_rb_entries;
            pthread_mutex_t  m_releaseEnable;
            pthread_mutex_t  m_getEventEnable;

            volatile uint32_t *m_raw_event_buffer;
            EventDescriptor   *m_reports;

            void     initMembers();
            int      initializeDmaChannel();
            void     prepareSharedMemory();
            void     deleteParts();
            void     updateBufferOffsets();
            void     clearSharedMemory();

            const
            uint32_t* getRawEvent(EventDescriptor report);

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
