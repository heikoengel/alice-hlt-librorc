/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
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
 *     * Neither the name of University Frankfurt, CERN nor the
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
 **/
#include "class_crorc.hpp"

crorc::crorc(uint32_t deviceId) {
  m_dev = NULL;
  m_bar = NULL;
  m_sm = NULL;
  m_refclk = NULL;
  for (uint32_t i = 0; i < LIBRORC_MAX_LINKS; i++) {
    m_link[i] = NULL;
    m_gtx[i] = NULL;
    m_diu[i] = NULL;
    m_siu[i] = NULL;
    m_ddl[i] = NULL;
  }
  try {
    m_dev = new librorc::device(deviceId);
  } catch (...) {
    throw;
  }

  try {
#ifdef MODELSIM
    m_bar = new librorc::sim_bar(m_dev, 1);
#else
    m_bar = new librorc::rorc_bar(m_dev, 1);
#endif
  } catch (...) {
    throw;
  }

  try {
    m_sm = new librorc::sysmon(m_bar);
  } catch (...) {
    throw;
  }

  try {
    m_refclk = new librorc::refclk(m_sm);
  } catch (...) {
    throw;
  }

  m_nchannels = m_sm->numberOfChannels();

  for (uint32_t i = 0; i < m_nchannels; i++) {
    try {
      m_link[i] = new librorc::link(m_bar, i);
      m_gtx[i] = new librorc::gtx(m_link[i]);
      m_ddl[i] = new librorc::ddl(m_link[i]);

      switch (m_link[i]->linkType()) {
      case RORC_CFG_LINK_TYPE_DIU:
        m_diu[i] = new librorc::diu(m_link[i]);
        break;
      case RORC_CFG_LINK_TYPE_SIU:
        m_siu[i] = new librorc::siu(m_link[i]);
        break;
      default:
        break;
      }
    } catch (...) {
      throw;
    }
  }
}

crorc::~crorc() {
  for (uint32_t i = 0; i < LIBRORC_MAX_LINKS; i++) {
    if (m_ddl[i]) {
      delete m_ddl[i];
    }
    if (m_siu[i]) {
      delete m_siu[i];
    }
    if (m_diu[i]) {
      delete m_diu[i];
    }
    if (m_gtx[i]) {
      delete m_gtx[i];
    }
    if (m_link[i]) {
      delete m_link[i];
    }
  }
  if (m_refclk) {
    delete m_refclk;
  }
  if (m_sm) {
    delete m_sm;
  }
  if (m_bar) {
    delete m_bar;
  }
  if (m_dev) {
    delete m_dev;
  }
}

const char *crorc::getFanState() {
  if (m_sm->systemFanIsAutoMode()) {
    return "auto";
  } else if (m_sm->systemFanIsEnabled()) {
    return "on";
  } else {
    return "off";
  }
}

void crorc::setFanState(uint32_t state) {
  if (state == FAN_ON) {
    m_sm->systemFanSetEnable(1, 1);
  } else if (state == FAN_OFF) {
    m_sm->systemFanSetEnable(1, 0);
  } else {
    m_sm->systemFanSetEnable(0, 1);
  }
}

const char *crorc::getLedState() {
  if (m_sm->bracketLedInBlinkMode()) {
    return "blink";
  } else {
    return "auto";
  }
}

void crorc::setLedState(uint32_t state) {
  uint32_t mode = (state == LED_BLINK) ? 1 : 0;
  m_sm->setBracketLedMode(mode);
}

uint32_t crorc::getLinkmask() { return m_sm->getLinkmask(); }

void crorc::setLinkmask(uint32_t mask) { m_sm->setLinkmask(mask); }

void crorc::doBoardReset(){};

void crorc::setAllQsfpReset(uint32_t reset) {
  m_sm->qsfpSetReset(0, reset);
  m_sm->qsfpSetReset(1, reset);
  m_sm->qsfpSetReset(2, reset);
}

void crorc::setAllGtxReset(uint32_t reset) {
  for (uint32_t i = 0; i < m_nchannels; i++) {
    m_gtx[i]->setReset(reset);
  }
}

void crorc::configAllGtxPlls(librorc::gtxpll_settings pllcfg) {
  for (uint32_t i = 0; i < m_nchannels; i++) {
    m_gtx[i]->drpSetPllConfig(pllcfg);
  }
}
