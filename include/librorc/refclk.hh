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

#include <librorc/defines.hh>

#ifndef LIBRORC_REFCLK_H
#define LIBRORC_REFCLK_H

namespace LIBRARY_NAME
{

#define LIBRORC_REFCLK_ERROR_CONSTRUCTOR_FAILED  1
#define LIBRORC_REFCLK_ERROR_INVALID_PARAMETER 2

#define LIBRORC_REFCLK_DEFAULT_FOUT 212.500

    typedef struct
    {
        uint32_t hs_div;
        uint32_t n1;
        uint64_t rfreq_int;
        double rfreq_float;
        double fdco;
        double fxtal;
    }refclkopts;

    class sysmon;

    /**
     * @brief GTX Reference Clock Generator Interface class
     *
     * See SiliconLabs Si570 Datasheet for technical details:
     * http://www.silabs.com/Support%20Documents/TechnicalDocs/si570.pdf
     **/
    class refclk
    {
        public:
            refclk(sysmon *parent_sysmon);
            ~refclk();

            /**
             * read back current oscillator settings from device
             * @param fout current output frequency.
             * @return struct refclkopts with all settings
             **/
            refclkopts
            getCurrentOpts
            (
                double fout
            );

            /**
             * calculate a new set of oscillator settings with a
             * new output frequency based on the device specific
             * XTAL frequency, which has been retrived via
             * getCurrentOpts()
             * @param new output frequency
             * @param device specific XTAL frequency
             * @return a new set of oscillator settings
             **/
            refclkopts
            calcNewOpts
            (
                double new_freq,
                double fxtal
            );

            /**
             * get current reference clock frequency from clock options struct
             * @param opts clock options struct
             * @return clock frequency
             **/
            float
            getFout
            (
                refclkopts opts
            );

            /**
             * reset the reference clock generator to the default frequency
             **/
            void
            reset();

            /**
             * write oscillator settings into device
             **/
            void
            writeOptsToDevice
            (
                refclkopts opts
            );

        private:
                sysmon *m_sysmon;

        protected:

            /**
             * Release DCO
             **/
            void
            releaseDCO();

            /**
             * setReser/Freeze/Memory Control Register
             * @param flag value to be written
             **/
            void
            setRFMCtrl
            (
                uint8_t flag
            );

            /**
             * set Freeze-DCO bit
             **/
            void
            setFreezeDCO();

            /**
             * wait for flag to be cleared in RFMC register
             * @param flag bitmask of requested bits. This call blocks
             * until (reg135 & flag)==0.
             **/
            void
            waitForClearance
            (
                uint8_t flag
            );

            /**
             * low-level read from RefClkGen
             * @param addr target address
             * @return value read
             **/
            uint8_t
            refclk_read
            (
                uint8_t addr
            );

            /**
             * low-level write to RefClkGen
             * @param addr target address
             * @param value value to be written
             **/
            void
            refclk_write
            (
                uint8_t addr,
                uint8_t value
            );
    };
}

#endif /** LIBRORC_REFCLK_H */
