/**
 * Copyright (c) 2015, Heiko Engel <hengel@cern.ch>
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

#include <librorc.h>
#include <time.h>
#include <iostream>

using namespace std;


uint64_t timediff_ms(struct timespec start, struct timespec end) {
    uint64_t elapsed = (end.tv_sec - start.tv_sec) * 1000000000;
    elapsed += (end.tv_nsec - start.tv_nsec);
    return elapsed / 1000000;
}

int main(int argc, char *argv[]) {
  int arg;
  uint32_t device_id = 0;
  uint32_t buffer_id = 0;

  while ((arg = getopt(argc, argv, "n:b:")) != -1) {
    switch (arg) {
    case 'n':
      device_id = strtol(optarg, NULL, 0);
      break;

    case 'b':
      buffer_id = strtol(optarg, NULL, 0);
      break;

    case 'h':
      cout << "Parameter: -n [device_id] -b [buffer_id]" << endl;
      break;

    default:
      break;
    }
  }

  librorc::device *dev;
  librorc::buffer *buf;

  struct timespec tstart, tdev, tbuf, tacc;
  try {
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    dev = new librorc::device(device_id);
    clock_gettime(CLOCK_MONOTONIC, &tdev);
    buf = new librorc::buffer(dev, buffer_id, 1);
    clock_gettime(CLOCK_MONOTONIC, &tbuf);

    for (size_t i=0; i<buf->size()/4; i++) {
        uint32_t *mem = buf->getMem();
        mem[i] = mem[i]+1;
    }
    clock_gettime(CLOCK_MONOTONIC, &tacc);

  } catch (int e) {
    cerr << "Failed with: (" << e << ") " << librorc::errMsg(e) << endl;
    return -1;
  }
  cout << "device: " << timediff_ms(tstart, tdev)
       << " ms, buffer: " << timediff_ms(tdev, tbuf)
       << " ms, access: " << timediff_ms(tbuf, tacc)
       << " ms" << endl;

  delete buf;
  delete dev;
  return 0;
}
