/**
 * @file reportbufferdump.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-07-08
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * */
#include <cstdio>
#include <unistd.h>
#include <librorc.h>

using namespace std;

#define HELP_TEXT "reportbufferdump usage: \n\
        reportbufferdump [parameters] \n\
parameters: \n\
        -n      [0..255]   Device ID \n\
        -c      [0..11]    Channel ID\n\
        -v      be verbose \n\
        --help  Show this text \n"

int main( int argc, char *argv[])
{
    librorc::device *dev = NULL;
    librorc::buffer *buf = NULL;

    int32_t DeviceId = -1;
    int64_t ChannelId= -1;
    int32_t verbose = 0;

    int opt;
    while ( (opt = getopt(argc, argv, "n:c:vh")) != -1)
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
            case 'c':
                ChannelId = strtol(optarg, NULL, 0);
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

    /** create new device instance **/
    try{ dev = new librorc::device(DeviceId); }
    catch(...)
    {
        cout << "ERROR: failed to initialize device." << endl;
        exit(-1);
    }


    /** Connect to reportbuffer **/
    try{ buf = new librorc::buffer(dev, 2*ChannelId+1, 1); }
    catch(...)
    {
        perror("ERROR: buf->connect");
        delete dev;
        exit(-1);
    }

    cout << "Device " << DeviceId << ", Channel " << ChannelId << endl;
    cout << "Number of SG Entries: " << buf->getnSGEntries() << endl;
    cout << "Overmapped:           " << buf->isOvermapped() << endl;
    cout << "Physical Size:        " << buf->getPhysicalSize()
        << " Bytes (" << dec << (buf->getPhysicalSize()/(1<<20)) << " MB)" << endl;

    uint64_t rbsize = buf->getPhysicalSize();
    uint64_t nentries = rbsize / sizeof(librorc::EventDescriptor);
    cout << "Number of RB Entries: " << nentries << endl;

    librorc::EventDescriptor *descptr = (librorc::EventDescriptor*)buf->getMem();

    for( uint64_t i=0; i<nentries; i++ )
    {
        uint64_t offset = i*sizeof(librorc::EventDescriptor);
        uint64_t addr;
        uint64_t seg_len;
        if( !buf->offsetToPhysAddr(offset, &addr, &seg_len) )
        {
            cout << "Failed to resolv physical addr!" << endl;
        }
        if(verbose || descptr[i].reported_event_size>>30 )
        {
            cout << "Entry " << dec << i << hex
                << " RepEventSize 0x" << descptr[i].reported_event_size
                << " CalcEventSize 0x" << descptr[i].calc_event_size
                << " Physical Addr 0x" << addr << dec << endl;
        }
    }

    std::vector<librorc::ScatterGatherEntry> list;
    if( !buf->composeSglistFromBufferSegment(0, buf->getPhysicalSize(), &list) )
    {
        cout << "Failed to compose sglist" << endl;
    }

    std::vector<librorc::ScatterGatherEntry>::iterator iter;
    for(iter=list.begin(); iter!=list.end(); iter++)
    {
        cout << "list physaddr 0x" << hex << iter->pointer
             << " len 0x" << iter->length << endl;
    }

    cout << "SGlist length=" << list.size() << endl;


    delete buf;
    delete dev;

    return 0;
}



