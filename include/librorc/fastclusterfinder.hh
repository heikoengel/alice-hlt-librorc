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
             * enable FastClusterFinder
             **/
            void
            enable();

            /**
             * disable FastClusterFinder
             **/
            void
            disable();

            /**
             * get current state
             * @return true if enabled, false if disabled
             **/
            bool
            isEnabled();


            /**
             * enable/disable Single Pad Suppression. If enabled, clusters
             * with only one pad will be suppressed.
             * @param spSupprValue Single Pad Suppression value: 0,1
             **/
            void
            setSinglePadSuppression
            (
                int spSupprValue
            );

            /**
             * get current value for Single Pad Suppression
             * @return Single Pad Suppression value
             **/
            int
            singlePadSuppression();

            /**
             * enable or disable the Merger bypass. If bypass is enabled
             * the data is not merged but pushed out as cluster candidates
             * @param bypassValue bypass setting, 1 to bypass merger, 0 to
             * use merger (default)
             **/
            void
            setBypassMerger
            (
                int bypassValue
            );

            /**
             * get current Merger Bypass value
             * @return 1 if merger is bypassed, else 0
             **/
            int
            bypassMerger();


            /**
             * enable/disable deconvolution in pad direction
             * @param deconvValue 1 to enable deconvolution, 0 to disable
             * deconvolution
             **/
            void
            setDeconvPad
            (
                int deconvValue
            );

            /**
             * get current deconvolution setting
             * @return 1 if deconvolution is enabled, 0 if disabled.
             **/
            int
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
                uint8_t limit
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
                uint16_t limit
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
             * @param bFollow when set to true, the merger takes the 
             * timestamp of the current cluster candidate as a reference
             * for the distance check to the next. When set to false, the
             * merger uses the timestamp of the first candidate as a
             * reference for all following candidates.
             **/
            void
            setMergerAlgorithmFollow
            (
                bool bFollow
            );

            /**
             * get current Merger Algorithm Mode setting.
             * @return true when mode is set to 'follow', false if mode
             * is set to 'keep'
             **/
            bool
            mergerAlgorithmFollow();

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

        protected:
            link *m_link;
    };
}
#endif
