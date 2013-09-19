/**
 * @file bufferstats.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-12
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

#include <pda.h>
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
    try{ buf = new librorc::buffer(dev, BufferId); }
    catch(...)
    {
        perror("ERROR: buf->connect");
        delete dev;
        exit(-1);
    }

    cout << "Device " << DeviceId << ", Buffer " << BufferId << endl;
    cout << "Number of SG Entries: " << buf->getnSGEntries() << endl;
    cout << "Overmapped:           " << buf->isOvermapped() << endl;
    cout << "Size:                 " << buf->getSize()
        << " Bytes (" << dec << (buf->getSize()/(1<<20)) << " MB)" << endl;

    if ( verbose )
    {
        cout << "SGList:" << endl;
        /** print scatter-gather list */
        vector<librorc_sg_entry> sglist = buf->sgList();
        for ( uint64_t i=0; i< sglist.size(); i++ )
        {
            uint32_t addr_high = sglist[i].pointer>>32;
            cout << "                 [" << i << "]: addr=0x" << hex;
            cout << setw(8) << setfill('0') << addr_high << "_";
            cout << (sglist[i].pointer & 0xffffffff)
                 << ", len=0x" << sglist[i].length <<endl;
        }
    }

    delete buf;
    delete dev;

    return 0;
}
