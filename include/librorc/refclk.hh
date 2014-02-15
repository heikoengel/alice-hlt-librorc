/**
 * @file refclk.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-27
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

#include "include_ext.hh"
#include "defines.hh"

#ifndef LIBRORC_REFCLK_H
#define LIBRORC_REFCLK_H

#define LIBRORC_REFCLK_ERROR_CONSTRUCTOR_FAILED  1
#define LIBRORC_REFCLK_ERROR_INVALID_PARAMETER 2

#define LIBRORC_REFCLK_I2C_CHAIN 4
#define LIBRORC_REFCLK_I2C_SLAVE 0x55
#define LIBRORC_REFCLK_DEFAULT_FOUT 212.500


/** Register 135 Pin Mapping */
#define M_RECALL (1<<0)
#define M_FREEZE (1<<5)
#define M_NEWFREQ (1<<6)
/** Register 137 Pin Mapping */
#define FREEZE_DCO (1<<4)

typedef struct
{
    uint32_t hs_div;
    uint32_t n1;
    uint64_t rfreq_int;
    double rfreq_float;
    double fdco;
    double fxtal;
}refclkopts;


namespace LIBRARY_NAME
{
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
             * Release DCO
             **/
            void
            releaseDCO();
            
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
            getNewOpts
            (
                double new_freq,
                double fxtal
            );

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
             * write oscillator settings into device
             **/
            void 
            setOpts
            (
                refclkopts opts
            );

        private:
                sysmon *m_sysmon;

        protected:
                
            /**
             * convert 48bit hex value to float, see Si570 UG
             **/
            double
            hex2float
            (
                uint64_t value
            );

            /**
             * convert float to 48bit hex value, see Si570 UG
             **/
            uint64_t
            float2hex
            (
                double value
            );
            
            /**
             * convert encoding to logical value for HsDiv
             **/
            int32_t
            hsdiv_reg2val
            (
                uint32_t hs_reg
            );

            /**
             * convert logical value to encoding for HsDiv
             **/
            int32_t
            hsdiv_val2reg
            (
                uint32_t hs_val
            );
            
            /**
             * convert encoding to logical value for N1
             **/
            int32_t
            n1_reg2val
            (
                uint32_t n1_reg
            );

            /**
             * convert logical value to encoding for N1
             **/
            int32_t
            n1_val2reg
            (
                uint32_t n1_val
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
