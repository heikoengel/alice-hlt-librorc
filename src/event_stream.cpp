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
        delete dev;
    }

    prepareChannel
    (
        int32_t   deviceId,
        int32_t   channelId
    )
    {
        /** Create new device instance */
        m_dev = NULL;
        try
        { dev = new librorc::device(deviceId); }
        catch(...)
        {
            throw LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED;
            //printf("ERROR: failed to initialize device.\n");
            //abort();
        }

        /** Bind to BAR1 */
        m_bar1 = NULL;
        try
        {
        #ifdef SIM
            bar1 = new librorc::sim_bar(m_dev, 1);
        #else
            bar1 = new librorc::rorc_bar(m_dev, 1);
        #endif
        }
        catch(...)
        {
            printf("ERROR: failed to initialize BAR1.\n");
            abort();
        }

    }

}
