/**
 * @file test_fmc.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-31
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

#define HELP_TEXT "test_fmc usage: \n\
        test_fmc -d [0...255] \n\
where -d specifies the target device. \n\
"

using namespace std;

int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int arg;

    while ( (arg = getopt(argc, argv, "hn:")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                {
                    cout << HELP_TEXT;
                    return 0;
                }
                break;
            case 'n':
                {
                    device_number = atoi(optarg);
                }
                break;default:
                {
                    cout << "Unknown parameter (" << arg << ")!" << endl;
                    cout << HELP_TEXT;
                    return -1;
                }
                break;
        } //switch
    } //while

    if ( device_number < 0 || device_number > 255 )
    {
        cout << "No or invalid device selected, using default device 0"
             << endl;
        device_number = 0;
    }

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

    /** Check if firmware is 'hwtest' */
    if ( bar->get(RORC_REG_TYPE_CHANNELS)>>16 != RORC_CFG_PROJECT_hwtest )
    {
        cout << "Current firmware is no hwtest firmware! "
             << "Won't do anything..." << endl;
        delete bar;
        delete dev;
        return 0;
    }

    /** set la_00_p as output and high */
    uint32_t ctrl[3];

    cout << "Driving LA_00_P high...";

    /** set LA_00_P output to '1' */
    ctrl[0] = bar->get(RORC_REG_FMC_CTRL_LOW);
    ctrl[0] |= 0x01; //drive la_00_p high
    bar->set(RORC_REG_FMC_CTRL_LOW, ctrl[0]);

    /** configure LA_00_P as output */
    ctrl[2] = bar->get(RORC_REG_FMC_CTRL_HIGH);
    ctrl[2] &= ~(1<<31); //clear tristate bit (=make output)
    bar->set(RORC_REG_FMC_CTRL_HIGH, ctrl[2]);

    /** read input */
    ctrl[0] = bar->get(RORC_REG_FMC_CTRL_LOW);
    ctrl[1] = bar->get(RORC_REG_FMC_CTRL_MID);
    ctrl[2] = bar->get(RORC_REG_FMC_CTRL_HIGH);

    if(ctrl[0] != 0xffffffff ||
            ctrl[1] != 0xffffffff ||
            (ctrl[2] & 0x0f) != 0x0f )
    {
        cout << "FAILED!" << endl;
        cout << "ERROR: expected 0x0000000f_ffffffff_ffffffff" << endl
             << "       received 0x" << hex << setw(8) << setfill('0')
             << (ctrl[2]&0x0f) << "_" << ctrl[1]
             << "_" << ctrl[0] << endl;
    }
    else
    {
        cout << "passed." << endl;
    }

    /** drive la_00_p low */
    cout << "Driving LA_00_P low...";

    /** set LA_00_P output to '0' */
    ctrl[0] = bar->get(RORC_REG_FMC_CTRL_LOW);
    ctrl[0] &= ~(0x01); //drive la_00_p low
    bar->set(RORC_REG_FMC_CTRL_LOW, ctrl[0]);

    /** read input */
    ctrl[0] = bar->get(RORC_REG_FMC_CTRL_LOW);
    ctrl[1] = bar->get(RORC_REG_FMC_CTRL_MID);
    ctrl[2] = bar->get(RORC_REG_FMC_CTRL_HIGH);

    if(ctrl[0] != 0 || ctrl[1] != 0 || (ctrl[2] & 0x0f) != 0 )
    {
        cout << "FAILED!" << endl;
        cout << "ERROR: expected 0x00000000_00000000_00000000" << endl
             << "       received 0x" << hex << setw(8) << setfill('0')
             << (ctrl[2]&0x0f) << "_" << ctrl[1]
             << "_" << ctrl[0] << endl;
    }
    else
    {
        cout << "passed." << endl;
    }


    /** configure LA_00_P as input again */
    ctrl[2] = bar->get(RORC_REG_FMC_CTRL_HIGH);
    ctrl[2] |= (1<<31); //set tristate bit (=make input)
    bar->set(RORC_REG_FMC_CTRL_HIGH, ctrl[2]);

    delete bar;
    delete dev;

    return 0;
}
