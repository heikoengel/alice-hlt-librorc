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

#include <pda.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <librorc.h>

using namespace std;

#define HELP_TEXT "bufferstats usage: \n\
        bufferstats [parameters] \n\
parameters: \n\
        -n      [0..255]   Device ID \n\
        -b      [BufferID] BufferID \n\
        -v      be verbose \n\
        --help  Show this text \n"

int main( int argc, char *argv[])
{

    librorc::device *dev = NULL;
    librorc::buffer *buf = NULL;

    int32_t DeviceId = -1;
    int64_t BufferId= -1;
    int32_t verbose = 0;

    int opt;
    while ( (opt = getopt(argc, argv, "n:b:vh")) != -1)
    {
        switch (opt)
        {
            case 'h':
                cout << HELP_TEXT;
                return 0;
                break;
            case 'n':
                DeviceId = strtol(optarg, NULL, 0);
                break;
            case 'b':
                BufferId = strtol(optarg, NULL, 0);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                cout << "Unknown parameter (" << opt << ")!" << endl;
                cout << HELP_TEXT;
                abort();
                break;

        } //switch
    } //while

    /** sanity checks on command line arguments **/
    if ( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "DeviceId invalid or not set: " << DeviceId << endl;
        cout << HELP_TEXT;
        exit(-1);
    }

    if ( BufferId < 0 )
    {
        cout << "BufferId invalid or not set: " << BufferId << endl;
        cout << HELP_TEXT;
        exit(-1);
    }

    /** create new device instance **/
    try{ dev = new librorc::device(DeviceId); }
    catch(...)
    {
        cout << "ERROR: failed to initialize device." << endl;
        exit(-1);
    }


    /** Connect to buffer **/
    try{ buf = new librorc::buffer(dev, BufferId, 1); }
    catch(...)
    {
        perror("ERROR: buf->connect");
        delete dev;
        exit(-1);
    }

    cout << "Device " << DeviceId << ", Buffer " << BufferId << endl;
    cout << "Number of SG Entries: " << buf->getnSGEntries() << endl;
    cout << "Overmapped:           " << buf->isOvermapped() << endl;
    cout << "Physical Size:        " << buf->getPhysicalSize()
        << " Bytes (" << dec << (buf->getPhysicalSize()/(1<<20)) << " MB)" << endl;

    if ( verbose )
    {
        cout << "SGList:" << endl;
        /** print scatter-gather list */
        vector<librorc::ScatterGatherEntry> sglist = buf->sgList();
        for ( uint64_t i=0; i< sglist.size(); i++ )
        {
            uint32_t addr_high = sglist[i].pointer>>32;
            uint32_t addr_low = (sglist[i].pointer & 0xffffffff);
            cout << "                 [" << dec << i << "]: addr=0x"
                << hex << setw(8) << setfill('0')
                << addr_high << "_"
                << hex << setw(8) << setfill('0')
                << addr_low
                << ", len=0x" << sglist[i].length <<endl;
        }
    }

    delete buf;
    delete dev;

    return 0;
}
