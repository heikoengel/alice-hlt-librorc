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

#define SLVADDR 0x50
#define LIBRORC_MAX_QSFP 3



void qsfp_set_page0_and_config
(
    struct rorcfs_sysmon *sm,
    uint32_t index
);


void qsfp_print_vendor_name
(
    rorcfs_sysmon *sm,
    uint32_t       index
);

void        qsfp_print_part_number(struct rorcfs_sysmon *sm);
void        qsfp_print_temp(struct rorcfs_sysmon *sm);

uint32_t    pcieNumberOfLanes(librorc_bar *bar1);
uint32_t    pcieGeneration(librorc_bar *bar1);

bool        systemClockIsRunning(librorc_bar *bar1);
bool        systemFanIsEnabled(librorc_bar *bar1);
bool        systemFanIsRunning(librorc_bar *bar1);
double      systemFanSpeed(librorc_bar *bar1);


int main(int argc, char **argv)
{
    uint32_t qsfp_ctrl;

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
    rorcfs_sysmon *sm = new rorcfs_sysmon();
    if ( sm->init(bar1) == -1 )
    {
        cout << "Sysmon init failed!" << endl;
        delete bar1;
        delete dev;
        abort();
    }

    /** Printout revision, date, and serial number */
    uint64_t device_serial = 0;
    try
    {
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setprecision(8) << sm->getFwRevision()  << dec << endl
             << "Firmware Date : " << hex << setprecision(8) << sm->getFwBuildDate() << dec << endl
             << "Serial Number : " << hex << showbase << device_serial << dec << endl;
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

    /** Print Voltages, Temperature */
    cout << "Temperature   : " << sm->getFPGATemperature() << " °C" << endl
         << "FPGA VCCINT   : " << sm->getVCCINT() << " V" << endl
         << "FPGA VCCAUX   : " << sm->getVCCAUX() << " V" << endl;

    /** print and check reported PCIe link width/speed */
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

//DONE
  // read QSFP CTRL
    cout << "QSFPs" << endl;

    qsfp_ctrl = bar1->get(RORC_REG_QSFP_CTRL);

    for(uint32_t i=0;i<3;i++)
    {
        printf("QSFP %d present: %d\n", i, ((~qsfp_ctrl)>>(8*i+2) & 0x01));
        printf("QSFP %d LED0: %d, LED1: %d\n", i,
        ((~qsfp_ctrl)>>(8*i) & 0x01),
        ((~qsfp_ctrl)>>(8*i+1) & 0x01));

        //TODO: read module temp & model
        if((~qsfp_ctrl)>>(8*i+2) & 0x01)
        {
            printf("Checking QSFP%d i2c access:\n", i);

            qsfp_set_page0_and_config(sm, i);
            qsfp_print_vendor_name(sm, i);
            qsfp_print_part_number(sm);
            qsfp_print_temp(sm);
        }
        printf("\n");
    }

    for(uint8_t i; i<LIBRORC_MAX_QSFP; i++)
    {

    }

    exit(EXIT_SUCCESS);
}

//TODO :  MOVE to sysmon soon! ________________________________________________________

//QSFP

void qsfp_set_page0_and_config
(
    struct rorcfs_sysmon *sm,
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

    sm->i2c_set_config(0x01f30081 | ((1<<index)<<8));
}



void qsfp_print_vendor_name
(
    rorcfs_sysmon *sm,
    uint32_t       index
)
{
    cout << "Vendor Name: ";

    uint8_t data_r;
    //TODO this is redundant
    for(uint8_t i=148; i<=163; i++)
    {
        try
        { data_r = sm->i2c_read_mem(SLVADDR, i); }
        catch(...)
        {
            cout << "Failed to read from i2c!" << endl;
            return;
        }
        cout << (char)data_r;
    }
    cout << endl;
}



void qsfp_print_part_number(struct rorcfs_sysmon *sm)
{
    cout << "Part Number: ";

    uint8_t data_r = 0;
    for(uint8_t i=168; i<=183; i++)
    {
        try
        { data_r = sm->i2c_read_mem(SLVADDR, i); }
        catch(...)
        {
            cout << "Failed to read from i2c!" << endl;
            return;
        }
        cout << (char)data_r;
    }
    cout << endl;
}



void qsfp_print_temp(struct rorcfs_sysmon *sm)
{
    uint8_t data_r;
    try
    { data_r = sm->i2c_read_mem(SLVADDR, 23); }
    catch(...)
    {
        cout << "Failed to read from i2c!" << endl;
        return;
    }

    uint32_t temp = data_r;
    try
    { data_r = sm->i2c_read_mem(SLVADDR, 22); }
    catch(...)
    {
        cout << "Failed to read from i2c!" << endl;
        return;
    }

    temp += ((uint32_t)data_r<<8);
    printf("\tTemperature:\t%.2f °C\n", ((float)temp/256));
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
