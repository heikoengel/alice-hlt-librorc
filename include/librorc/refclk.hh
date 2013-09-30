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

    class refclk
    {
        public:
            refclk(sysmon *parent_sysmon);
            ~refclk();
            
            void
            releaseDCO();
            
            refclkopts
            getCurrentOpts
            (
                double fout
            );
            
            refclkopts
            getNewOpts
            (
                double new_freq,
                double fxtal
            );

            void
            setRFMCtrl
            (
                uint8_t flag
            );

            void
            setFreezeDCO();

            void
            waitForClearance
            (
                uint8_t flag
            );

            void 
            setOpts
            (
                refclkopts opts
            );

        private:
                sysmon *m_sysmon;

        protected:
            double
            hex2float
            (
                uint64_t value
            );

            uint64_t
            float2hex
            (
                double value
            );
            
            int32_t
            hsdiv_reg2val
            (
                uint32_t hs_reg
            );

            int32_t
            hsdiv_val2reg
            (
                uint32_t hs_val
            );
            
            int32_t
            n1_reg2val
            (
                uint32_t n1_reg
            );

            int32_t
            n1_val2reg
            (
                uint32_t n1_val
            );

            uint8_t
            refclk_read
            (
                uint8_t addr
            );

            void 
            refclk_write
            (
                uint8_t addr,
                uint8_t value
            );
    };
}                

#endif /** LIBRORC_REFCLK_H */
