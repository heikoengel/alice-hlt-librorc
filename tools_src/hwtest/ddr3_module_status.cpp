/**
 * @file ddr3_module_status.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-08-30
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

#define HELP_TEXT "ddr3_module_status usage: \n\
        ddr3_module_status -n [device] (-m [0/1]) \n\
paramters: \n\
        -n [0...255]    Target device ID \n\
        -m [0,1]        (optional) Module select\n\
"

/** use I2C Chain 3 (DDR3) */
#define I2C_CHAIN 3
#define I2C_SLV_SPD 0x50
#define I2C_SLV_TMP 0x18

using namespace std;
    

uint8_t
spd_read
(
    librorc::sysmon *sm,
    uint8_t module,
    uint8_t address
)
{
    uint8_t rdval = 0;
    try
    {
        rdval = sm->i2c_read_mem(I2C_CHAIN, I2C_SLV_SPD + module, address);
    }
    catch (...)
    {
        cout << "Failed to read from I2C" << endl;
        abort();
    }
    return rdval;
}


string*
spd_read_string
(
    librorc::sysmon *sm,
    uint8_t module,
    uint8_t start,
    uint8_t end
)
{
    string *readout = new string();
    char data_r = 0;
    for(uint8_t i=start; i<=end; i++)
    {
        data_r = spd_read(sm, module, i);
        readout->append(1, data_r);
    }
    return readout;
}




int
main
(
    int argc,
    char *argv[]
)
{
    int32_t device_number = -1;
    int32_t module_number = -1;

    int arg;

    while ( (arg = getopt(argc, argv, "hn:m:")) != -1 )
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
            case 'm':
                module_number = strtol(optarg, NULL, 0) & 0x1;
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

    if (  module_number > 1 || module_number < 0 )
    {
        cout << "No or invalid module selected, defaulting to module 0" 
             << endl;
        module_number = 0;
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
    #ifdef MODELSIM
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
   
    /** Instantiate a new sysmon */
    librorc::sysmon *sm;
    try
    { sm = new librorc::sysmon(bar); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar;
        delete dev;
        abort();
    }

    cout << "Part Number      : " 
         << *spd_read_string(sm, module_number, 128, 145) << endl;

    uint8_t spdrev = spd_read(sm, module_number, 0x01);
    cout << "SPD Revision     : " <<  hex << setw(1) << setfill('0')
         << (int)(spdrev>>4) << "." << (int)(spdrev&0x0f) << endl;

    cout << "DRAM Device Type : 0x" << hex << setw(2) << setfill('0')
         << (int)spd_read(sm, module_number, 0x02)
         << endl;

    cout << "Module Type      : 0x" << hex << setw(2) << setfill('0')
         << (int)spd_read(sm, module_number, 0x03)
         << endl;

    uint8_t density = spd_read(sm, module_number, 0x04);
    uint8_t ba_bits = (((density>>4)&0x7) + 3);
    uint32_t sd_cap = 256*(1<<(density&0xf));

    cout << "Bank Address     : " << dec << (int)ba_bits << " bit" << endl;
    cout << "SDRAM Capacity   : " << dec << (int)sd_cap 
         << " Mbit" << endl;

    uint8_t mod_org = spd_read(sm, module_number, 0x07);
    uint8_t n_ranks = ((mod_org>>3)&0x7) + 1;
    uint8_t dev_width = 4*(1<<(mod_org&0x07));

    cout << "Number of Ranks  : " << dec << (int)n_ranks << endl;
    cout << "Device Width     : " << dec << (int)dev_width << " bit" << endl;

    uint8_t mod_width = spd_read(sm, module_number, 0x08);
    uint8_t pb_width = 8*(1<<(mod_width & 0x7));

    cout << "Bus Width        : " << dec << (int)pb_width << " bit" << endl;

    uint32_t total_cap = sd_cap / 8 * pb_width / dev_width * n_ranks;
    cout << "Total Capacity   : " << dec << total_cap 
         << " MB" << endl;

    uint8_t mtb_dividend = spd_read(sm, module_number, 10);
    uint8_t mtb_divisor = spd_read(sm, module_number, 11);
    float timebase = (float)mtb_dividend / (float)mtb_divisor;
    cout << "Medium Timebase  : " << timebase << " ns" << endl;

    uint8_t tckmin = spd_read(sm, module_number, 12);
    cout << "tCKmin           : " << tckmin * timebase << " ns" << endl;

    uint8_t taamin = spd_read(sm, module_number, 16);
    cout << "tAAmin           : " << taamin * timebase << " ns" << endl;

    uint8_t twrmin = spd_read(sm, module_number, 17);
    cout << "tWRmin           : " << twrmin * timebase << " ns" << endl;

    uint8_t trcdmin = spd_read(sm, module_number, 18);
    cout << "tRCDmin          : " << trcdmin * timebase << " ns" << endl;

    uint8_t trrdmin = spd_read(sm, module_number, 19);
    cout << "tRRDmin          : " << trrdmin * timebase << " ns" << endl;

    uint8_t trpmin = spd_read(sm, module_number, 20);
    cout << "tRPmin           : " << trpmin * timebase << " ns" << endl;

    uint8_t trasrcupper = spd_read(sm, module_number, 21);

    uint16_t trasmin = ((trasrcupper & 0x0f)<<8) | 
        spd_read(sm, module_number, 22);
    cout << "tRASmin          : " << trasmin * timebase << " ns" << endl;

    uint16_t trcmin = ((trasrcupper & 0xf0)<<4) | 
        spd_read(sm, module_number, 23);
    cout << "tRCmin           : " << trcmin * timebase << " ns" << endl;

    uint16_t trfcmin = (spd_read(sm, module_number, 25)<<8) | 
        spd_read(sm, module_number, 24);
    cout << "tRFCmin          : " << trfcmin * timebase << " ns" << endl;

    uint8_t twtrmin = spd_read(sm, module_number, 26);
    cout << "tWTRmin          : " << twtrmin * timebase << " ns" << endl;

    uint8_t trtpmin = spd_read(sm, module_number, 27);
    cout << "tRTPmin          : " << trtpmin * timebase << " ns" << endl;

    uint16_t tfawmin = ((spd_read(sm, module_number, 28)<<8)&0x0f) | 
        spd_read(sm, module_number, 29);
    cout << "tFAWmin          : " << tfawmin * timebase << " ns" << endl;

    uint8_t tropts = spd_read(sm, module_number, 32);
    cout << "Thermal Sensor   : " << (int)((tropts>>7)&1) << endl;

    uint16_t cassupport = (spd_read(sm, module_number, 15)<<8) | spd_read(sm, module_number, 14);
    cout << "CAS Latencies    : ";
    for( int i=0; i<14; i++ )
    {
        if ( (cassupport>>i) & 1 )
        {
            cout << "CL" << (i+4) << " ";
        }
    }
    cout << endl;


    delete sm;
    delete bar;
    delete dev;

    return 0;
}
