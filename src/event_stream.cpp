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

using namespace std;


namespace librorc
{

    event_stream::event_stream
    (
        int32_t   deviceId,
        int32_t   channelId
    )
    {

    }

    event_stream::event_stream
    (
        int32_t   deviceId,
        int32_t   channelId,
        uint32_t  eventSize
    )
    {

    }

    event_stream::~event_stream()
    {
        delete m_eventBuffer;
        delete m_reportBuffer;
        delete m_bar1;
        delete m_dev;
    }

    void
    event_stream::generateDMAChannel
    (
        int32_t   deviceId,
        int32_t   channelId
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
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED; }

        /** create new DMA report buffer */
        try
        { m_reportBuffer = new librorc::buffer(m_dev, RBUFSIZE, (2*channelId+1), 1, LIBRORC_DMA_FROM_DEVICE); }
        catch(...)
        { throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED; }
    }

}