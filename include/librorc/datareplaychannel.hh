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


#ifndef LIBRORC_DDR3DATAREPLAY_H
#define LIBRORC_DDR3DATAREPLAY_H

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

            /**
             * set event limit for continuous replay. A limit of 0 means
             * no limit.
             * @param event_limit number of events to be replayed
             **/
            void
            setEventLimit
            (
                uint32_t event_limit
            );

            /**
             * get the current event limit for continuous replay
             * @return number of events to be replayed
             **/
            uint32_t
            eventLimit();


        protected:
            link *m_link;
    };
}
#endif
