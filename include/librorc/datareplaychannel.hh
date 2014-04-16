/**
 * @file datareplaychannel.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-04-16
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
#ifndef LIBRORC_DDR3DATAREPLAY_H
#define LIBRORC_DDR3DATAREPLAY_H

#include <librorc/include_ext.hh>
#include <librorc/link.hh>
#include <librorc/defines.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME
{
    class link;

    /**
     * @brief: interface class to a DDR3 data replay channel
     **/
    class datareplaychannel
    {
        public:
            datareplaychannel( link *link );
            ~datareplaychannel();


            /**
             * check done state. done is asserted after a non-continuous
             * operation has been completed (either oneshot or until end of
             * channel)
             * @return true if done, else false
             **/
            bool
            isDone();

            /**
             * check if channel controller is waiting for data from DDR3. 
             * If isWaiting() returns true and no data
             * is received this may be a sign of invalid data in the RAM.
             * @return true if waiting, else false
             **/
            bool
            isWaiting();

            /**
             * check replay mode configuration
             * @return true if oneshot mode is enabled, else false
             **/
            bool
            isOneshotEnabled();

            /**
             * check replay mode configuration
             * @return true if continuous mode is enabled, else false
             **/
            bool
            isContinuousEnabled();

            /**
             * check enable state
             * @return true if enabled, false if disabled
             **/
            bool
            isEnabled();

            /**
             * check reset state
             * @return true if in reset, else false
             **/
            bool
            isInReset();

            /**
             * set replay channel enable
             * @param enable 1 to enable, 0 to disable
             **/
            void
            setEnable
            (
                uint32_t enable
            );

            /**
             * set replay channel reset
             * @param reset 1 to set reset, 0 to release reset
             **/
            void
            setReset
            (
                uint32_t reset
            );

            /**
             * set replay mode parameter for oneshot. If oneshot is enabled
             * only the next event is read from RAM upon enabling replay.
             * Oneshot overrides continuous mode. If oneshot is enabled,
             * the behavior is independent of continuous mode setting.
             * @param oneshot 1 to activate oneshot mode,
             * 0 to disable oneshot mode.
             **/
            void
            setModeOneshot
            (
                uint32_t oneshot
            );

            /**
             * set replay mode parameter for continuous. 
             * If continuous is enabled and oneshot is disabled,
             * all data in the channel is continuously read from RAM.
             * If continuous is disabled and oneshot is disabled only
             * the remaining data is read once.
             * Oneshot overrides continuous mode. If oneshot is enabled,
             * the behavior is independent of continuous mode setting.
             * @param continuous 1 to activate continuous mode,
             * 0 to disable continuous mode.
             **/
            void
            setModeContinuous
            (
                uint32_t continuous
            );


            /**
             * get next address to be read from DDR3 RAM
             * @return last DDR3 address
             **/
            uint32_t
            nextAddress();

            /**
             * get configured start address for current channel
             * @return DDR3 start address
             **/
            uint32_t
            startAddress();


            /**
             * set start address for current channel
             * @param start_address DDR3 start address
             **/
            void
            setStartAddress
            (
                uint32_t start_address
            );


        protected:
            link *m_link;
    };
}
#endif
