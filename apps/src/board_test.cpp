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

uint32_t pcieNumberOfLanes(librorc_bar *bar1);
uint32_t pcieGeneration(librorc_bar *bar1);

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
    cout << "SysClk locked : " << sm->systemClockIsRunning() << endl;

    /** Check if fan is running */
    cout << "Fan speed     : " << sm->systemFanSpeed() << " rpm" << endl;
    if( sm->systemFanIsEnabled() == false)
    {
        cout << "WARNING: fan seems to be disabled!" << endl;
    }

    if( sm->systemFanIsRunning() == false)
    {
        cout << "WARNING: fan seems to be stopped!" << endl;
    }

    /** Show QSFP status */
    cout << "QSFPs" << endl;
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        cout << endl << "-------------------------------------" << endl << endl;
        try
        {
            cout << "QSFP " << i << " present: " << sm->qsfpIsPresent(i)  << endl;
            cout << "QSFP " << i << " LED0 : "   << sm->qsfpLEDIsOn(i, 0)
                                 << " LED1 : "   << sm->qsfpLEDIsOn(i, 1) << endl;

            if( sm->qsfpIsPresent(i) )
            {
                cout << "Checking QSFP" << i << " i2c access:" << endl;
                cout << "Vendor Name : " << sm->qsfpVendorName(i)  << endl;
                cout << "Part Number : " << sm->qsfpPartNumber(i)  << endl;
                cout << "Temperature : " << sm->qsfpTemperature(i) << "°C" << endl;
            }
        }
        catch(...)
        {
            cout << "QSFP readout failed!" << endl;
        }
    }

    cout << endl;
    cout << "-------------------------------------" << endl;

    exit(EXIT_SUCCESS);
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

