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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#ifndef LIBRORC_FASTCLUSTERFINDER_H
#define LIBRORC_FASTCLUSTERFINDER_H

#include <iostream>
#include <librorc/defines.hh>
//#include <librorc/link.hh>
#include <librorc/registers.h>

namespace LIBRARY_NAME {
class link;

/**
 * @brief Interface class to the FastClusterFinder component
 **/
class fastclusterfinder {
public:
  fastclusterfinder(link *link);
  ~fastclusterfinder();

  /**
   * set Clusterfinder reset
   * @param val reset
   **/
  void setReset(uint32_t val);

  /**
   * set Clusterfinder enable
   * @param val enable
   **/
  void setEnable(uint32_t val);

  /**
   * set Clusterfinder bypass
   * @param val if set to 1, all data is pushed through the
   * clusterfinder without being touched. Enabling this bypass
   * allows to read out the raw data from the DDL. Set to 0 to
   * enable cluster finding.
   **/
  void setBypass(uint32_t val);

  /**
   * set Branch override
   * @param override if set 1, the clusterfinder will treat all input
   * data as "branch A" data. If set 0, the clusterfind will take the
   * branch information from the highest RCU hardware address bit.
   **/
  void setBranchOverride(uint32_t ovrd);

  /**
   * get Branch override
   * @return current override setting
   **/
  uint32_t branchOverride();

  /**
   * set split cluster tagging
   * @param 1 to enable, 0 to disable split cluster tagging
   **/
  void setSplitClusterTagging(uint32_t tag);

  /**
   * get current split cluster tagging setting
   * @return 1 if enabled, 0 if disabled
   **/
  uint32_t splitClusterTagging();

  /**
  * legacy wrapper around setReset and setEnable
  * @param reset set clusterfinder reset to 1 or 0
  * @param enable set clusterfinder enable to 1 or 0
  **/
  void setState(uint32_t reset, uint32_t enable);

  /**
   * get current enable state
   * @return true if enabled, false if disabled
   **/
  bool isEnabled();

  /**
   * get current reset state
   * @return true if in reset, otherwise false
   **/
  bool isInReset();

  /**
   * get current bypass setting
   * @return true if bypass is active, otherwise false
   **/
  bool isBypassed();

  /**
   * enable/disable Single Pad Suppression. If enabled, clusters
   * with only one pad will be suppressed.
   * @param spSupprValue Single Pad Suppression value: 0,1
   **/
  void setSinglePadSuppression(uint32_t spSupprValue);

  /**
   * get current value for Single Pad Suppression
   * @return Single Pad Suppression value
   **/
  uint32_t singlePadSuppression();

  /**
   * enable or disable the Merger bypass. If bypass is enabled
   * the data is not merged but pushed out as cluster candidates.
   * WARNING: bypassing the merger is broken in the firmware!
   * @param bypassValue bypass setting, 1 to bypass merger, 0 to
   * use merger (default)
   **/
  void setBypassMerger(uint32_t bypassValue);

  /**
   * get current Merger Bypass value
   * @return 1 if merger is bypassed, else 0
   **/
  uint32_t bypassMerger();

  /**
   * enable/disable deconvolution in pad direction
   * @param deconvValue 1 to enable deconvolution, 0 to disable
   * deconvolution
   **/
  void setDeconvPad(uint32_t deconvValue);

  /**
   * get current deconvolution setting
   * @return 1 if deconvolution is enabled, 0 if disabled.
   **/
  uint32_t deconvPad();

  /**
   * set a lower limit on the sum of charges for a single
   * pad sequence. This cut is applied after gain correction
   * and is applied on the integer part of the sum.
   * @param limit lower limit on the integer part of the sum
   * of a sequence. Valid values are 0 to 255.
   **/
  void setSingleSeqLimit(uint8_t singe_seq_limit);

  /**
   * get current lower limit on the sum of charges for a single
   * sequence.
   * @return 8 bit integer, representing the integer part of the
   * sum
   **/
  uint8_t singleSeqLimit();

  /** set a lower limit on the sum of charges for a merged
   * cluster. This cut is applied after the merging and is
   * applied on the integer part of the total charge.
   * @param limit lower limit on the total charge. Valid values
   * are 0 to (2^15-1).
   **/
  void setClusterLowerLimit(uint16_t cluster_low_limit);

  /**
   * get current lower limit for the total charge of merged
   * clusters
   * @return 16 bit unsigned integer, represents the integer
   * part of the sum.
   **/
  uint16_t clusterLowerLimit();

  /**
   * set maximum timestamp distance of cluster candidates
   * that should be merger.
   * @param match_distance 4 bit integer representing the
   * maximum merge distance in multiples of time samples.
   **/
  void setMergerDistance(uint8_t match_distance);

  /**
   * get current merger distance
   * @return maximum distance of cluster candidates that should
   * be merged as 4 bin integer in multiples of time samples.
   **/
  uint8_t mergerDistance();

  /**
   * set Merger Algorithm Mode
   * @param mode when set to 1, the merger takes the
   * timestamp of the current cluster candidate as a reference
   * for the distance check to the next. When set to 0, the
   * merger uses the timestamp of the first candidate as a
   * reference for all following candidates.
   **/
  void setMergerAlgorithm(uint32_t mode);

  /**
   * get current Merger Algorithm Mode setting.
   * @return 1 when mode is set to 'follow', 0 if mode
   * is set to 'keep'
   **/
  uint32_t mergerAlgorithm();

  /**
   * set charge tolerance threshold for peak finder.
   * @param charge_tolerance 4 bit integer representing the
   * minimum difference in raw ADC sample values required to
   * be detected as positive or negative slope. The occurence
   * of a positive and a negative slope in the same sequence
   * make a peak.
   **/
  void setChargeTolerance(uint8_t charge_tolerance);

  /**
   * get current charge tolerance threshold setting.
   * @return 4 bit integer representing the
   * minimum difference in raw ADC sample values required to
   * be detected as positive or negative slope. The occurence
   * of a positive and a negative slope in the same sequence
   * make a peak.
   **/
  uint8_t chargeTolerance();

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
  uint8_t errorMask();

  /**
   * clear all FCF error flags
   **/
  void clearErrors();

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
  void writeMappingRamEntry(uint32_t addr, uint32_t data);

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
  uint32_t readMappingRamEntry(uint32_t addr);

  /**
   * set noise suppression value
   * @param noise_suppression noise suppression value, valid range from 0-15
   **/
  void setNoiseSuppression(uint8_t noise_suppresion);

  /**
   * get noise suppression value
   * @return noise suppression value
   **/
  uint8_t noiseSuppression();

  /**
   * set noise suppression minimum value
   * @param noise_suppression noise suppression minimum value, valid range from
   *        0-15
   **/
  void setNoiseSuppressionMinimum(uint8_t noise_suppresion);

  /**
   * get noise suppression minimum value
   * @return noise suppression minimum value
   **/
  uint8_t noiseSuppressionMinimum();

  /**
   * set noise suppression neighbor value
   * @param noise_suppression noise suppression neighbor value, valid range from
   *        0-15
   **/
  void setNoiseSuppressionNeighbor(uint8_t noise_suppresion);

  /**
   * get noise suppression neighbor value
   * @return noise suppression neighbor value
   **/
  uint8_t noiseSuppressionNeighbor();

  /**
   * set lower limit for cluster Qmax
   * @param limit integer part of qmax lower limit cut
   **/
  void setClusterQmaxLowerLimit(uint16_t limit);

  /**
   * get lower limit for cluster Qmax
   * @return iteger part of qmax lower limit cut
   **/
  uint16_t clusterQmaxLowerLimit();

  /**
   * get edge cluster tagging status
   * @return 1 if enabled, 0 if disabled
   **/
  uint32_t tagEdgeClusters();

  /**
   * enable/disable edge cluster flagging
   * @param tag 1 to enable, 0 to disable
   **/
  void setTagEdgeClusters(uint32_t tag);

protected:
  void setCtrlBit(uint32_t pos, uint32_t val);
  uint32_t getCtrlBit(uint32_t pos);
  link *m_link;
};
}
#endif
