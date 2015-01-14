/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

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
             * Set the 'Filter all' flag: If enabled, the event payload
             * is stripped for any incoming data event. Software events
             * are not touched.
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
            getFilterAll();
            

            /**
             * set a filter mask for events to be filtered.
             * @param mask The lower 24 bit of this filter mask are
             * logically AND'ed with the event orbit ID from CDH.
             * If the result is 0 the event is fully read out.
             * If the result is !=0 the event payload is dropped and
             * the CDH is modified to signal this change.
             * Set to 0 to read out all events.
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
            getFilterMask();

        protected:
            link *m_link;
    };
}
#endif
