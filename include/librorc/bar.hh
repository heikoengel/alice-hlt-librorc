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
 **/
#ifndef LIBRORC_BAR_H
#define LIBRORC_BAR_H

#define LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED 1

namespace LIBRARY_NAME {
class device;
class bar_impl_hw;
class bar_impl_sim;

typedef uint64_t bar_address;

class bar {
public:
  bar(device *dev, int32_t n);
  ~bar();

  /**
   * copy buffer from host to device and vice versa
   * @param target address
   * @param source address
   * @param num number of bytes to be copied to destination
   * */
  void memcopy(bar_address target, const void *source, size_t num);

  void memcopy(void *target, bar_address source, size_t num);

  /**
   * read DWORD from BAR address
   * @param addr (unsigned int) aligned address within the
   *              BAR to read from.
   * @return data read from BAR[addr]
   **/
  uint32_t get32(bar_address address);

  /**
   * read WORD from BAR address
   * @param addr within the BAR to read from.
   * @return data read from BAR[addr]
   **/
  uint16_t get16(bar_address address);

  /**
   * write DWORD to BAR address
   * @param addr (unsigned int) aligned address within the
   *              BAR to write to
   * @param data (unsigned int) data word to be written.
   **/
  void set32(bar_address address, uint32_t data);

  /**
   * write WORD to BAR address
   * @param addr within the BAR to write to
   * @param data (unsigned int) data word to be written.
   **/
  void set16(bar_address address, uint16_t data);

  /**
   * get current time of day
   * @param tv pointer to struct timeval
   * @param tz pointer to struct timezone
   * @return return value from gettimeof day or zero for FLI simulation
   **/
  int32_t gettime(struct timeval *tv, struct timezone *tz);

  /**
   * get size of mapped BAR. This value is only valid after init()
   * @return size of mapped BAR in bytes
   **/
  size_t size();

  void simSetPacketSize(uint32_t packet_size);

protected:


#ifdef MODELSIM
  bar_impl_sim *p;
#else
  bar_impl_hw *p;
#endif
};
}
#endif /** LIBRORC_BAR_H */
