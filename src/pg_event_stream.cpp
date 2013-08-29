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

#include <librorc/pg_event_stream.hh>

#include <librorc/device.hh>
#include <librorc/buffer.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>
#include <librorc/dma_channel.hh>

using namespace std;


namespace librorc
{

    pg_event_stream::pg_event_stream
    (
        int32_t   deviceId,
        int32_t   channelId,
        uint32_t  eventSize
    )
    {
        m_eventSize = eventSize;

        generateDMAChannel(deviceId, channelId);
        setupDMAChannel();
    }

    pg_event_stream::~pg_event_stream()
    {
        deleteParts();
    }

    void
    pg_event_stream::setupDMAChannel()
    {
        m_channel->enable();
        //cout << "Waiting for GTX to be ready..." << endl;
        m_channel->waitForGTXDomain();
        //cout << "Configuring pattern generator ..." << endl;
        m_channel->configurePatternGenerator(m_eventSize);
    }

}
