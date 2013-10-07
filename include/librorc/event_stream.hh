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
#include "defines.hh"

#define LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_FAILED   1


/**
 * TODO: which operating modes do we want to have here?
 * further options:
 * HLT_IN:
 * - generate event data by SW: no DMA transfers involved
 *     - from software PG, e.g. LIBRORC_ES_IN_SWPG
 *     - from local file(s), e.g. LIBRORC_ES_IN_FILE
 * HLT_OUT:
 * - use external interface to get event data
 *
 * NOTE:
 * HLT-OUT HWPG does not need an instance of event_stream to run,
 * as no DMA data transfer is involved in this mode.
 * */
typedef enum
{
    /** HLT-IN */
    LIBRORC_ES_IN_GENERIC, /** no source specific intializations */
    LIBRORC_ES_IN_DDL, /** use DIU as data source */
    LIBRORC_ES_IN_HWPG, /** use hardware PG as data source */
    /** HLT-OUT */
    LIBRORC_ES_OUT_SWPG, /** use software based PG as data source */
    LIBRORC_ES_OUT_FILE /** use file as data source */
} LibrorcEsType;


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

namespace LIBRARY_NAME
{

class dma_channel;
class bar;
class buffer;
class device;

    class event_stream
    {
        public:

             event_stream
             (
                int32_t deviceId,
                int32_t channelId
             );

             event_stream
             (
                int32_t       deviceId,
                int32_t       channelId,
                uint32_t      eventSize,
                LibrorcEsType esType
             );

            ~event_stream();
            void printDeviceStatus();

            /** Member Variables */
            device      *m_dev;
            bar         *m_bar1;
            buffer      *m_eventBuffer;
            buffer      *m_reportBuffer;
            dma_channel *m_channel;


        protected:
            uint32_t  m_eventSize;
            int32_t   m_channelId;

            void
            generateDMAChannel
            (
                int32_t       deviceId,
                int32_t       channelId,
                LibrorcEsType esType
            );

            void chooseDMAChannel(LibrorcEsType esType);
            void deleteParts();

    };

}

#endif /** LIBRORC_EVENT_STREAM_H */
