/**
 * @file buffer_alloc_test.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-07-21
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
#include <librorc.h>

#define DEVICE_ID 0
#define BUFFER_ID 0
#define MIN_BUF_SIZE 0x1000 // 4k = 1 page
#define MAX_BUF_SIZE ((uint64_t)0x1<<20) // 1MB

using namespace std;

int main()
{

    librorc::device *dev = NULL;

    /** create new device instance **/
    try{ dev = new librorc::device(DEVICE_ID); }
    catch(...)
    {
        cout << "ERROR: failed to initialize device." << endl;
        exit(-1);
    }

    int32_t overmap = 1;
    uint64_t bufferSize = MIN_BUF_SIZE;

    //for( bufferSize=MIN_BUF_SIZE; bufferSize<MAX_BUF_SIZE; bufferSize=(bufferSize*2))
    //{
        cout << "Trying RequestedSize=0x" << hex << bufferSize << "..." << endl;

        librorc::buffer *buf = NULL;
        try{ buf = new librorc::buffer(dev, bufferSize, BUFFER_ID, overmap); }
        catch(int e)
        {
            cout << "buffer contructor failed with " << e << endl;
            delete dev;
            exit(-1);
        }

        cout << "\tPhysicalSize=0x" << hex << buf->getPhysicalSize()
            << " MappingSize=0x" << hex << buf->getMappingSize()
            << " Overmapped=" << hex << buf->isOvermapped()
            << endl;

        buf->deallocate();
        delete buf;
    //}

    return 0;
}



