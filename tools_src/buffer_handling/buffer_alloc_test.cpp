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
 *
 **/

#include <librorc.h>

#define DEVICE_ID 0
#define MIN_BUF_SIZE 0x1000 // 4k = 1 page
#define MAX_BUF_SIZE ((uint64_t)0x1<<20) // 1MB

using namespace std;

int main(int argc, char *argv[])
{
    if( argc!=2 ) {
        cerr << "provide buffer ID as parameter" << endl;
        return -1;
    }
    uint64_t bufId = strtoul(argv[1], NULL, 0);

    librorc::device *dev = NULL;

    /** create new device instance **/
    try{ dev = new librorc::device(DEVICE_ID); }
    catch(int e)
    {
        cout << "ERROR: failed to initialize device. (" << e << ")" << endl;
        exit(-1);
    }

    int32_t overmap = 1;
    uint64_t bufferSize = MIN_BUF_SIZE;

    while(true)
    {
        librorc::buffer *buf = NULL;
        try{ buf = new librorc::buffer(dev, bufferSize, bufId, overmap); }
        catch(int e)
        {
            cout << "buffer alloc failed with " << e << endl;
            delete dev;
            exit(-1);
        }

        delete buf;

        try{ buf = new librorc::buffer(dev, bufId, overmap); }
        catch(int e)
        {
            cout << "buffer attach failed with " << e << endl;
            delete dev;
            exit(-1);
        }

        //buf->deallocate();
        delete buf;

        bufferSize = (bufferSize << 1);
        if( bufferSize > MAX_BUF_SIZE ) {
            bufferSize = MIN_BUF_SIZE;
        }
    }

    return 0;
}
