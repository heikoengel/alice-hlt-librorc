/**
 * @file
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>, Heiko Engel <hengel@cern.ch>
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
        librorc::device *dev,
        librorc::bar    *bar,
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

        if( !m_called_with_bar )
        {
            try
            {
                m_dev = new librorc::device(m_deviceId);
#ifdef MODELSIM
                m_bar1 = new librorc::sim_bar(m_dev, 1);
#else
                m_bar1 = new librorc::rorc_bar(m_dev, 1);
#endif
            }
            catch(...)
            { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }
        }

        /** check if selected channel is available in current FW */
        m_sm = new librorc::sysmon(m_bar1);
        if( (uint32_t)m_channelId >= m_sm->numberOfChannels() )
        {
            cout << "ERROR: Requsted channel " << m_channelId
                 << " is not implemented in firmware "
                 << "- exiting" << endl;
            throw LIBRORC_EVENT_STREAM_ERROR_INVALID_CHANNEL;
        }
        m_fwtype                  = m_sm->firmwareType();
        m_link                    = new librorc::link(m_bar1, m_channelId);
        m_linktype                = m_link->linkType();
        m_event_generation_offset = 0;

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
                    new librorc::buffer
                    (m_dev, eventBufferSize, eventBufferId, 1);
            }
            else
            { m_eventBuffer = new librorc::buffer(m_dev, eventBufferId); }

            uint64_t reportBufferSize
                = (m_eventBuffer->size()/ m_pciePacketSize)
                * sizeof(librorc_event_descriptor);

            // ReportBuffer uses by default EventBuffer-ID + 1
            m_reportBuffer =
                new librorc::buffer
                (m_dev, reportBufferSize, (m_eventBuffer->getID()+1), 1);
        }
        catch(...)
        {
            // TODO: different return codes for different exceptions
            // TODO: which exceptions can be thrown?
            return -1;
        }

        m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
        m_done             = false;
        m_event_callback   = NULL;
        m_status_callback  = NULL;
        m_reports          = (librorc_event_descriptor*)m_reportBuffer->getMem();
        m_release_map      = new bool[m_reportBuffer->size()/sizeof(librorc_event_descriptor)];

        for(uint64_t i = 0; i<(m_reportBuffer->size()/sizeof(librorc_event_descriptor)); i++)
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


    void
    event_stream::overridePciePacketSize( uint32_t pciePacketSize )
    {
        //TODO: validity checks here?!
        m_pciePacketSize = pciePacketSize;
        if(m_channel)
        { m_channel->setPciePacketSize(pciePacketSize); }
    }


    int
    event_stream::initializeDmaChannel()
    {
        if( m_eventBuffer==NULL || m_reportBuffer==NULL )
        { return -1; }

        try
        {
            m_channel = new librorc::dma_channel
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
            {
                cout << "prepareSharedMemory failed (1)!" << endl;
                throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED);
            }

            /** attach to shared memory */
            shm = (char*)shmat(shID, 0, 0);
            if(shm==(char*)-1)
            {
                cout << "prepareSharedMemory failed (2)!" << endl;
                throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED);
            }
        #else
            #pragma message "Compiling without SHM"
            shm = (char*)malloc(sizeof(librorcChannelStatus));
            if(shm == NULL)
            {
                cout << "prepareSharedMemory failed (2)!" << endl;
                throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED);
            }
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
        m_channel->getLink()->setPciReg(RORC_REG_DMA_ELFIFO, event_size);
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

    /** TODO : this does not work, because the event size in HLT_OUT is usually not fixed*/
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
        uint32_t el_fifo_state       = m_channel->getLink()->pciReg(RORC_REG_DMA_ELFIFO);
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
        uint32_t el_fifo_state       = m_channel->getLink()->pciReg(RORC_REG_DMA_ELFIFO);
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
}
