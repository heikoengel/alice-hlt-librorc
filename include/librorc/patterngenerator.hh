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

#ifndef LIBRORC_PATTERNGENERATOR_H
#define LIBRORC_PATTERNGENERATOR_H

#include <librorc/defines.hh>


/**
 * TODO: this is a copy of include/librorc/event_sanity_checker.hh:51
 **/
#define PG_PATTERN_INC    0 /** Increment value by 1 */
#define PG_PATTERN_DEC    2 /** Decrement value by 1 */
#define PG_PATTERN_SHIFT  1 /** Shifts the value to the left, the leftmost bit is inserted on the right side */
#define PG_PATTERN_TOGGLE 3 /** Toggles between the value and the negated value : 0x000000A5 -> 0xffffff5A */

#define LIBRORC_PG_TIMEOUT 1000


namespace LIBRARY_NAME
{
    class link;

    /**
     * @brief Interface class for the programmable hardware
     * pattern generator.
     *
     * The hardware pattern generator can be used as data source
     * for HLT_IN to DMA and for HLT_OUT to DDL.
     **/
    class patterngenerator
    {
        public:
            patterngenerator
            (
                 link *link
            );

            ~patterngenerator();

            /**
             * enable PatternGenerator
             **/
            void
            enable();

            /**
             * disable PatternGenerator
             **/
            void
            disable();

            /**
             * reset the EventID counter in the pattern generator
             **/
            void
            resetEventId();

            /**
             * configure PatternGenerator to generate events with a
             * fixed size
             * @param eventSize event size in number of DWs including CDH
             **/
            void
            setStaticEventSize
            (
                uint32_t eventSize
            );

            /**
             * configure PatternGenerator to generate events with a
             * pseudo-random event size
             * @param prbs_min_size Minimum event size. The internal 
             * random event size is OR'ed with this value
             * @param prbs_max_size_mask Maximum event size mask. The
             * internal random event size is AND'ed with the upper 
             * 16 bit of this value.
             **/
            void
            setPrbsSize
            (
                uint16_t prbs_min_size,
                uint32_t prbs_max_size_mask
            );


            /**
             * check if PatternGenerator has finished
             * @return true when finished, false when active
             **/
            bool done();
            

            /**
             * configure PatternGenerator
             * @param patternMode can be PG_PATTERN_INC, PG_PATTERN_DEC,
             * PG_PATTERN_SHIFT, PG_PATTERN_TOGGLE
             * @param initialPattern starting value for patternMode
             * @param numberOfEvents number of events to be generated.
             * Set 0 to enable continuous event generation.
             **/
            void
            configureMode
            (
                uint32_t patternMode,
                uint32_t initialPattern,
                uint32_t numberOfEvents
            );


            /**
             * configure datapath to use PatternGenerator as data source.
             * This overwrites any previous call of useAsDataSource from
             * other components, e.g. diu.
             **/
            void
            useAsDataSource();

        protected:
            link *m_link;
    };
}
#endif
