/**
 * @file rorcfs_device.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @date 2011-08-16
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
 **/

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

#include <pda.h>

#include "rorcfs_device.hh"

using namespace std;

rorcfs_device::rorcfs_device()
{
}

rorcfs_device::~rorcfs_device()
{
}

int rorcfs_device::init(int n)
{
    /** A list of PCI ID to which PDA has to attach. */
    const char *pci_ids[] =
    {
        "10dc beaf", /* CRORC BETA */
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    if( (dop=DeviceOperator_new(pci_ids)) == NULL )
    {
        cout << "Unable to get device-operator!" << endl;
        return -1;
    }

    /** Get a device object device n in the list. */
    device = NULL;
    if(DeviceOperator_getPciDevice(dop, &device, n) != PDA_SUCCESS)
    {
        cout << "Can't get device!" << endl;
        return -1;
    }

	return 0;
}

uint8_t rorcfs_device::getBus()
{
    uint8_t bus_id;
    if(PciDevice_getBusID(device, &bus_id) == PDA_SUCCESS)
    {
        return(bus_id);
    }

    return(0);
}

uint8_t rorcfs_device::getSlot()
{

}

uint8_t rorcfs_device::getFunc()
{

}
