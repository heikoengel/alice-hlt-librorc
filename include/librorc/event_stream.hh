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

#include <librorc/include_ext.hh>

#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED   1
#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BAR_FAILED      2
#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BUFFER_FAILED   3
#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DCHANNEL_FAILED 4

/** Buffer Sizes (in Bytes) **/
#ifndef SIM
    #define EBUFSIZE (((uint64_t)1) << 28)
    #define RBUFSIZE (((uint64_t)1) << 26)
    #define STAT_INTERVAL 1.0
#else
    #define EBUFSIZE (((uint64_t)1) << 19)
    #define RBUFSIZE (((uint64_t)1) << 17)
    #define STAT_INTERVAL 0.00001
#endif

namespace librorc
{

class dma_channel;
class bar;
class buffer;
class device;

    class event_stream
    {
        public:

//             event_stream
//             (
//                int32_t   deviceId,
//                int32_t   channelId
//             );

//             event_stream
//             (
//                int32_t   deviceId,
//                int32_t   channelId,
//                uint32_t  eventSize
//             );

            ~event_stream(){};


            /** Member Variables */
            device      *m_dev;
            bar         *m_bar1;
            buffer      *m_eventBuffer;
            buffer      *m_reportBuffer;
            dma_channel *m_channel;


        protected:

            void
            generateDMAChannel
            (
                int32_t deviceId,
                int32_t channelId
            );

            void deleteParts();

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
