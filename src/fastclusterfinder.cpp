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

#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <cassert>
#include <librorc/fastclusterfinder.hh>
#include <librorc/link.hh>

namespace LIBRARY_NAME {
fastclusterfinder::fastclusterfinder(link *link) { m_link = link; }

fastclusterfinder::~fastclusterfinder() { m_link = NULL; }

void fastclusterfinder::setCtrlBit(uint32_t pos, uint32_t val) {
  assert(pos < 32);
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
  fcfctrl &= ~(1 << pos);
  fcfctrl |= ((val & 1) << pos);
  m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
}

uint32_t fastclusterfinder::getCtrlBit(uint32_t pos) {
  assert(pos < 32);
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
  return ((fcfctrl >> pos) & 1);
}

/****************************************************
 * FastClusterFinder Basic Controls
 ***************************************************/
void fastclusterfinder::setReset(uint32_t val) { setCtrlBit(15, val); }

void fastclusterfinder::setEnable(uint32_t val) { setCtrlBit(0, val); }

void fastclusterfinder::setBypass(uint32_t val) { setCtrlBit(13, val); }

void fastclusterfinder::setState(uint32_t reset, uint32_t enable) {
  setReset(reset);
  setEnable(enable);
}

bool fastclusterfinder::isEnabled() { return (getCtrlBit(0) == 1); }

bool fastclusterfinder::isInReset() { return (getCtrlBit(15) == 1); }

bool fastclusterfinder::isBypassed() { return (getCtrlBit(13) == 1); }

/****************************************************
 * FastClusterFinder Configuration/Status
 ***************************************************/

void fastclusterfinder::setSinglePadSuppression(uint32_t spSupprValue) {
  setCtrlBit(2, spSupprValue);
}

uint32_t fastclusterfinder::singlePadSuppression() { return getCtrlBit(2); }

void fastclusterfinder::setBypassMerger(uint32_t bypassValue) {
  setCtrlBit(3, bypassValue);
}

uint32_t fastclusterfinder::bypassMerger() { return getCtrlBit(3); }

void fastclusterfinder::setDeconvPad(uint32_t deconvValue) {
  setCtrlBit(4, deconvValue);
}

uint32_t fastclusterfinder::deconvPad() { return getCtrlBit(4); }

void fastclusterfinder::setMergerAlgorithm(uint32_t mode) {
  setCtrlBit(5, mode);
}

uint32_t fastclusterfinder::mergerAlgorithm() { return getCtrlBit(5); }

void fastclusterfinder::setBranchOverride(uint32_t ovrd) {
  setCtrlBit(14, ovrd);
}

uint32_t fastclusterfinder::branchOverride() { return getCtrlBit(14); }

uint32_t fastclusterfinder::splitClusterTagging() { return getCtrlBit(16); }

void fastclusterfinder::setSplitClusterTagging(uint32_t tag) {
  setCtrlBit(16, tag);
}

void fastclusterfinder::setSingleSeqLimit(uint8_t singe_seq_limit) {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  limits &= ~(0xff << 16);
  limits |= ((singe_seq_limit & 0xff) << 16);
  m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
}

uint8_t fastclusterfinder::singleSeqLimit() {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  return ((limits >> 16) & 0xff);
}

void fastclusterfinder::setClusterLowerLimit(uint16_t cluster_low_limit) {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  limits &= ~(0x0000ffff);
  limits |= (cluster_low_limit & 0xffff);
  m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
}

uint16_t fastclusterfinder::clusterLowerLimit() {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  return (limits & 0xffff);
}

void fastclusterfinder::setMergerDistance(uint8_t match_distance) {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  limits &= ~(0x0f << 24);
  limits |= ((match_distance & 0x0f) << 24);
  m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
}

uint8_t fastclusterfinder::mergerDistance() {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  return (limits >> 24 & 0x0f);
}

void fastclusterfinder::setChargeTolerance(uint8_t charge_tolerance) {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  limits &= ~(0x0f << 28);
  limits |= ((charge_tolerance & 0x0f) << 28);
  m_link->setDdlReg(RORC_REG_FCF_LIMITS, limits);
}

uint8_t fastclusterfinder::chargeTolerance() {
  uint32_t limits = m_link->ddlReg(RORC_REG_FCF_LIMITS);
  return (limits >> 28 & 0x0f);
}

uint8_t fastclusterfinder::errorMask() {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
  return ((fcfctrl >> 6) & 0x7f);
}

void fastclusterfinder::clearErrors() {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
  fcfctrl |= (1 << 1);
  m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
  fcfctrl &= ~(1 << 1);
  m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
}

void fastclusterfinder::setNoiseSuppression(uint8_t noise_suppresion) {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_LIMITS2);
  fcfctrl &= ~(0xf << 28);
  fcfctrl |= (noise_suppresion & 0xf) << 28;
  m_link->setDdlReg(RORC_REG_FCF_LIMITS2, fcfctrl);
}

uint8_t fastclusterfinder::noiseSuppression() {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_LIMITS2);
  return (fcfctrl >> 28) & 0xf;
}

void fastclusterfinder::setNoiseSuppressionMinimum(uint8_t noise_suppresion) {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_LIMITS2);
  fcfctrl &= ~(0xf << 24);
  fcfctrl |= (noise_suppresion & 0xf) << 24;
  m_link->setDdlReg(RORC_REG_FCF_LIMITS2, fcfctrl);
}

uint8_t fastclusterfinder::noiseSuppressionMinimum() {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_LIMITS2);
  return (fcfctrl >> 24) & 0xf;
}

void fastclusterfinder::setNoiseSuppressionNeighbor(uint8_t noise_suppresion) {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
  fcfctrl &= ~(0x3 << 17);
  fcfctrl |= (noise_suppresion & 0x3) << 17;
  m_link->setDdlReg(RORC_REG_FCF_CTRL, fcfctrl);
}

uint8_t fastclusterfinder::noiseSuppressionNeighbor() {
  uint32_t fcfctrl = m_link->ddlReg(RORC_REG_FCF_CTRL);
  return (fcfctrl >> 17) & 0x3;
}

void fastclusterfinder::setClusterQmaxLowerLimit(uint16_t limit) {
  uint32_t regval = m_link->ddlReg(RORC_REG_FCF_LIMITS2);
  regval &= ~(0x7ff);
  regval |= (limit & 0x7ff);
  m_link->setDdlReg(RORC_REG_FCF_LIMITS2, regval);
}

uint16_t fastclusterfinder::clusterQmaxLowerLimit() {
  return m_link->ddlReg(RORC_REG_FCF_LIMITS2) & 0x7ff;
}

uint32_t fastclusterfinder::tagEdgeClusters() { return getCtrlBit(19); }

void fastclusterfinder::setTagEdgeClusters(uint32_t tag) {
  setCtrlBit(19, tag);
}

/****************************************************
 * Mapping RAM access
 ***************************************************/

/** see header file for 'data' bit mapping */
void fastclusterfinder::writeMappingRamEntry(uint32_t addr, uint32_t data) {
  m_link->setDdlReg(RORC_REG_FCF_RAM_DATA, data);
  m_link->setDdlReg(RORC_REG_FCF_RAM_CTRL, (addr | (1 << 31)));
}

uint32_t fastclusterfinder::readMappingRamEntry(uint32_t addr) {
  m_link->setDdlReg(RORC_REG_FCF_RAM_CTRL, addr);
  return m_link->ddlReg(RORC_REG_FCF_RAM_DATA);
}
}
