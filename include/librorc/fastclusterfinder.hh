/**
 * @file fastclusterfinder.hh
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

#ifndef LIBRORC_FASTCLUSTERFINDER_H
#define LIBRORC_FASTCLUSTERFINDER_H

#include <librorc/defines.hh>
#include <librorc/link.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME
{
    class link;

    /**
     * @brief Interface class to the FastClusterFinder component
     **/
    class fastclusterfinder
    {
        public:
            fastclusterfinder( link *link );
            ~fastclusterfinder();

            /**
             * set clusterfinder state
             * When 'reset' is asserted, the clusterfinder will not process
             * any data: input is ignored, thus no output. This overrides
             * ant 'enable' setting.
             * When 'reset' is deasserted, 'enable' controls the behavior:
             * enable==1: activates the processing,
             *            raw input -> processed output
             * enable==0: push any data through without processing it,
             *            output==input
             * @param reset set clusterfinder reset to 1 or 0
             * @param enable set clusterfinder enable to 1 or 0
             **/
            void
            setState
            (
                uint32_t reset,
                uint32_t enable
            );

            /**
             * get current enable state
             * @return true if enabled, false if disabled
             **/
            bool
            isEnabled();

            /**
             * get current reset state
             * @return true if in reset, otherwise false
             **/
            bool
            isInReset();

            /**
             * enable/disable Single Pad Suppression. If enabled, clusters
             * with only one pad will be suppressed.
             * @param spSupprValue Single Pad Suppression value: 0,1
             **/
            void
            setSinglePadSuppression
            (
                uint32_t spSupprValue
            );

            /**
             * get current value for Single Pad Suppression
             * @return Single Pad Suppression value
             **/
            uint32_t
            singlePadSuppression();

            /**
             * enable or disable the Merger bypass. If bypass is enabled
             * the data is not merged but pushed out as cluster candidates.
             * WARNING: bypassing the merger is broken in the firmware!
             * @param bypassValue bypass setting, 1 to bypass merger, 0 to
             * use merger (default)
             **/
            void
            setBypassMerger
            (
                uint32_t bypassValue
            );

            /**
             * get current Merger Bypass value
             * @return 1 if merger is bypassed, else 0
             **/
            uint32_t
            bypassMerger();


            /**
             * enable/disable deconvolution in pad direction
             * @param deconvValue 1 to enable deconvolution, 0 to disable
             * deconvolution
             **/
            void
            setDeconvPad
            (
                uint32_t deconvValue
            );

            /**
             * get current deconvolution setting
             * @return 1 if deconvolution is enabled, 0 if disabled.
             **/
            uint32_t
            deconvPad();

            /**
             * set a lower limit on the sum of charges for a single
             * pad sequence. This cut is applied after gain correction
             * and is applied on the integer part of the sum.
             * @param limit lower limit on the integer part of the sum
             * of a sequence. Valid values are 0 to 255.
             **/
            void
            setSingleSeqLimit
            (
                uint8_t singe_seq_limit
            );

            /**
             * get current lower limit on the sum of charges for a single
             * sequence.
             * @return 8 bit integer, representing the integer part of the
             * sum
             **/
            uint8_t
            singleSeqLimit();


            /** set a lower limit on the sum of charges for a merged
             * cluster. This cut is applied after the merging and is
             * applied on the integer part of the total charge.
             * @param limit lower limit on the total charge. Valid values
             * are 0 to (2^15-1).
             **/
            void
            setClusterLowerLimit
            (
                uint16_t cluster_low_limit
            );

            /**
             * get current lower limit for the total charge of merged
             * clusters
             * @return 16 bit unsigned integer, represents the integer
             * part of the sum.
             **/
            uint16_t
            clusterLowerLimit();


            /**
             * set maximum timestamp distance of cluster candidates
             * that should be merger.
             * @param match_distance 4 bit integer representing the
             * maximum merge distance in multiples of time samples.
             **/
            void
            setMergerDistance
            (
               uint8_t match_distance
            );

            /**
             * get current merger distance
             * @return maximum distance of cluster candidates that should
             * be merged as 4 bin integer in multiples of time samples.
             **/
            uint8_t
            mergerDistance();


            /**
             * set Merger Algorithm Mode
             * @param mode when set to 1, the merger takes the
             * timestamp of the current cluster candidate as a reference
             * for the distance check to the next. When set to 0, the
             * merger uses the timestamp of the first candidate as a
             * reference for all following candidates.
             **/
            void
            setMergerAlgorithm
            (
                uint32_t mode
            );

            /**
             * get current Merger Algorithm Mode setting.
             * @return 1 when mode is set to 'follow', 0 if mode
             * is set to 'keep'
             **/
            uint32_t
            mergerAlgorithm();

            /**
             * set charge tolerance threshold for peak finder.
             * @param charge_tolerance 4 bit integer representing the
             * minimum difference in raw ADC sample values required to
             * be detected as positive or negative slope. The occurence
             * of a positive and a negative slope in the same sequence
             * make a peak.
             **/
            void
            setChargeTolerance
            (
                uint8_t charge_tolerance
            );


            /**
             * get current charge tolerance threshold setting.
             * @return 4 bit integer representing the
             * minimum difference in raw ADC sample values required to
             * be detected as positive or negative slope. The occurence
             * of a positive and a negative slope in the same sequence
             * make a peak.
             **/
            uint8_t
            chargeTolerance();

            /**
             * get FCF error bitmask. Each flag is set when the according
             * error occurs and is only cleared by calling clearErrors().
             * Error Mask Mapping
             * [7]: not used, always reads as 0
             * [6]: Divider Underflow Error
             * [5]: Divider Overflow Error
             * [4]: RCU Decoder FIFO Overflow
             * [3]: Divider FIFO Overflow
             * [2]: Merger FIFO B Overflow
             * [1]: Merger FIFO A Overflow
             * [0]: ChannelProcessor FIFO Overflow
             * @return error mask. 0 means no error
             **/
            uint8_t
            errorMask();

            /**
             * clear all FCF error flags
             **/
            void
            clearErrors();

            /**
             * write an entry to the mapping RAM
             * @param addr target RCU channel address
             * @param data:
             *        [28:16]: gain
             *        [15]   : channel active
             *        [14]   : first or last pad in row
             *        [13:8] : row
             *        [7:0]  : pad
             **/
            void
            writeMappingRamEntry
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * read an entry from the mapping RAM
             * @param addr target RCU channel address
             * @return data:
             *        [28:16]: gain
             *        [15]   : channel active
             *        [14]   : first or last pad in row
             *        [13:8] : row
             *        [7:0]  : pad
             **/
            uint32_t
            readMappingRamEntry
            (
                uint32_t addr
            );

            uint32_t
            hexstringToUint32
            (
                std::string line
            );

            void
            loadMappingRam
            (
                const char *fname
            );

        protected:
            link *m_link;
    };
}
#endif
