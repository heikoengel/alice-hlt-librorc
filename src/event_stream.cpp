/**
 * @file
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

#define LIBRORC_INTERNAL
#include <librorc/event_stream.hh>

#include <librorc/device.hh>
#include <librorc/buffer.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/sysmon.hh>
#include <librorc/link.hh>

#include <librorc/dma_channel.hh>
#include <librorc/event_sanity_checker.hh>

using namespace std;

namespace LIBRARY_NAME
{

    event_stream::event_stream
    (
        int32_t       deviceId,
        int32_t       channelId,
        LibrorcEsType esType,
        uint64_t      bufferSize
    )
    {
        m_deviceId        = deviceId;
        m_called_with_bar = false;
        m_channelId       = channelId;

        initMembers();
        initializeDmaBuffers(esType, EBUFSIZE, RBUFSIZE);
        initializeDmaChannel(esType);
        prepareSharedMemory();
    }

    event_stream::event_stream
    (
        librorc::device *dev,
        librorc::bar    *bar,
        int32_t          channelId,
        LibrorcEsType    esType,
        uint64_t         bufferSize
    )
    {
        m_dev             = dev;
        m_bar1            = bar;
        m_deviceId        = dev->getDeviceId();
        m_called_with_bar = true;
        m_channelId       = channelId;

        initMembers();
        initializeDmaBuffers(esType, EBUFSIZE, RBUFSIZE);
        initializeDmaChannel(esType);
        prepareSharedMemory();
    }

    void
    event_stream::initMembers()
    {
        m_done             = false;
        m_event_callback   = NULL;
        m_status_callback  = NULL;
        m_raw_event_buffer = NULL;
        try
        {
            if( !m_called_with_bar )
            {
                m_dev = new librorc::device(m_deviceId);
                #ifdef SIM
                    m_bar1 = new librorc::sim_bar(m_dev, 1);
                #else
                    m_bar1 = new librorc::rorc_bar(m_dev, 1);
                #endif
            }
        }
        catch(...)
        {
            cout << "initMembers failed!" << endl;
            throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
        }

        m_link                    = new librorc::link(m_bar1, m_channelId);
        m_sm                      = new librorc::sysmon(m_bar1);
        m_fwtype                  = m_sm->firmwareType();
        m_linktype                = m_link->linkType();
        m_event_generation_offset = 0;

        pthread_mutex_init(&m_releaseEnable, NULL);
        pthread_mutex_init(&m_getEventEnable, NULL);
    }


    event_stream::~event_stream()
    {
        deleteParts();
        shmdt(m_channel_status);

        pthread_mutex_destroy(&m_releaseEnable);
        pthread_mutex_destroy(&m_getEventEnable);
    }


    void
    event_stream::deleteParts()
    {
        delete m_sm;
        delete m_channel;
        delete m_eventBuffer;
        delete m_reportBuffer;
        delete m_link;
        delete[] m_release_map;
        if( !m_called_with_bar )
        {
            delete m_bar1;
            delete m_dev;
        }
    }

    void
    event_stream::checkFirmwareCompatibility(LibrorcEsType esType)
    {
        int32_t availDmaChannels = m_sm->numberOfChannels();
        if( m_channelId >= availDmaChannels )
        {
            cout << "ERROR: Requsted channel " << m_channelId
                 << " is not implemented in firmware "
                 << "- exiting" << endl;
            throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
        }

        bool fwOK;
        switch(esType)
        {
            case LIBRORC_ES_TO_HOST:
            {
                fwOK =
                      m_sm->firmwareIsHltHardwareTest()
                    | m_sm->firmwareIsHltIn()
                    | m_sm->firmwareIsHltInFcf();
            }
            break;

            case LIBRORC_ES_TO_DEVICE:
            { fwOK = m_sm->firmwareIsHltOut(); }
            break;

            default:
            { fwOK = false; }
            break;
        }

        if( !fwOK )
        {
            cout << "ERROR: Wrong or unknown firmware loaded!"
                 << endl;
            throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
        }
    }

    void
    event_stream::initializeDmaBuffers
    (
        LibrorcEsType esType,
        uint64_t eventBufferSize,
        uint64_t reportBufferSize
    )
    {
        /** make sure requested channel is available for selected esType */
        checkFirmwareCompatibility(esType);

        /** TODO : remove this? */
        /** set EventBuffer DMA direction according to EventStream type */
        int32_t dma_direction;
        switch (esType)
        {
            case LIBRORC_ES_TO_DEVICE:
            { dma_direction = LIBRORC_DMA_TO_DEVICE; }
            break;

            default:
            { dma_direction = LIBRORC_DMA_FROM_DEVICE; }
            break;
        }

        try
        {
            /**
             * TODO: this simply connects to an unknown buffer
             * if a buffer with the given ID already exists.
             * => Check if buffer of requested size already exists:
             * YES: connect to existing buffer
             * NO : check if dma_channel is already active
             *      (e.g. another process is already using
             *      these buffers). only re-allocate with
             *      requested size if not active!
             **/
            //uint64_t
            //reportBufferSize = eventBufferSize / m_dev->maxPayloadSize();

            m_eventBuffer =
                new librorc::buffer
                    (m_dev, eventBufferSize, (2*m_channelId), 1, dma_direction);

            m_reportBuffer =
                new librorc::buffer
                    (m_dev, reportBufferSize, (2*m_channelId+1), 1, LIBRORC_DMA_FROM_DEVICE);

            m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
            m_done             = false;
            m_event_callback   = NULL;
            m_status_callback  = NULL;
            m_reports          = (librorc_event_descriptor*)m_reportBuffer->getMem();
            m_release_map      = new bool[m_reportBuffer->size()/sizeof(librorc_event_descriptor)];
        }
        catch(...)
        {
            cout << "initializeDmaBuffers failed" << endl;
            throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
        }

        for(uint64_t i = 0; i<(m_reportBuffer->size()/sizeof(librorc_event_descriptor)); i++)
        { m_release_map[i] = false; }
    }



    void
    event_stream::initializeDmaChannel(LibrorcEsType esType)
    {
        /*
         * TODO: get MaxPayload/MaxReadReq sizes from PDA!
         **/
        uint32_t max_pkt_size = 0;
        if( esType == LIBRORC_ES_TO_HOST )
        { max_pkt_size = m_dev->maxPayloadSize(); }
        else
        { max_pkt_size = 128; }

        m_channel = new librorc::dma_channel
        (
            m_channelId,
            max_pkt_size,
            m_dev,
            m_bar1,
            m_eventBuffer,
            m_reportBuffer
        );

        m_channel->enable();
    }



    void
    event_stream::prepareSharedMemory()
    {
        m_channel_status = NULL;

        int shID =
            shmget(SHM_KEY_OFFSET + m_deviceId*SHM_DEV_OFFSET + m_channelId,
                sizeof(librorcChannelStatus), IPC_CREAT | 0666);
        if(shID==-1)
        {
            cout << "prepareSharedMemory failed (1)!" << endl;
            throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED);
        }

        /** attach to shared memory */
        char *shm = (char*)shmat(shID, 0, 0);
        if(shm==(char*)-1)
        {
            cout << "prepareSharedMemory failed (2)!" << endl;
            throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED);
        }

        m_channel_status = (librorcChannelStatus*)shm;
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
        printf("EventBuffer size: 0x%lx bytes\n", m_eventBuffer->size());
        printf("ReportBuffer size: 0x%lx bytes\n", m_reportBuffer->size());
        printf("Bus %x, Slot %x, Func %x\n", m_dev->getBus(), m_dev->getSlot(), m_dev->getFunc() );

        try
        {
            cout << "CRORC FPGA" << endl
                 << "Firmware Rev. : " << hex << setw(8) << m_sm->FwRevision()  << dec << endl
                 << "Firmware Date : " << hex << setw(8) << m_sm->FwBuildDate() << dec << endl;
        }
        catch(...)
        { cout << "Firmware Rev. and Date not available!" << endl; }
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
            m_channel_status->last_id =  getEventIdFromCdh(dwordOffset(**report));
            *event                    =  getRawEvent(**report);
        pthread_mutex_unlock(&m_getEventEnable);
        return true;
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

        events_processed++;

        if(0 != (m_event_callback != NULL) ? m_event_callback(user_data, *report, event, m_channel_status) : 1)
        {
            cout << "Event Callback is not set!" << endl;
            abort();
        }

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


    patterngenerator*
    event_stream::getPatternGenerator()
    {
        if( m_link->patternGeneratorAvailable() )
        { return new patterngenerator(m_link); }
        else
        { return NULL; }
        // TODO: log message: getPatternGenerator failed, Firmware, channelId
    }


    diu*
    event_stream::getDiu()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_DIU)
        { return new diu(m_link); }
        else
        { return NULL; }
        // TODO: log message: getDiu failed, Firmware, channelId
    }


    siu*
    event_stream::getSiu()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_SIU)
        { return new siu(m_link); }
        else
        { return NULL; }
        // TODO: log message: getSiu failed, Firmware, channelId
    }

/** HLT out API ---------------------------------------------------------------*/

    void
    event_stream::packEventIntoBuffer
    (
        uint32_t *event,
        uint32_t  event_size
    )
    {
        uint32_t *dest = (m_eventBuffer->getMem() + (m_event_generation_offset >> 2));

        memcpy((void*) (dest), event, (event_size * sizeof(uint32_t)));
        pushEventSizeIntoELFifo(event_size);
        iterateEventBufferFillState(event_size);
        wrapFillStateIfNecessary();
    }

    uint64_t
    event_stream::availableBufferSpace()
    {
        m_last_event_buffer_offset
            = m_channel->getLastEBOffset();

        return   (m_event_generation_offset < m_last_event_buffer_offset)
               ? m_last_event_buffer_offset - m_event_generation_offset
               : m_last_event_buffer_offset + m_eventBuffer->size()
               - m_event_generation_offset; /** wrap in between */
    }

    void
    event_stream::pushEventSizeIntoELFifo(uint32_t event_size)
    {
        m_channel->getLink()->setPacketizer(RORC_REG_DMA_ELFIFO, event_size);
    }

    void
    event_stream::iterateEventBufferFillState(uint32_t event_size)
    {
        m_event_generation_offset += fragmentSize(event_size);
    }

    void
    event_stream::wrapFillStateIfNecessary()
    {
        m_event_generation_offset
            = (m_event_generation_offset >= m_eventBuffer->size())
            ? (m_event_generation_offset - m_eventBuffer->size())
            : m_event_generation_offset;
    }

    uint32_t
    event_stream::fragmentSize(uint32_t event_size)
    {
        uint32_t max_read_req = m_channel->pciePacketSize();

        return   ((event_size << 2) % max_read_req)
               ? (trunc((event_size << 2) / max_read_req) + 1) * max_read_req
               : (event_size << 2);
    }

    uint64_t
    event_stream::numberOfEvents(uint32_t event_size)
    {

        if(!isSufficientFifoSpaceAvailable())
        { return 0; }

        uint64_t
        number_of_events
            = numberOfEventsThatFitIntoBuffer
                (availableBufferSpace(), event_size, fragmentSize(event_size));

        number_of_events
            = maximumElfifoCanHandle(number_of_events);

        number_of_events
            = reduceNumberOfEventsToCustomMaximum(number_of_events);

        return number_of_events;
    }

    bool
    event_stream::isSufficientFifoSpaceAvailable()
    {
        uint32_t el_fifo_state       = m_channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO);
        uint32_t el_fifo_write_limit = ((el_fifo_state >> 16) & 0x0000ffff);
        uint32_t el_fifo_write_count = (el_fifo_state & 0x0000ffff);
        return !(el_fifo_write_count + 10 >= el_fifo_write_limit);
    }

    uint64_t
    event_stream::numberOfEventsThatFitIntoBuffer
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

    uint64_t
    event_stream::maximumElfifoCanHandle(uint64_t number_of_events)
    {
        uint32_t el_fifo_state       = m_channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO);
        uint32_t el_fifo_write_limit = ((el_fifo_state >> 16) & 0x0000ffff);
        uint32_t el_fifo_write_count = (el_fifo_state & 0x0000ffff);

        return
        (el_fifo_write_limit - el_fifo_write_count < number_of_events)
        ? (el_fifo_write_limit - el_fifo_write_count) : number_of_events;
    }

    uint64_t
    event_stream::reduceNumberOfEventsToCustomMaximum(uint64_t number_of_events)
    {
        return
        (MAX_EVENTS_PER_ITERATION && number_of_events > MAX_EVENTS_PER_ITERATION)
        ? MAX_EVENTS_PER_ITERATION : number_of_events;
    }

    fastclusterfinder*
    event_stream::getFastClusterFinder()
    {
        if(m_fwtype==RORC_CFG_PROJECT_hlt_in_fcf &&
                m_linktype==RORC_CFG_LINK_TYPE_DIU)
        { return new fastclusterfinder(m_link); }
        else
        {
            // TODO: log message: getSiu failed,
            // Firmware, channelId
            return NULL;
        }
    }

    ddl*
    event_stream::getRawReadout()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_VIRTUAL)
        { return new ddl(m_link); }
        else
        {
            // TODO: log message: getRawReadout failed,
            // Firmware, channelId
            return NULL;
        }
    }

}
