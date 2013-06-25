/**
 * @file board_test.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-05-10
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

#include <iostream>
#include <iomanip>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>

#include "librorc.h"

using namespace std;

#ifndef RORC_REG_DDR3_CTRL
    #define RORC_REG_DDR3_CTRL 0
#endif

#define SLVADDR          0x50
#define LIBRORC_MAX_QSFP 3



void
qsfp_set_page0_and_config
(
    rorcfs_sysmon *sm,
    uint32_t index
);

string*
qsfp_i2c_string_readout
(
    rorcfs_sysmon *sm,
    uint8_t        start,
    uint8_t        end
);

bool
qsfpIsPresent
(
    librorc_bar   *bar1,
    uint32_t       index
);

bool
qsfpLEDIsOn
(
    librorc_bar   *bar1,
    uint32_t       qsfp_index,
    uint32_t       LED_index
);

string*
qsfpVendorName
(
    rorcfs_sysmon *sm,
    uint32_t       index
);

string*
qsfpPartNumber
(
    rorcfs_sysmon *sm,
    uint32_t index
);

float
qsfpTemperature
(
    struct rorcfs_sysmon *sm,
    uint32_t              index
);


uint32_t pcieNumberOfLanes(librorc_bar *bar1);
uint32_t pcieGeneration(librorc_bar *bar1);
bool     systemClockIsRunning(librorc_bar *bar1);
bool     systemFanIsEnabled(librorc_bar *bar1);
bool     systemFanIsRunning(librorc_bar *bar1);
double   systemFanSpeed(librorc_bar *bar1);


int main(int argc, char **argv)
{
    /** create new device object */
    rorcfs_device *dev = new rorcfs_device();
    if ( dev->init(0) == -1 )
    {
        printf("failed to initialize device 0 - "
        "is the board detected with lspci?\n");
        abort();
    }

    /** bind to BAR1 */
    librorc_bar *bar1 = NULL;
    #ifdef SIM
        bar1 = new sim_bar(dev, 1);
    #else
        bar1 = new rorc_bar(dev, 1);
    #endif

    if ( bar1->init() == -1 )
    {
        delete dev;
        abort();
    }

    /** instantiate a new sysmon */
    rorcfs_sysmon *sm;
    try
    { sm = new rorcfs_sysmon(bar1); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar1;
        delete dev;
        abort();
    }

    /** Printout revision and  date */
    try
    {
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setprecision(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setprecision(8) << sm->FwBuildDate() << dec << endl;
    }
    catch(...)
    {
        cout << "Reading Date returned wrong. This is likely a "
             << "PCIe access problem. Check lspci if the device is up"
             << "and the BARs are enabled" << endl;
        delete sm;
        delete bar1;
        delete dev;
        abort();
    }

    /** Print Voltages and Temperature */
    cout << "Temperature   : " << sm->FPGATemperature() << " °C" << endl
         << "FPGA VCCINT   : " << sm->VCCINT() << " V"  << endl
         << "FPGA VCCAUX   : " << sm->VCCAUX() << " V"  << endl;

    /** Print and check reported PCIe link width/speed */
    cout << "Detected as   : PCIe Gen" << pcieGeneration(bar1)
         << " x" << pcieNumberOfLanes(bar1) << endl;

    if( (pcieGeneration(bar1)!=2) || (pcieNumberOfLanes(bar1)!=8) )
    { cout << " WARNING: FPGA reports unexpexted PCIe link parameters!" << endl; }

    /** Check if system clock is running */
    cout << "SysClk locked : " << systemClockIsRunning(bar1) << endl;

    /** Check if fan is running */
    cout << "Fan speed     : " << systemFanSpeed(bar1) << " rpm" << endl;
    if( systemFanIsEnabled(bar1) == false)
    {
        cout << "WARNING: fan seems to be disabled!" << endl;
    }

    if( systemFanIsRunning(bar1) == false)
    {
        cout << "WARNING: fan seems to be stopped!" << endl;
    }

    /** Show QSFP status */
    cout << "QSFPs" << endl;
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        cout << endl << "-------------------------------------" << endl << endl;

        cout << "QSFP " << i << " present: " << qsfpIsPresent(bar1, i)  << endl;
        cout << "QSFP " << i << " LED0 : "   << qsfpLEDIsOn(bar1, i, 0)
                             << " LED1 : "   << qsfpLEDIsOn(bar1, i, 0) << endl;

        if( qsfpIsPresent(bar1, i) )
        {
            cout << "Checking QSFP" << i << " i2c access:" << endl;

            try
            {
                cout << "Vendor Name : " << qsfpVendorName(sm, i)  << endl;
                cout << "Part Number : " << qsfpPartNumber(sm, i)  << endl;
                cout << "Temperature : " << qsfpTemperature(sm, i) << "°C" << endl;
            }
            catch(...)
            {
                cout << "QSFP readout failed!" << endl;
            }
        }
    }

    cout << endl;
    cout << "-------------------------------------" << endl;

    exit(EXIT_SUCCESS);
}

//TODO :  MOVE to sysmon soon! ________________________________________________________

//QSFP


bool
qsfpIsPresent
(
    librorc_bar   *bar1,
    uint32_t       index
)
{
    uint32_t qsfp_ctrl = bar1->get(RORC_REG_QSFP_CTRL);

    if( ((~qsfp_ctrl)>>(8*index+2) & 0x01) == 1 )
    {
        return true;
    }

    return false;
}



bool
qsfpLEDIsOn
(
    librorc_bar   *bar1,
    uint32_t       qsfp_index,
    uint32_t       LED_index
)
{
    uint32_t qsfp_ctrl = bar1->get(RORC_REG_QSFP_CTRL);

    if( ((~qsfp_ctrl)>>(8*qsfp_index+LED_index) & 0x01) == 1 )
    {
        return true;
    }

    return false;
}



string*
qsfpVendorName
(
    rorcfs_sysmon *sm,
    uint32_t       index
)
{
    qsfp_set_page0_and_config(sm, index);
    return( qsfp_i2c_string_readout(sm, 148, 163) );
}



string*
qsfpPartNumber
(
    rorcfs_sysmon *sm,
    uint32_t       index
)
{
    qsfp_set_page0_and_config(sm, index);
    return( qsfp_i2c_string_readout(sm, 168, 183) );
}



    string*
    qsfp_i2c_string_readout
    (
        rorcfs_sysmon *sm,
        uint8_t        start,
        uint8_t        end
    )
    {
        string *readout = new string();
        uint8_t data_r = 0;
        for(uint8_t i=start; i<=end; i++)
        {
            data_r = sm->i2c_read_mem(SLVADDR, i);
            readout += (char)data_r;
        }
        return readout;
    }



float
qsfpTemperature
(
    struct rorcfs_sysmon *sm,
    uint32_t              index
)
{
    qsfp_set_page0_and_config(sm, index);

    uint8_t data_r;
    data_r = sm->i2c_read_mem(SLVADDR, 23);

    uint32_t temp = data_r;
    data_r = sm->i2c_read_mem(SLVADDR, 22);

    temp += ((uint32_t)data_r<<8);

    return ((float)temp/256);
}



    void
    qsfp_set_page0_and_config
    (
        rorcfs_sysmon *sm,
        uint32_t index
    )
    {
        uint8_t data_r;

        try
        {
            data_r = sm->i2c_read_mem(SLVADDR, 127);
        }
        catch(...)
        {
            cout << "Failed to read from i2c!" << endl;
            return;
        }

        if( data_r!=0 )
        {
            sm->i2c_write_mem(SLVADDR, 127, 0);
        }

        sm->i2c_set_config( 0x01f30081 | ((1<<index)<<8) );
    }

//PCI

uint32_t
pcieNumberOfLanes
(
    librorc_bar *bar1
)
{
    uint32_t status = bar1->get(RORC_REG_PCIE_CTRL);
    return(1<<(status>>3 & 0x3));
}



uint32_t
pcieGeneration
(
    librorc_bar *bar1
)
{
    uint32_t status = bar1->get(RORC_REG_PCIE_CTRL);
    return(1<<(status>>5 & 0x01));
}



//system
bool
systemClockIsRunning
(
    librorc_bar *bar1
)
{
    uint32_t ddrctrl = bar1->get(RORC_REG_DDR3_CTRL);
    if( ((ddrctrl>>3)&1) == 1 )
    { return true; }
    else
    { return false; }
}



bool
systemFanIsEnabled
(
    librorc_bar *bar1
)
{
    uint32_t fanctrl = bar1->get(RORC_REG_FAN_CTRL);
    if ( !(fanctrl & (1<<31)) )
    { return false; }
    else
    { return true; }
}



bool
systemFanIsRunning
(
    librorc_bar *bar1
)
{
    uint32_t fanctrl = bar1->get(RORC_REG_FAN_CTRL);
    if( !(fanctrl & (1<<29)) )
    { return false; }
    else
    { return true; }
}



double
systemFanSpeed
(
    librorc_bar *bar1
)
{
    uint32_t fanctrl = bar1->get(RORC_REG_FAN_CTRL);
    return 15/((fanctrl & 0x1fffffff)*0.000000004);
}
