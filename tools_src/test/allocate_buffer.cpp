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
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  int arg;
  uint32_t device_id = 0;
  uint32_t buffer_id = 0;
  uint64_t buffer_size = 0;

  while ((arg = getopt(argc, argv, "n:b:s:")) != -1) {
    switch (arg) {
    case 'n':
      device_id = strtol(optarg, NULL, 0);
      break;

    case 'b':
      buffer_id = strtol(optarg, NULL, 0);
      break;

    case 's':
      buffer_size = strtoul(optarg, NULL, 0);
      break;

    case 'h':
      cout << "Parameter: -n [device_id] -b [buffer_id] -s [buffer_size]" << endl;
      break;

    default:
      break;
    }
  }

  if (buffer_size == 0) {
    cerr << "No or invalid buffer size" << endl;
    return -1;
  }

  librorc::device *dev;
  librorc::buffer *buf;

  try {
    dev = new librorc::device(device_id);
    buf = new librorc::buffer(dev, buffer_size, buffer_id, 0);
  } catch (int e) {
    cerr << "Failed with: (" << e << ") " << librorc::errMsg(e) << endl;
    return -1;
  }

  delete buf;
  delete dev;
  return 0;
}
