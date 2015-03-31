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

#include <iostream>
#include <librorc.h>

using namespace std;
#define MAX_ITERATIONS 30
bool overmap = true;

uint64_t timediff_ms(struct timespec start, struct timespec end) {
  uint64_t elapsed = (end.tv_sec - start.tv_sec) * 1000000000;
  elapsed += (end.tv_nsec - start.tv_nsec);
  return elapsed / 1000000;
}

int main(int argc, char *argv[]) {
  librorc::sysfs_handler *sh = NULL;
  try {
    sh = new librorc::sysfs_handler(0);
  } catch (int e) {
    cerr << "Failed to create sysfs_handler: " << librorc::errMsg(e) << endl;
  }
  
  std::vector<uint64_t> bufferlist = sh->list_all_buffers();
  uint64_t nbuffers = bufferlist.size();

  for(uint64_t iterations=0; iterations<MAX_ITERATIONS; iterations++) {
    for(uint64_t i=0; i<nbuffers; i++) {
      struct timespec start, end;
      clock_gettime(CLOCK_MONOTONIC, &start);
      uint64_t size = sh->get_buffer_size(bufferlist[i]);
      uint32_t *map = NULL;
      if( sh->mmap_buffer(&map, bufferlist[i], size, overmap) == -1) {
        cout << "mmap failed!" << endl;
      }
      clock_gettime(CLOCK_MONOTONIC, &end);
      sh->munmap_buffer(map, size, overmap);
      //uint64_t nsgs = sh->get_sglist_size(bufferlist[i]) / sizeof(struct librorc::scatter);
      cout << timediff_ms(start, end) << ", " << size << endl;
      //     << ", " << nsgs << endl;
    }
  }

  delete sh;
  return 0;
}
