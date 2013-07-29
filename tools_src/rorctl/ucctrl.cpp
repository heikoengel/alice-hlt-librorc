/**
 * @file ucctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-26
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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "librorc.h"

using namespace std;


int
    main
(
 int argc,
 char *argv[]
 )
{
    int32_t device_number = 0;
    int arg;
    int do_write = 0;
    double new_freq = 0.0;

    /** Instantiate device **/
    librorc::device *dev = NULL;
    try{ 
        dev = new librorc::device(device_number);
    }
    catch(...)
    { 
        cout << "Failed to intialize device " << device_number
            << endl;
        return -1;
    }

    /** Instantiate a new bar */
#ifdef SIM
    librorc::bar *bar = new librorc::sim_bar(dev, 1);
#else
    librorc::bar *bar = new librorc::rorc_bar(dev, 1);
#endif

    if ( bar->init() == -1 )
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    cout << hex << setw(8) << setfill('0')
        << bar->get(RORC_REG_UC_CTRL) << endl;

    /**
     * TODO:
     * * reset uC
     * * read uC config status
     * * config uC
     * */

    delete bar;
    delete dev;

    return 0;
}
