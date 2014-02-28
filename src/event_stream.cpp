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
        LibrorcEsType    esType
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
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }

        m_link     = new librorc::link(m_bar1, m_channelId);
        m_sm       = new librorc::sysmon(m_bar1);
        m_fwtype   = m_sm->firmwareType();
        m_linktype = m_link->linkType();
    }


    event_stream::~event_stream()
    {
        deleteParts();
        shmdt(m_channel_status);
    }


    void
    event_stream::deleteParts()
    {
        delete m_sm;
        delete m_channel;
        delete m_eventBuffer;
        delete m_reportBuffer;
        delete m_link;
        if ( !m_called_with_bar )
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
                fwOK = m_sm->firmwareIsHltHardwareTest() |
                    m_sm->firmwareIsHltIn() |
                    m_sm->firmwareIsHltInFcf();
                break;
            case LIBRORC_ES_TO_DEVICE:
                fwOK = m_sm->firmwareIsHltOut();
                break;
            default:
                fwOK = false;
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
            {
                dma_direction = LIBRORC_DMA_TO_DEVICE;
            }
            break;
            default:
            {
                dma_direction = LIBRORC_DMA_FROM_DEVICE;
            }
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
            m_eventBuffer = new librorc::buffer(m_dev, eventBufferSize,
                        (2*m_channelId), 1, dma_direction);
            m_reportBuffer = new librorc::buffer(m_dev, reportBufferSize,
                        (2*m_channelId+1), 1, LIBRORC_DMA_FROM_DEVICE);


            m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
        }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }

    }


    void
    event_stream::initializeDmaChannel(LibrorcEsType esType)
    {
        /*
         * TODO: get MaxPayload/MaxReadReq sizes from PDA!
         **/
        uint32_t max_pkt_size = 0;
        if( esType == LIBRORC_ES_TO_HOST )
        {
            max_pkt_size = MAX_PAYLOAD;
        }
        else
        {
            max_pkt_size = 128;
        }

        m_channel = new librorc::dma_channel(
                m_channelId,
                max_pkt_size,
                m_dev,
                m_bar1,
                m_eventBuffer,
                m_reportBuffer);

        //m_bar1->simSetPacketSize(32);
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
        { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }

        /** attach to shared memory */
        char *shm = (char*)shmat(shID, 0, 0);
        if(shm==(char*)-1)
        { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }

        m_channel_status = (librorcChannelStatus*)shm;

        /*memset(m_channel_status, 0, sizeof(librorcChannelStatus));
        m_channel_status->index = 0;
        m_channel_status->last_id = 0xfffffffff;
        m_channel_status->channel = (unsigned int)m_channelId;*/
    }


    void
    event_stream::clearSharedMemory()
    {
        if(m_channel_status==NULL)
        { throw(LIBRORC_EVENT_STREAM_ERROR_SHARED_MEMORY_FAILED); }

        memset(m_channel_status, 0, sizeof(librorcChannelStatus));
        m_channel_status->index = 0;
        m_channel_status->last_id = 0xfffffffff;
        m_channel_status->channel = (unsigned int)m_channelId;
    }





    void
    event_stream::printDeviceStatus()
    {
        printf("EventBuffer size: 0x%lx bytes\n", EBUFSIZE);
        printf("ReportBuffer size: 0x%lx bytes\n", RBUFSIZE);
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
    event_stream::eventLoop
    (
        void *user_data
    )
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



    uint64_t
    event_stream::handleChannelData(void *user_data)
    {
        librorc_event_descriptor *reports
            = (librorc_event_descriptor*)m_reportBuffer->getMem();

        //TODO: make this global
        uint64_t events_processed = 0;
        /** new event received */
        if( reports[m_channel_status->index].reported_event_size!=0 )
        {
            // capture index of the first found reportbuffer entry
            uint64_t starting_index       = m_channel_status->index;
            uint64_t events_per_iteration = 0;
            uint64_t event_buffer_offset  = 0;
            uint64_t report_buffer_offset = 0;

            // handle all following entries
            while( reports[m_channel_status->index].reported_event_size!=0 )
            {
                // increment number of events processed in this interation
                events_processed++;

                      librorc_event_descriptor  report   = reports[m_channel_status->index];
                      uint64_t                  event_id = getEventIdFromCdh(dwordOffset(report));
                const uint32_t                 *event    = getRawEvent(report);

                /**
                 * TODO: reported_event_size and calc_event_size still contain error/status flags
                 * at this point. Handle these flags and mask upper two bits for both sizes before
                 * using them as actual wordcounts.
                 **/
                uint64_t ret = (m_event_callback != NULL)
                    ? m_event_callback(user_data, event_id, report, event, m_channel_status)
                    : 1;

                if(ret != 0)
                {
                    cout << "Event Callback is set!" << endl;
                    abort();
                }

                m_channel_status->last_id = event_id;

                // increment the number of bytes received
                // Note that the upper two status bits are shifted out!
                m_channel_status->bytes_received +=
                    (reports[m_channel_status->index].calc_event_size<<2);

                // save new EBOffset
                event_buffer_offset = reports[m_channel_status->index].offset;

                // increment report-buffer offset
                report_buffer_offset
                    = ((m_channel_status->index)*sizeof(librorc_event_descriptor))
                    % m_reportBuffer->getPhysicalSize();

                // wrap RB index if necessary
                m_channel_status->index
                    = (m_channel_status->index < m_reportBuffer->getMaxRBEntries()-1)
                    ? (m_channel_status->index+1) : 0;

                //increment total number of events received
                m_channel_status->n_events++;

                //increment number of events processed in this while-loop
                events_per_iteration++;

                DEBUG_PRINTF
                (
                     PDADEBUG_CONTROL_FLOW,
                     "CH %d - Event, %d DWs\n",
                     m_channel_status->channel,
                     (report.calc_event_size & 0x7fffffff)
                );
            }

            // clear processed report-buffer entries
            memset(&reports[starting_index], 0, events_per_iteration*sizeof(librorc_event_descriptor) );


            // update min/max statistics on how many events have been received
            // in the above while-loop
            if(events_per_iteration > m_channel_status->max_epi)
            { m_channel_status->max_epi = events_per_iteration; }

            if(events_per_iteration < m_channel_status->min_epi)
            { m_channel_status->min_epi = events_per_iteration; }

            events_per_iteration = 0;
            m_channel_status->set_offset_count++;

            // actually update the offset pointers in the firmware
            m_channel->setBufferOffsetsOnDevice(event_buffer_offset, report_buffer_offset);

            DEBUG_PRINTF
            (
                PDADEBUG_CONTROL_FLOW,
                "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
                m_channel_status->channel,
                report_buffer_offset,
                event_buffer_offset
            );
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
        if(m_fwtype==RORC_CFG_PROJECT_hlt_in ||
                m_fwtype==RORC_CFG_PROJECT_hlt_out || 
                m_fwtype==RORC_CFG_PROJECT_hwtest)
        {
            return new patterngenerator(m_link);
        }
        else
        {
            // TODO: log message: getPatternGenerator failed,
            // Firmware, channelId
            return NULL;
        }
    }


    diu*
    event_stream::getDiu()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_DIU)
        {
            return new diu(m_link);
        }
        else
        {
            // TODO: log message: getDiu failed,
            // Firmware, channelId
            return NULL;
        }
    }


    siu*
    event_stream::getSiu()
    {
        if(m_linktype==RORC_CFG_LINK_TYPE_SIU)
        {
            return new siu(m_link);
        }
        else
        {
            // TODO: log message: getSiu failed,
            // Firmware, channelId
            return NULL;
        }
    }


}
