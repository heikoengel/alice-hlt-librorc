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

#ifndef CLASS_CRORC_HPP
#define CLASS_CRORC_HPP

#include <librorc.h>
#define LIBRORC_MAX_LINKS 12

enum tFanState { FAN_ON = 1, FAN_OFF = 0, FAN_AUTO = 2 };
enum tLedState { LED_AUTO = 0, LED_BLINK = 1 };

class crorc {
public:
  crorc(uint32_t deviceId);
  ~crorc();
  const char *getFanState();
  void setFanState(tFanState state);
  const char *getLedState();
  void setLedState(tLedState state);
  uint32_t getLinkmask();
  void setLinkmask(uint32_t mask);
  void doBoardReset();

  librorc::device *m_dev;
  librorc::bar *m_bar;
  librorc::sysmon *m_sm;
  librorc::refclk *m_refclk;
  librorc::link *m_link[LIBRORC_MAX_LINKS];
  librorc::gtx *m_gtx[LIBRORC_MAX_LINKS];
  uint32_t m_nchannels;
};


#endif
