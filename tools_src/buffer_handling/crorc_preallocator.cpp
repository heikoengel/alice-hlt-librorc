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

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

using namespace std;

/** maximum channel number allowed **/
#define MAX_CHANNEL 11
#define EBUFSIZE (((uint64_t)1) << 28) // 256 MB

void
alloc_channel
(
    uint32_t         ChannelID,
    librorc::device *Dev,
    uint64_t         size
);

int main( int argc, char *argv[])
{
    uint64_t DefaultSize = EBUFSIZE;

    if(argc == 2)
    {
        // argv[1] is size in MB
        sscanf(argv[1], "%lu", &DefaultSize);
        DefaultSize <<= 20; //convert MB to byte
    }

    for(uint16_t DeviceId=0; DeviceId<UINT16_MAX; DeviceId++)
    {
        librorc::device *Dev;
        try{ Dev = new librorc::device(DeviceId); }
        catch(...){ return(0); }

        Dev->deleteAllBuffers();

        for( uint32_t ChannelId = 0; ChannelId<=MAX_CHANNEL; ChannelId++ )
        { alloc_channel(ChannelId, Dev, DefaultSize); }
    }

    return(0);
}



void
alloc_channel
(
    uint32_t         ChannelID,
    librorc::device *Dev,
    uint64_t         size
)
{
    librorc::event_stream *eventStream = NULL;
    try
    {
        eventStream =
            new librorc::event_stream
                (Dev->getDeviceId(), ChannelID, LIBRORC_ES_TO_HOST);
        eventStream->initializeDmaBuffers( 2*ChannelID, size);
    }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream. "
             << "Buffer allocation failed" << endl;
        return;
    }

    delete eventStream;
}
