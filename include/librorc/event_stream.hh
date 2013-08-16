/**
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

#ifndef LIBRORC_EVENT_STREAM_H
#define LIBRORC_EVENT_STREAM_H

#include "librorc/include_ext.hh"
#include "librorc/include_int.hh"


#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED 1

namespace librorc
{

    class event_stream
    {
        public:

             event_stream
             (
                int32_t   deviceId,
                int32_t   channelId
             );

             event_stream
             (
                int32_t   deviceId,
                int32_t   channelId,
                uint32_t  eventSize
             );

            ~event_stream();

            /** Member Variables */
            librorc::device *m_dev;
            librorc::bar    *m_bar1;
            librorc::buffer *m_ebuf;
            librorc::buffer *m_rbuf;


        protected:

            void
            generateDMAChannel
            (
                int32_t deviceId,
                int32_t channelId
            );

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
