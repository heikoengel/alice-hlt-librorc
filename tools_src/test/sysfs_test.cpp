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
#include <stdio.h>

#define MIN_BUF_SIZE (1ul<<12) //4k
#define MAX_BUF_SIZE (1ul<<30) //1G

using namespace std;

uint64_t timediff_ms(struct timespec start, struct timespec end) {
  uint64_t elapsed = (end.tv_sec - start.tv_sec) * 1000000000;
  elapsed += (end.tv_nsec - start.tv_nsec);
  return elapsed / 1000000;
}

int main(int argc, char *argv[]) {

  librorc::device *dev = NULL;
  librorc::buffer *buf = NULL;
  uint64_t size = MIN_BUF_SIZE;
  struct timespec start, end;
  //clock_t start, end;
  
  while (true) {
    //start = clock();
    clock_gettime(CLOCK_MONOTONIC, &start);
    try {
      dev = new librorc::device(0);
    } catch (int e) {
      cerr << "Failed to initialize device: (" << e << ") "
           << librorc::errMsg(e) << endl;
      return -1;
    }

    try {
      buf = new librorc::buffer(dev, size, 0, 1);
    } catch (int e) {
      cerr << "Failed to allocate buffer: (" << e << ") "
           << librorc::errMsg(e) << endl;
      delete dev;
      return -1;
    }
    //end = clock();
    clock_gettime(CLOCK_MONOTONIC, &end);
    //uint64_t tdiff1 = ((end - start) * 1000)/CLOCKS_PER_SEC;
    uint64_t tdiff1 = timediff_ms(start, end);
    delete buf;
    delete dev;


    //start = clock();
    clock_gettime(CLOCK_MONOTONIC, &start);
    try {
      dev = new librorc::device(0);
    } catch (int e) {
      cerr << "Failed to initialize device: (" << e << ") "
           << librorc::errMsg(e) << endl;
      return -1;
    }
    try {
      buf = new librorc::buffer(dev, 0, 1);
    } catch (int e) {
      cerr << "Failed to connect to buffer: (" << e << ") "
           << librorc::errMsg(e) << endl;
      delete buf;
      delete dev;
      return -1;
    }
    //end = clock();
    clock_gettime(CLOCK_MONOTONIC, &end);

    //uint64_t tdiff2 = ((end - start) * 1000)/CLOCKS_PER_SEC;
    uint64_t tdiff2 = timediff_ms(start, end);

    delete buf;
    delete dev;
    
    printf("%ld, %ld, %ld\n", size, tdiff1, tdiff2);

    size = (size<<1);
    if( size > MAX_BUF_SIZE) {
        size = MIN_BUF_SIZE;
    }
  }
  return 0;
}
