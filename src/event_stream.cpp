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

#define LIBRORC_INTERNAL
#include <librorc/event_stream.hh>

#include <librorc/buffer.hh>
#include <librorc/device.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/sysmon.hh>
#include <librorc/link.hh>
#include <librorc/dma_channel.hh>

#include <librorc/fastclusterfinder.hh>
#include <librorc/diu.hh>
#include <librorc/ddl.hh>
#include <librorc/siu.hh>
#include <librorc/patterngenerator.hh>

#include <librorc/event_sanity_checker.hh>

namespace LIBRARY_NAME
{

    event_stream::event_stream
    (
        int32_t       deviceId,
        int32_t       channelId,
        LibrorcEsType esType
    )
    {
        m_deviceId        = deviceId;
        m_called_with_bar = false;
        m_channelId       = channelId;
        m_esType          = esType;

        initMembers();
        prepareSharedMemory();
    }

    event_stream::event_stream
    (
        device *dev,
        bar    *bar,
        int32_t          channelId,
        LibrorcEsType    esType
    )
    {
        m_dev             = dev;
        m_bar1            = bar;
        m_deviceId        = dev->getDeviceId();
        m_called_with_bar = true;
        m_channelId       = channelId;
        m_esType          = esType;

        initMembers();
        prepareSharedMemory();
    }

    int
    event_stream::initializeDma
    (
        uint64_t bufferId,
        uint64_t bufferSize
    )
    {
        if( initializeDmaBuffers(bufferId, bufferSize) != 0 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Failed to initialize DMA buffers");
            return -1;
        }
        if( initializeDmaChannel() != 0 )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "Failed to initialize DMA channel");
            return -1;
        }
        return 0;
    }


    void
    event_stream::initMembers()
    {
        m_done             = false;
        m_event_callback   = NULL;
        m_status_callback  = NULL;
        m_raw_event_buffer = NULL;
        m_eventBuffer      = NULL;
        m_reportBuffer     = NULL;
        m_channel          = NULL;
        m_release_map      = NULL;

        if( !m_called_with_bar )
        {
            try
            {
                m_dev = new device(m_deviceId);
#ifdef MODELSIM
                m_bar1 = new sim_bar(m_dev, 1);
#else
                m_bar1 = new rorc_bar(m_dev, 1);
#endif
            }
            catch(...)
            { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }
        }

        /** check if selected channel is available in current FW */
        m_sm = new sysmon(m_bar1);
        if( (uint32_t)m_channelId >= m_sm->numberOfChannels() )
        {
            throw LIBRORC_EVENT_STREAM_ERROR_INVALID_CHANNEL;
        }
        m_fwtype                  = m_sm->firmwareType();
        m_link                    = new link(m_bar1, m_channelId);
        m_linktype                = m_link->linkType();

        if( m_link->dmaEngineIsActive() )
        { throw LIBRORC_EVENT_STREAM_ERROR_BUSY; }

        // get default pciePacketSize from device
        switch(m_esType)
        {
            case LIBRORC_ES_TO_DEVICE:
                m_pciePacketSize = m_dev->maxReadRequestSize();
                break;
            case LIBRORC_ES_TO_HOST:
                m_pciePacketSize = m_dev->maxPayloadSize();
                break;
            default:
                throw LIBRORC_EVENT_STREAM_ERROR_INVALID_ES_TYPE;
                break;
        }

        pthread_mutex_init(&m_releaseEnable, NULL);
        pthread_mutex_init(&m_getEventEnable, NULL);

        /** make sure requested channel is available for selected esType */
        checkLinkTypeCompatibility();
    }


    event_stream::~event_stream()
    {
        deleteParts();

        if(m_channel_status!=NULL)
        {
        #ifdef SHM
            shmdt(m_channel_status);
        #else
            #pragma message "Compiling without SHM"
            free(m_channel_status);
        #endif
        }

        pthread_mutex_destroy(&m_releaseEnable);
        pthread_mutex_destroy(&m_getEventEnable);
    }


    void
    event_stream::deleteParts()
    {
        if( m_sm )
        { delete m_sm; }
        if( m_channel )
        { delete m_channel; }
        if( m_link )
        { delete m_link; }
        deinitializeDmaBuffers();
        if( !m_called_with_bar )
        {
            if( m_bar1 )
            { delete m_bar1; }
            if( m_dev )
            { delete m_dev; }
        }
    }

    void
    event_stream::checkLinkTypeCompatibility()
    {
        bool fwOK;
        switch(m_esType)
        {
            case LIBRORC_ES_TO_HOST:
            { fwOK = (m_linktype!=RORC_CFG_LINK_TYPE_SIU); }
            break;
            case LIBRORC_ES_TO_DEVICE:
            { fwOK = (m_linktype==RORC_CFG_LINK_TYPE_SIU); }
            break;
            default:
            { fwOK = false; }
            break;
        }

        if( !fwOK )
        {
            DEBUG_PRINTF(PDADEBUG_ERROR, "ERROR: selected event stream type \
                    not available for this channel!");
            throw LIBRORC_EVENT_STREAM_ERROR_ES_TYPE_NOT_AVAILABLE;
        }
    }

    int
    event_stream::initializeDmaBuffers
    (
        uint64_t      eventBufferId,
        uint64_t      eventBufferSize
    )
    {
        try
        {
            // allocate a new buffer if a size was provided, else connect
            // to existing buffer
            if( eventBufferSize )
            {
                m_eventBuffer =
                    new buffer
                    (m_dev, eventBufferSize, eventBufferId, 1);
            }
            else
            { m_eventBuffer = new buffer(m_dev, eventBufferId, 1); }

            uint64_t reportBufferSize
                = (m_eventBuffer->getPhysicalSize()/ m_pciePacketSize)
                * sizeof(librorc_event_descriptor);

            // ReportBuffer uses by default EventBuffer-ID + 1
            m_reportBuffer =
                new buffer
                (m_dev, reportBufferSize, (m_eventBuffer->getID()+1), 1);
        }
        catch(...)
        {
            // buffer can throw LIBRORC_BUFFER_ERROR_CONSTRUCTOR_FAILED
            return -1;
        }

        m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
        m_done             = false;
        m_event_callback   = NULL;
        m_status_callback  = NULL;
        m_reports          = (librorc_event_descriptor*)m_reportBuffer->getMem();
        m_release_map      = new bool[m_reportBuffer->getPhysicalSize()/sizeof(librorc_event_descriptor)];

        for(uint64_t i = 0; i<(m_reportBuffer->getPhysicalSize()/sizeof(librorc_event_descriptor)); i++)
        { m_release_map[i] = false; }
        return 0;
    }

    void
    event_stream::deinitializeDmaBuffers()
    {
        if( m_eventBuffer )
        { delete m_eventBuffer; }
        if( m_reportBuffer )
        { delete m_reportBuffer; }
        if( m_release_map )
        { delete[] m_release_map; }
    }


    int
    event_stream::overridePciePacketSize( uint32_t pciePacketSize )
    {
        if( pciePacketSize % 4 )
        { return -1; }
        if( m_esType==LIBRORC_ES_TO_HOST )
        {
            // DMA to Host: 256B write requests max.
            if ( pciePacketSize > 256 )
            { return -1; }
        }
        else
        {
            // DMA to Device: 512B read requests max.
            if ( pciePacketSize > 512 )
            { return -1; }
        }

        m_pciePacketSize = pciePacketSize;
        if(m_channel)
        { m_channel->setPciePacketSize(pciePacketSize); }
        return 0;
    }


    int
    event_stream::initializeDmaChannel()
    {
        if( m_eventBuffer==NULL || m_reportBuffer==NULL )
        { return -1; }

        try
        {
            m_channel = new dma_channel
                (
                 m_channelId,
                 m_pciePacketSize,
                 m_dev,
                 m_bar1,
                 m_eventBuffer,
                 m_reportBuffer,
                 m_esType
                );
        }
        catch(...)
        { return -1; }

        m_channel->enable();
        return 0;
    }



    void
    event_stream::prepareSharedMemory()
    {
        m_channel_status = NULL;
        char *shm        = NULL;

        #ifdef SHM
            int shID =
                shmget(SHM_KEY_OFFSET + m_deviceId*SHM_DEV_OFFSET + m_channelId,
                    sizeof(librorcChannelStatus), IPC_CREAT | 0666);
            if(shID==-1)
            { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }

            /** attach to shared memory */
            shm = (char*)shmat(shID, 0, 0);
            if(shm==(char*)-1)
            { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }
        #else
            #pragma message "Compiling without SHM"
            shm = (char*)malloc(sizeof(librorcChannelStatus));
            if(shm == NULL)
            { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }
        #endif

        m_channel_status = (librorcChannelStatus*)shm;
        clearSharedMemory();
    }



    void
    event_stream::clearSharedMemory()
    {
        if(m_channel_status==NULL)
        { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }

        memset(m_channel_status, 0, sizeof(librorcChannelStatus));

        m_channel_status->index        = EVENT_INDEX_UNDEFINED;
        m_channel_status->shadow_index = 0;
        m_channel_status->last_id      = EVENT_INDEX_UNDEFINED;
        m_channel_status->channel      = (unsigned int)m_channelId;
        m_channel_status->device       = m_deviceId;
    }



    void
    event_stream::printDeviceStatus()
    {
        printf("EventBuffer size: 0x%lx bytes\n", m_eventBuffer->getPhysicalSize());
        printf("ReportBuffer size: 0x%lx bytes\n", m_reportBuffer->getPhysicalSize());
        printf("Bus %x, Slot %x, Func %x\n", m_dev->getBus(), m_dev->getSlot(), m_dev->getFunc() );

        try
        {
            std::cout << "CRORC FPGA" << std::endl
                 << "Firmware Rev. : " << std::hex << std::setw(8)
                 << m_sm->FwRevision()  << std::dec << std::endl
                 << "Firmware Date : " << std::hex << std::setw(8)
                 << m_sm->FwBuildDate() << std::dec << std::endl;
        }
        catch(...)
        { std::cout << "Firmware Rev. and Date not available!" << std::endl; }
    }



    uint64_t
    event_stream::eventLoop(void *user_data)
    {
        m_last_bytes_received  = 0;
        m_last_events_received = 0;

        /** Capture starting time */
        m_bar1->gettime(&m_start_time, 0);
        m_last_time     = m_start_time;
        m_current_time  = m_start_time;

        uint64_t result = 0;
        while( !m_done )
        {
            m_bar1->gettime(&m_current_time, 0);

            result = handleChannelData(user_data);

            if(gettimeofdayDiff(m_last_time, m_current_time)>STAT_INTERVAL)
            {
                m_status_callback
                ? m_status_callback
                  (
                      m_last_time,
                      m_current_time,
                      m_channel_status,
                      m_last_events_received,
                      m_last_bytes_received
                  ) : 0;

                m_last_bytes_received  = m_channel_status->bytes_received;
                m_last_events_received = m_channel_status->n_events;
                m_last_time = m_current_time;
            }

            if(result == 0)
            { usleep(200); } /** no events available */

        }

        m_bar1->gettime(&m_end_time, 0);

        return result;
    }



    uint64_t
    event_stream::dwordOffset(librorc_event_descriptor report_entry)
    {
        return(report_entry.offset / 4);
    }



    uint64_t
    event_stream::getEventIdFromCdh(uint64_t offset)
    {
        // TODO: no event/offset size check here - prone to segmentation faults!
        uint64_t cur_event_id = (uint32_t) * (m_raw_event_buffer + offset + 2) & 0x00ffffff;
        cur_event_id <<= 12;
        cur_event_id |= (uint32_t) * (m_raw_event_buffer + offset + 1) & 0x00000fff;
        return cur_event_id;
    }



    bool
    event_stream::getNextEvent
    (
        librorc_event_descriptor **report,
        const uint32_t           **event,
        uint64_t                  *reference
    )
    {
        pthread_mutex_lock(&m_getEventEnable);

            uint64_t tmp_index = 0;
            if(m_channel_status->index == EVENT_INDEX_UNDEFINED)
            { tmp_index = 0; }
            else
            {
                tmp_index
                    = (m_channel_status->index < m_reportBuffer->getMaxRBEntries()-1)
                    ? (m_channel_status->index+1) : 0;
            }

            if( m_reports[tmp_index].reported_event_size==0 )
            {
                pthread_mutex_unlock(&m_getEventEnable);
                return false;
            }

            m_channel_status->index   =  tmp_index;
            *reference                =  m_channel_status->index;
            *report                   = &m_reports[m_channel_status->index];
            //m_channel_status->last_id =  getEventIdFromCdh(dwordOffset(**report));
            *event                    =  getRawEvent(**report);
        pthread_mutex_unlock(&m_getEventEnable);
        return true;
    }

    void
    event_stream::updateChannelStatus
    (
        librorc_event_descriptor *report
    )
    {
        m_channel_status->bytes_received += (report->calc_event_size << 2);
        m_channel_status->n_events++;
    }


    void
    event_stream::setBufferOffsets()
    {
        if(m_release_map[m_channel_status->shadow_index] != true)
        { return; }

        uint64_t report_buffer_offset;
        uint64_t event_buffer_offset;
        uint64_t shadow_index = m_channel_status->shadow_index;
        uint64_t counter = 0;

        while(m_release_map[m_channel_status->shadow_index] == true)
        {
            m_release_map[m_channel_status->shadow_index] = false;

            event_buffer_offset =
                m_reports[m_channel_status->shadow_index].offset;

            report_buffer_offset =
                ((m_channel_status->shadow_index)*sizeof(librorc_event_descriptor))
                    % m_reportBuffer->getPhysicalSize();

            counter++;

            m_channel_status->shadow_index
                = (m_channel_status->shadow_index < m_reportBuffer->getMaxRBEntries()-1)
                ? (m_channel_status->shadow_index+1) : 0;

            if(m_channel_status->shadow_index==0)
            {
                memset(&m_reports[shadow_index], 0, counter*sizeof(librorc_event_descriptor));
                counter      = 0;
                shadow_index = 0;
            }
        }

        memset(&m_reports[shadow_index], 0, counter*sizeof(librorc_event_descriptor));
        m_channel->setBufferOffsetsOnDevice(event_buffer_offset, report_buffer_offset);
    }

    void
    event_stream::releaseEvent(uint64_t reference)
    {
        pthread_mutex_lock(&m_releaseEnable);
            m_release_map[reference] = true;
            setBufferOffsets();
        pthread_mutex_unlock(&m_releaseEnable);
    }

    uint64_t
    event_stream::handleEvent
    (
        uint64_t                  events_processed,
        void                     *user_data,
        librorc_event_descriptor *report,
        const uint32_t           *event,
        uint64_t                 *events_per_iteration
    )
    {

        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "New RB Entry: offset=0x%lx, calcSize=0x%x, repSize=0x%x\n",
                report->offset, report->calc_event_size, report->reported_event_size);

        events_processed++;

        if( m_event_callback != NULL )
        { m_event_callback(user_data, *report, event, m_channel_status); }

        m_channel_status->bytes_received += (m_reports[m_channel_status->index].calc_event_size << 2);
        m_channel_status->n_events++;
        *events_per_iteration = *events_per_iteration + 1;
        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Event, %d DWs\n",
            m_channel_status->channel,
            report->calc_event_size
        );

        return events_processed;
    }

    uint64_t
    event_stream::handleChannelData(void *user_data)
    {
        uint64_t                  events_processed     = 0;
        librorc_event_descriptor *report               = NULL;
        const uint32_t           *event                = 0;
        uint64_t                  events_per_iteration = 0;
        uint64_t                  reference            = 0;
        uint64_t                  init_reference       = 0;

        /** New event(s) received */
        if( getNextEvent(&report, &event, &init_reference) )
        {
            events_processed =
                handleEvent
                (
                    events_processed,
                    user_data,
                    report,
                    event,
                    &events_per_iteration
                );

            /** handle all following entries */
            while( getNextEvent(&report, &event, &reference) )
            {
                events_processed =
                    handleEvent
                    (
                        events_processed,
                        user_data,
                        report,
                        event,
                        &events_per_iteration
                    );

                releaseEvent(reference);
            }

            releaseEvent(init_reference);

            if(events_per_iteration > m_channel_status->max_epi)
            { m_channel_status->max_epi = events_per_iteration; }

            if(events_per_iteration < m_channel_status->min_epi)
            { m_channel_status->min_epi = events_per_iteration; }

            events_per_iteration = 0;
            m_channel_status->set_offset_count++;
        }

        return events_processed;
    }

    const uint32_t*
    event_stream::getRawEvent(librorc_event_descriptor report)
    {
        return (const uint32_t*)&m_raw_event_buffer[report.offset/4];
    }



    /************************* Generators *************************/

    patterngenerator*
    event_stream::getPatternGenerator()
    {
        if( m_link->patternGeneratorAvailable() )
        { return new patterngenerator(m_link); }
        else
        { return NULL; }
    }

    diu*
    event_stream::getDiu()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_DIU)
        { return new diu(m_link); }
        else
        { return NULL; }
    }

    siu*
    event_stream::getSiu()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_SIU)
        { return new siu(m_link); }
        else
        { return NULL; }
    }

    fastclusterfinder*
    event_stream::getFastClusterFinder()
    {
        if(m_fwtype==RORC_CFG_PROJECT_hlt_in_fcf &&
                m_linktype==RORC_CFG_LINK_TYPE_DIU)
        { return new fastclusterfinder(m_link); }
        else
        { return NULL; }
    }

    ddl*
    event_stream::getRawReadout()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_VIRTUAL)
        { return new ddl(m_link); }
        else
        { return NULL; }
    }

}
