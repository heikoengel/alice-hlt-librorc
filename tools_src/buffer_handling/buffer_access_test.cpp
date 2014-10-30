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

#include <librorc.h>
using namespace std;

#define DEVICE_ID 0
#define BUFFER_ID 0
#define BUFFER_SIZE (1<<17)

int main()
{

    int overmap = 1;
    librorc::device *dev = NULL;
    librorc::buffer *buf = NULL;
    int i;

    /** create new device instance **/
    try{ dev = new librorc::device(DEVICE_ID); }
    catch(...)
    {
        cout << "ERROR: failed to initialize device." << endl;
        exit(-1);
    }

    for( i=0; i<10; i++ )
    {
        try{ buf = new librorc::buffer(dev, BUFFER_SIZE*(i+1), i, overmap); }
        catch(int e)
        {
            cout << "1: buffer contructor " << i  << " failed with " << e << endl;
            delete dev;
            exit(-1);
        }
        delete buf;
    }

    for( i=0; i<10; i++ )
    {
        try{ buf = new librorc::buffer(dev, i, overmap); }
        catch(int e)
        {
            cout << "2: buffer contructor " << i  << " failed with " << e << endl;
            delete dev;
            exit(-1);
        }

        uint32_t *bh = (uint32_t *)buf->getMem();
        uint64_t sum = 0;

        for( uint64_t j = 0; j < (buf->size()/4); j++ )
        {
            sum += bh[j];
        }
        printf("%lx\n", sum);
        delete buf;
    }

    delete dev;
    return 0;
}


