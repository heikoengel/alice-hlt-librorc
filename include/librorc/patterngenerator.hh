/**
 * @file patterngenerator.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-13
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

#ifndef LIBRORC_PATTERNGENERATOR_H
#define LIBRORC_PATTERNGENERATOR_H

#include <librorc/include_ext.hh>
#include <librorc/link.hh>
#include <librorc/registers.h>
#include <librorc/defines.hh>


/**
 * TODO: this is a copy of include/librorc/event_sanity_checker.hh:51
 **/
#define PG_PATTERN_INC    0 /** Increment value by 1 */
#define PG_PATTERN_DEC    2 /** Decrement value by 1 */
#define PG_PATTERN_SHIFT  1 /** Shifts the value to the left, the leftmost bit is inserted on the right side */
#define PG_PATTERN_TOGGLE 3 /** Toggles between the value and the negated value : 0x000000A5 -> 0xffffff5A */

#define LIBRORC_PG_TIMEOUT 10000


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
