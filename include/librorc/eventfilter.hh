/**
 * @file eventfilter.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-18
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
 * */

#ifndef LIBRORC_EVENTFILTER_H
#define LIBRORC_EVENTFILTER_H

#include <librorc/defines.hh>
#include <librorc/link.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME
{
    class link;

    /**
     * @brief Interface class to the event filter available in
     * any RORC-to-host event stream.
     **/
    class eventfilter
    {
        public:
            eventfilter( link *link );
            ~eventfilter();


            /**
             * enable the filter. Note: the filter has to be enabled to
             * allow any data to go through, even if no events are dropped.
             **/
            void
            enable();


            /**
             * disable the filter. An event stream with a disabled filter
             * will not provide any data to the DMA engine and will not
             * signal busy/XOFF to the data source. Disable any unused raw
             * readout channels to avoid backpressure.
             **/
            void
            disable();


            /**
             * get current filter state
             * @return true when enabled, false when disabled
             **/
            bool
            isEnabled();


            /**
             * Set the 'Filter all' flag: If enabled, the event payload
             * is stripped for any incoming event.
             * The CDH for each incoming event is still transmitted to the
             * DMA engine.
             * @param filter_all set true to enable, false to disable
             **/
            void
            setFilterAll
            (
                bool filter_all
            );


            /**
             * get current setting for 'filter all'.
             * @return true when enabled, false when disabled.
             **/
            bool
            filterAll();
            

            /**
             * set a filter mask for events to be filtered.
             * @param mask The filter mask is logically AND'ed with
             * the event orbit ID from CDH. If the result is 0 the event
             * is fully read out. If the result is !=0 the event payload
             * is dropped and the CDH is modified to signal this change.
             * @param mask 24 bit filter mask applied on orbit ID. Set to
             * 0 to read out all events, set 1 to read out every second
             * event, 3 for every third, 7 for every fourth, etc...
             **/
            void
            setFilterMask
            (
                uint32_t mask
            );


            /**
             * get current filter mask. see setFilterMask for a description.
             * @return 24 bit filter mask.
             **/
            uint32_t
            filterMask();

        protected:
            link *m_link;
    };
}
#endif
