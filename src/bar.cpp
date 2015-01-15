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
 *     * Neither the name of University Frankfurt, CERN nor the
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
 **/

#include <librorc/device.hh>
#include <librorc/bar.hh>
#include <librorc/bar_impl_hw.hh>
#include <librorc/bar_impl_sim.hh>

namespace LIBRARY_NAME {

bar::bar(device *dev, int32_t n)
    : p(
#ifdef MODELSIM
          new bar_impl_sim(dev, n)
#else
          new bar_impl_hw(dev, n)
#endif
          ) {
}

bar::~bar() { delete p; }

void bar::memcopy(bar_address target, const void *source,
                  size_t num) {
  p->memcopy(target, source, num);
}

void bar::memcopy(void *target, bar_address source, size_t num) {
  p->memcopy(target, source, num);
}

uint32_t bar::get32(bar_address address) {
  return p->get32(address);
}

uint16_t bar::get16(bar_address address) {
  return p->get16(address);
}

void bar::set32(bar_address address, uint32_t data) {
  p->set32(address, data);
}

void bar::set16(bar_address address, uint16_t data) {
  p->set16(address, data);
}

int32_t bar::gettime(struct timeval *tv, struct timezone *tz) {
  return p->gettime(tv, tz);
}

size_t bar::size() { return p->size(); }

void bar::simSetPacketSize(uint32_t packet_size) {
  p->simSetPacketSize(packet_size);
}
}
