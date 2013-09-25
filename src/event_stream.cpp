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



namespace LIBRARY_NAME
{

    event_stream::event_stream
    (
        int32_t deviceId,
        int32_t channelId
    )
    {
        generateDMAChannel(deviceId, channelId, LIBRORC_ES_PURE);
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
        m_eventSize = eventSize;
        generateDMAChannel(deviceId, channelId, esType);
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
        delete m_bar1;
        delete m_dev;
    }



    void
    event_stream::generateDMAChannel
    (
        int32_t       deviceId,
        int32_t       channelId,
        LibrorcEsType esType
    )
    {
        m_deviceId  = deviceId;
        m_channelId = channelId;

        try
        {
            m_dev = new librorc::device(deviceId);
            #ifdef SIM
                m_bar1 = new librorc::sim_bar(m_dev, 1);
            #else
                m_bar1 = new librorc::rorc_bar(m_dev, 1);
            #endif
            m_eventBuffer
                = new librorc::buffer(m_dev, EBUFSIZE, (2*channelId), 1, LIBRORC_DMA_FROM_DEVICE);
            m_reportBuffer
                = new librorc::buffer(m_dev, RBUFSIZE, (2*channelId+1), 1, LIBRORC_DMA_FROM_DEVICE);

            chooseDMAChannel(esType);

            m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
        }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED; }

    }



    void
    event_stream::chooseDMAChannel(LibrorcEsType esType)
    {
        switch ( esType )
        {
            case LIBRORC_ES_PURE:
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

            case LIBRORC_ES_DDL:
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

            case LIBRORC_ES_PG:
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


}
