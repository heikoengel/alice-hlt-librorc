/**
 * @file rorcfs_device.cpp
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
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

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <pda.h>

#include "rorcfs_device.hh"

using namespace std;

rorcfs_device::rorcfs_device()
{
}



rorcfs_device::~rorcfs_device()
{
}



int
rorcfs_device::init
(
    int n
)
{
    /** A list of PCI ID to which PDA has to attach. */
    const char *pci_ids[] =
    {
        "10dc beaf", /* CRORC BETA */
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    if( (m_dop = DeviceOperator_new(pci_ids) ) == NULL)
    {
        cout << "Unable to get device-operator!" << endl;
        return -1;
    }

    /** Get a device object device n in the list. */
    m_device = NULL;
    if(DeviceOperator_getPciDevice(m_dop, &m_device, n) != PDA_SUCCESS)
    {
        cout << "Can't get device!" << endl;
        return -1;
    }

    m_number = n;

    return 0;
}



uint8_t
rorcfs_device::getBus()
{
    uint8_t bus_id;
    if(PciDevice_getBusID(m_device, &bus_id) == PDA_SUCCESS)
    {
        return(bus_id);
    }

    return(0);
}



uint8_t
rorcfs_device::getSlot()
{
    uint8_t device_id;
    if(PciDevice_getDeviceID(m_device, &device_id) == PDA_SUCCESS)
    {
        return(device_id);
    }

    return(0);
}



uint8_t
rorcfs_device::getFunc()
{
    uint8_t function_id;
    if(PciDevice_getFunctionID(m_device, &function_id) == PDA_SUCCESS)
    {
        return(function_id);
    }

    return(0);
}

uint8_t*
rorcfs_device::getBarMap
(
    uint8_t n
)
{
    uint8_t   *buffer = NULL;
    uint64_t   size;

    Bar *bar = NULL;
    if(PciDevice_getBar(m_device, &bar, n) != PDA_SUCCESS)
    {
        return(NULL);
    }

    if(Bar_getMap(bar, &buffer, &size) != PDA_SUCCESS)
    {
        return(NULL);
    }

    return(buffer);
}

uint64_t
rorcfs_device::getBarSize
(
    uint8_t n
)
{
    uint8_t   *buffer = NULL;
    uint64_t   size;

    Bar *bar = NULL;
    if(PciDevice_getBar(m_device, &bar, n) != PDA_SUCCESS)
    {
        return(0);
    }

    if(Bar_getMap(bar, &buffer, &size) != PDA_SUCCESS)
    {
        return(0);
    }

    return(size);
}

void
rorcfs_device::printDeviceDescription()
{
    uint16_t  domain_id   = 0;
    uint8_t   bus_id      = 0;
    uint8_t   device_id   = 0;
    uint8_t   function_id = 0;
    char     *description = (char*)malloc(1024 * sizeof(char));

        PdaDebugReturnCode ret = PDA_SUCCESS;
        ret += PciDevice_getDomainID(m_device, &domain_id);
        ret += PciDevice_getBusID(m_device, &bus_id);
        ret += PciDevice_getDeviceID(m_device, &device_id);
        ret += PciDevice_getFunctionID(m_device, &function_id);
        ret += PciDevice_getDescription(m_device, &description);

        if( ret != PDA_SUCCESS )
        {
            return;
        }

    printf("Device [%u] %04x:%02x:%02x.%x : %s", m_number,
            domain_id, bus_id, device_id, function_id, description);
    free(description);
}
