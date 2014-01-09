/**
 * @file ddr3_tester_status.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-09-17
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

#define HELP_TEXT "ddr3_tester_status usage: \n\
        ddr3_tester_status [parameters] \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -r [0/1]        Reset DDR3 \n\
        -h              Show this help text \n\
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
    int do_reset = 0;
    int32_t reset_val = 0;
    int arg;

    while ( (arg = getopt(argc, argv, "hn:r:")) != -1 )
    {
        switch (arg)
        {
            case 'h':
                cout << HELP_TEXT;
                return 0;
                break;
            
            case 'n':
                device_number = atoi(optarg);
                break;

            case 'r':
                reset_val = atoi(optarg);
                do_reset = 1;
                break;

            default:
                cout << HELP_TEXT;
                return -1;
        }
    }

    /** make sure all required parameters are provided and valid **/
    if (  device_number > 255 || device_number < 0 )
    {
        cout << "ERROR: no or invalid device ID selected: " 
             << device_number << endl;
        cout << HELP_TEXT;
        return -1;
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
    librorc::bar *bar = NULL;
    try
    {
    #ifdef SIM
        bar = new librorc::sim_bar(dev, 1);
    #else
        bar = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    /** Check if firmware is 'hwtest' */
    if ( bar->get32(RORC_REG_TYPE_CHANNELS)>>16 != RORC_CFG_PROJECT_hwtest )
    {
        cout << "Current firmware is no hwtest firmware! "
             << "Won't do anything..." << endl;
        delete bar;
        delete dev;
        return 0;
    }

    uint32_t ddrctrl = bar->get32(RORC_REG_DDR3_CTRL);

    if( do_reset )
    {
        /** clear reset bits */
        ddrctrl &= ~(1<<0 | 1<<16);
        /** set new values */
        ddrctrl |= (reset_val & 1);
        ddrctrl |= (reset_val & 1)<<16;
        /** write back new value */
        bar->set32(RORC_REG_DDR3_CTRL, ddrctrl);
    }
    else
    {
        uint32_t rorcval = bar->get32(RORC_REG_DDR3_C0_RDLO);
        uint64_t rdcnt0 = rorcval;

        rorcval = bar->get32(RORC_REG_DDR3_C0_WRLO);
        uint64_t wrcnt0 = rorcval;
        rorcval = bar->get32(RORC_REG_DDR3_C0_RDWRHI);
        rdcnt0 += ((uint64_t)(rorcval & 0xffff)<<32);
        wrcnt0 += (((uint64_t)(rorcval>>16) & 0xffff)<<32);

        rorcval = bar->get32(RORC_REG_DDR3_C0_RDLO);
        uint64_t rdcnt1 = rorcval;
        rorcval = bar->get32(RORC_REG_DDR3_C0_WRLO);
        uint64_t wrcnt1 = rorcval;
        rorcval = bar->get32(RORC_REG_DDR3_C0_RDWRHI);
        rdcnt1 += ((uint64_t)(rorcval & 0xffff)<<32);
        wrcnt1 += (((uint64_t)(rorcval>>16) & 0xffff)<<32);

        cout << "C0 Reset: " << (ddrctrl&0x01) << endl;
        cout << "C0 PhyInitDone: " << ((ddrctrl>>2)&1) << endl;
        cout << "C0 PLL Lock: " << ((ddrctrl>>3)&1) << endl;
        cout << "C0 Read Levelling Started: " << ((ddrctrl>>4)&3) << endl;
        cout << "C0 Read Levelling Done: " << ((ddrctrl>>6)&3) << endl;
        cout << "C0 Read Levelling Error: " << ((ddrctrl>>8)&3) << endl;
        cout << "C0 Write Levelling Started: " << ((ddrctrl>>10)&1) << endl;
        cout << "C0 Write Levelling Done: " << ((ddrctrl>>11)&1) << endl;
        cout << "C0 Write Levelling Error: " << ((ddrctrl>>12)&1) << endl;
        cout << "C0 Read Count: " << rdcnt0 << endl;
        cout << "C0 Write Count: " << wrcnt0 << endl;
        cout << "C0 TG Error: " << ((ddrctrl>>13)&1) << endl;
        cout << endl;

        cout << "C1 Reset: " << ((ddrctrl>>16)&0x01) << endl;
        cout << "C1 PhyInitDone: " << ((ddrctrl>>18)&1) << endl;
        cout << "C1 PLL Lock: " << ((ddrctrl>>19)&1) << endl;
        cout << "C1 Read Levelling Started: " << ((ddrctrl>>20)&3) << endl;
        cout << "C1 Read Levelling Done: " << ((ddrctrl>>22)&3) << endl;
        cout << "C1 Read Levelling Error: " << ((ddrctrl>>24)&3) << endl;
        cout << "C1 Write Levelling Started: " << ((ddrctrl>>26)&1) << endl;
        cout << "C1 Write Levelling Done: " << ((ddrctrl>>27)&1) << endl;
        cout << "C1 Write Levelling Error: " << ((ddrctrl>>28)&1) << endl;
        cout << "C1 Read Count: " << rdcnt1 << endl;
        cout << "C1 Write Count: " << wrcnt1 << endl;
        cout << "C1 TG Error: " << ((ddrctrl>>29)&1) << endl;
    }

    delete bar;
    delete dev;

    return 0;
}
