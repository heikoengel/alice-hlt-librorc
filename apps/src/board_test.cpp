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
extern "C"{
#include <pci/pci.h>
}

#include "librorc.h"

using namespace std;


#ifndef RORC_REG_DDR3_CTRL
#define RORC_REG_DDR3_CTRL 0
#endif

#define SLVADDR 0x50



void qsfp_set_page0(struct rorcfs_sysmon *sm);
void qsfp_print_vendor_name(struct rorcfs_sysmon *sm);
void qsfp_print_part_number(struct rorcfs_sysmon *sm);
void qsfp_print_temp(struct rorcfs_sysmon *sm);



int main(int argc, char **argv)
{
    rorcfs_device      *dev  = NULL;
    librorc_bar        *bar1 = NULL;
    rorcfs_sysmon      *sm   = NULL;

    struct    pci_access *pacc = NULL;
    struct    pci_dev *pdev    = NULL;
    uint64_t  device_serial    = 0;
    uint8_t  *devserptr        = (uint8_t *)&device_serial;

    uint32_t ddrctrl, fwrev, fwdate;
    uint32_t status;
    uint32_t qsfp_ctrl, fanctrl;
    int i;

    /** create new device object */
    dev = new rorcfs_device();
    if ( dev->init(0) == -1 )
    {
        printf("failed to initialize device 0 - "
        "is the board detected with lspci?\n");
        goto out;
    }

    /** bind to BAR1 */
    #ifdef SIM
        bar1 = new sim_bar(dev, 1);
    #else
        bar1 = new rorc_bar(dev, 1);
    #endif

    if ( bar1->init() == -1 )
    {
        printf("BAR1 init failed - check lspci if more "
        "than one device is bound to rorcfs\n");
        goto out;
    }

    /** instantiate a new sysmon */
    sm = new rorcfs_sysmon();
    if ( sm->init(bar1) == -1 )
    {
        printf("Sysmon init failed\n");
        goto out;
    }

    /** read device DNA, Firmware date & rev */
    pacc = pci_alloc();		  /** Get the pci_access structure */
    pci_init(pacc);		      /** Initialize the PCI library */
    pci_scan_bus(pacc);		  /** We want to get the list of devices */
    pdev = pci_get_dev(pacc, 0, dev->getBus(), dev->getSlot(), dev->getFunc());
    pci_read_block(pdev, 0x104, devserptr, 8);

    fwrev = sm->getFwRevision();
    if( fwrev==0xffffffff )
    {
        cout << "Reading FW-Revision returned -1. This is likely a "
             << "PCIe access problem. Check lspci if the device is "
             << "up and the BARs are enabled" << endl;
        goto out;
    }

    fwdate = sm->getFwBuildDate();
    if( fwdate==0xffffffff )
    {
        cout << "Reading FW-Date returned -1. This is likely a "
             << "PCIe access problem. Check lspci if the device is up"
             << "and the BARs are enabled" << endl;
        goto out;
    }

    /** Printout the stuff */
    printf("\n-=== FPGA ===-\n"
      "Firmware Rev.  %08x\nFirmare Date:  %08x\n"
      "Serial Number: 0x%016lx\n",
      fwrev, fwdate, device_serial);

  // read Voltages, Temperature
  printf("Temperature:   %.1f °C\n", sm->getFPGATemperature());
  printf("FPGA VCCINT:   %.2f V\n", sm->getVCCINT());
  printf("FPGA VCCAUX:   %.2f V\n", sm->getVCCAUX());

  // read reported PCIe link width/speed
  status = bar1->get(RORC_REG_PCIE_CTRL);
  if ((status>>3 & 0x3)!=3 || !(status>>5 & 0x01)) {
    printf(" WARNING: FPGA reports unexpexted PCIe link parameters:\n");
    printf("reported PCIe Link width: %d lanes (expected 8)\n",
        (1<<(status>>3 & 0x3)));
    printf("reported PCIe Link Gen: %d (expected 2)\n",
        (1<<(status>>5 & 0x01)));
  } else
    printf("Detected as:   PCIe Gen2 x8\n");

  // check if system clock is running
  ddrctrl = bar1->get(RORC_REG_DDR3_CTRL);
  printf("SysClk locked: %d\n", (ddrctrl>>3)&1);

  // check if fan is running
  fanctrl = bar1->get(RORC_REG_FAN_CTRL);
  if ( !(fanctrl & (1<<31)) )
    printf("WARNING: fan seems to be disabled!");
  else if ( !(fanctrl & (1<<29)) )
    printf("WARNING: fan seems to be stopped!");
  else
    printf("Fan:           %.1f RPM\n",
        15/((fanctrl & 0x1fffffff)*0.000000004));

  // read QSFP CTRL
  printf("\n-=== QSFPs ===-\n");
  qsfp_ctrl = bar1->get(RORC_REG_QSFP_CTRL);
  for(i=0;i<3;i++) {
    printf("QSFP %d present: %d\n", i, ((~qsfp_ctrl)>>(8*i+2) & 0x01));
    printf("QSFP %d LED0: %d, LED1: %d\n", i,
        ((~qsfp_ctrl)>>(8*i) & 0x01),
        ((~qsfp_ctrl)>>(8*i+1) & 0x01));

    //TODO: read module temp & model
    if((~qsfp_ctrl)>>(8*i+2) & 0x01) {
      printf("Checking QSFP%d i2c access:\n", i);
      sm->i2c_set_config(0x01f30081 | ((1<<i)<<8));
      qsfp_set_page0(sm);
      qsfp_print_vendor_name(sm);
      qsfp_print_part_number(sm);
      qsfp_print_temp(sm);
    }
    printf("\n");
  }



out:
  if (bar1)
    delete bar1;
  if (dev)
    delete dev;

  exit(EXIT_SUCCESS);
}

//TODO :  MOVE to sysmon soon! ________________________________________________________

void qsfp_set_page0(struct rorcfs_sysmon *sm)
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
}



void qsfp_print_vendor_name(struct rorcfs_sysmon *sm)
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
