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

#include <librorc/dma_channel.hh>
#include <librorc/dma_channel_ddl.hh>


using namespace std;


namespace librorc
{

    event_stream::event_stream
    (
        int32_t deviceId,
        int32_t channelId
    )
    {
        generateDMAChannel(deviceId, channelId, LIBRORC_ES_PURE);
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
    }

    event_stream::~event_stream()
    {
        deleteParts();
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
        /** Create new device instance */
        try
        { m_dev = new librorc::device(deviceId); }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED; }

        /** Bind to BAR1 */
        try
        {
        #ifdef SIM
            m_bar1 = new librorc::sim_bar(m_dev, 1);
        #else
            m_bar1 = new librorc::rorc_bar(m_dev, 1);
        #endif
        }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BAR_FAILED; }

        /** Create new DMA event buffer */
        try
        { m_eventBuffer = new librorc::buffer(m_dev, EBUFSIZE, (2*channelId), 1, LIBRORC_DMA_FROM_DEVICE); }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BUFFER_FAILED; }

        /** create new DMA report buffer */
        try
        { m_reportBuffer = new librorc::buffer(m_dev, RBUFSIZE, (2*channelId+1), 1, LIBRORC_DMA_FROM_DEVICE); }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BUFFER_FAILED; }

        try
        {
            m_channel =
            new librorc::dma_channel
            (channelId, MAX_PAYLOAD, m_dev, m_bar1, m_eventBuffer, m_reportBuffer);
        }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DCHANNEL_FAILED; }
    }

    void
    event_stream::setupPGChannel()
    {
        m_channel->enable();
        //cout << "Waiting for GTX to be ready..." << endl;
        m_channel->waitForGTXDomain();
        //cout << "Configuring pattern generator ..." << endl;
        m_channel->configurePatternGenerator(m_eventSize);
    }

}
