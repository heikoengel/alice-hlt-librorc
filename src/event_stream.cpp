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

#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_ddl.hh>
#include <librorc/dma_channel_pg.hh>
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
        m_deviceId = deviceId;
        m_called_with_bar = false;
        generateDMAChannel(m_deviceId, channelId, LIBRORC_ES_IN_GENERIC);
        prepareSharedMemory();
    }



    event_stream::event_stream
    (
        int32_t       deviceId,
        int32_t       channelId,
        uint32_t      eventSize,
        LibrorcEsType esType
    )
    {
        m_deviceId        = deviceId;
        m_eventSize       = eventSize;
        m_called_with_bar = false;
        generateDMAChannel(m_deviceId, channelId, esType);
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
        generateDMAChannel(m_deviceId, channelId, LIBRORC_ES_IN_GENERIC);
        prepareSharedMemory();
    }



    event_stream::event_stream
    (
        librorc::device *dev,
        librorc::bar    *bar,
        int32_t          channelId,
        uint32_t         eventSize,
        LibrorcEsType    esType
    )
    {
        m_dev             = dev;
        m_bar1            = bar;
        m_eventSize       = eventSize;
        m_deviceId        = dev->getDeviceId();
        m_called_with_bar = true;

        generateDMAChannel(m_deviceId, channelId, esType);
        prepareSharedMemory();
    }


    event_stream::~event_stream()
    {
        deleteParts();
        shmdt(m_channel_status);
    }



    void
    event_stream::deleteParts()
    {
        delete m_channel;
        delete m_eventBuffer;
        delete m_reportBuffer;
        if ( !m_called_with_bar )
        {
            delete m_bar1;
            delete m_dev;
        }
    }

    void
    event_stream::checkFirmware(LibrorcEsType esType)
    {
        sysmon monitor(m_bar1);

        if
        (
            esType == LIBRORC_ES_IN_GENERIC ||
            esType == LIBRORC_ES_IN_DDL     ||
            esType == LIBRORC_ES_IN_HWPG
        )
        {
            if( !monitor.firmwareIsHltIn() && !monitor.firmwareIsHltHardwareTest())
            {
                cout << "Wrong device firmware loaded [out] instead [in]" << endl;
                throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
            }
        }

        if
        (
            esType == LIBRORC_ES_OUT_GENERIC ||
            esType == LIBRORC_ES_OUT_SWPG ||
            esType == LIBRORC_ES_OUT_FILE
        )
        {
            if( !monitor.firmwareIsHltOut() )
            {
                cout << "Wrong device firmware loaded [in] instead [out]" << endl;
                throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
            }
        }
    }

    void
    event_stream::generateDMAChannel
    (
        int32_t       deviceId,
        int32_t       channelId,
        LibrorcEsType esType
    )
    {
        m_channelId = channelId;

        /** TODO : remove this */
        /** set EventBuffer DMA direction according to EventStream type */
        int32_t dma_direction;
        switch (esType)
        {
            case LIBRORC_ES_IN_GENERIC:
            case LIBRORC_ES_IN_DDL:
            case LIBRORC_ES_IN_HWPG:
            {
                dma_direction = LIBRORC_DMA_FROM_DEVICE;
            }
            break;

            case LIBRORC_ES_OUT_GENERIC:
            case LIBRORC_ES_OUT_SWPG:
            case LIBRORC_ES_OUT_FILE:
            {
                dma_direction = LIBRORC_DMA_TO_DEVICE;
            }
            break;
            default:
            { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }
        }

        try
        {
            if( !m_called_with_bar )
            {
                m_dev = new librorc::device(deviceId);
                #ifdef SIM
                    m_bar1 = new librorc::sim_bar(m_dev, 1);
                #else
                    m_bar1 = new librorc::rorc_bar(m_dev, 1);
                #endif
            }

            if( !m_dev->DMAChannelIsImplemented(channelId) )
            {
                printf("ERROR: Requsted channel %d is not implemented in "
                       "firmware - exiting\n", channelId);
                throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
            }


            m_eventBuffer
                = new librorc::buffer(m_dev, EBUFSIZE, (2*channelId), 1, dma_direction);
            m_reportBuffer
                = new librorc::buffer(m_dev, RBUFSIZE, (2*channelId+1), 1, LIBRORC_DMA_FROM_DEVICE);

            chooseDMAChannel(esType);

            m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
            m_done             = false;
            m_event_callback   = NULL;
            m_status_callback  = NULL;
        }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }

        checkFirmware(esType);
    }



    void
    event_stream::chooseDMAChannel(LibrorcEsType esType)
    {
        switch ( esType )
        {
            case LIBRORC_ES_IN_GENERIC:
            {
                m_channel =
                new librorc::dma_channel
                (
                     m_channelId,
                     MAX_PAYLOAD,
                     m_dev,
                     m_bar1,
                     m_eventBuffer,
                     m_reportBuffer
                 );
            }
            break;

            case LIBRORC_ES_IN_DDL:
            {
                m_channel =
                new librorc::dma_channel_ddl
                (
                    m_channelId,
                    MAX_PAYLOAD,
                    m_dev,
                    m_bar1,
                    m_eventBuffer,
                    m_reportBuffer
                );
            }
            break;

            case LIBRORC_ES_IN_HWPG:
            {
                m_channel =
                new librorc::dma_channel_pg
                (
                    m_channelId,
                    MAX_PAYLOAD,
                    m_dev,
                    m_bar1,
                    m_eventBuffer,
                    m_reportBuffer,
                    m_eventSize
                );
            }
            break;

            case LIBRORC_ES_OUT_SWPG:
            {
                m_channel =
                new librorc::dma_channel
                (
                     m_channelId,
                     128,
                     m_dev,
                     m_bar1,
                     m_eventBuffer,
                     m_reportBuffer
                 );

                 m_bar1->simSetPacketSize(32);
                 m_channel->enable();
            }
            break;

        default:
            throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED;
        }
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
            librorc::sysmon *sm = new librorc::sysmon(m_bar1);
            cout << "CRORC FPGA" << endl
                 << "Firmware Rev. : " << hex << setw(8) << sm->FwRevision()  << dec << endl
                 << "Firmware Date : " << hex << setw(8) << sm->FwBuildDate() << dec << endl;
            delete sm;
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
        if( reports[m_channel_status->index].calc_event_size!=0 )
        {
            // capture index of the first found reportbuffer entry
            uint64_t starting_index       = m_channel_status->index;
            uint64_t events_per_iteration = 0;
            uint64_t event_buffer_offset  = 0;
            uint64_t report_buffer_offset = 0;

            // handle all following entries
            while( reports[m_channel_status->index].calc_event_size!=0 )
            {
                // increment number of events processed in this interation
                events_processed++;

                      librorc_event_descriptor  report   = reports[m_channel_status->index];
                      uint64_t                  event_id = getEventIdFromCdh(dwordOffset(report));
                const uint32_t                 *event    = getRawEvent(report);

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

}
