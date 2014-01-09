/**
 * @file
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

#include <librorc/device.hh>
#include <librorc/registers.h>
#include <pda.h>

#include <librorc/bar.hh>



namespace LIBRARY_NAME
{

device::device
(
    int32_t       device_index,
    LibrorcEsType esType
)
{
    PDAInit();

    if(PDACheckVersion(7, 0,1) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }

    /** A list of PCI ID to which PDA has to attach. */
    const char *pci_ids[] =
    {
        "10dc 01a0", /* CRORC as registered at CERN */
        "10dc beaf", /* CRORC BETA */
        "17aa 1e22", /* Heikos SMBus Controller for modelsim*/
        "8086 1e22", /* Heikos SMBus Controller for modelsim*/
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    if( (m_dop = DeviceOperator_new(pci_ids) ) == NULL)
    { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }

    /** Get a device object device from the list. */
    if(DeviceOperator_getPciDevice(m_dop, &m_device, device_index) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }

    #ifndef SIM
        Bar *bar;
        if(PciDevice_getBar(m_device, &bar, 1) != PDA_SUCCESS)
        { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }

        uint8_t *buffer = NULL;
        uint64_t length = 0;
        if(Bar_getMap(bar, &buffer, &length) != PDA_SUCCESS)
        { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }
        uint32_t *bar32 = (uint32_t *)bar;

        switch (esType)
        {
            case LIBRORC_ES_BOTH:
            break;

            case LIBRORC_ES_IN_GENERIC:
            case LIBRORC_ES_IN_DDL:
            case LIBRORC_ES_IN_HWPG:
            {
                if((bar32[RORC_REG_TYPE_CHANNELS] >> 16) != RORC_CFG_PROJECT_hlt_in)
                { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }
            }
            break;

            case LIBRORC_ES_OUT_GENERIC:
            case LIBRORC_ES_OUT_SWPG:
            case LIBRORC_ES_OUT_FILE:
            {
                if((bar32[RORC_REG_TYPE_CHANNELS] >> 16) != RORC_CFG_PROJECT_hlt_out)
                { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }
            }
            break;

            default:
            { throw LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED; }
        }

    #endif

    m_number = device_index;
}



device::~device()
{
}



uint16_t
device::getDomain()
{
    uint16_t domain_id;
    if(PciDevice_getDomainID(m_device, &domain_id) == PDA_SUCCESS)
    {
        return(domain_id);
    }

    return(0);
}



uint8_t
device::getBus()
{
    uint8_t bus_id;
    if(PciDevice_getBusID(m_device, &bus_id) == PDA_SUCCESS)
    {
        return(bus_id);
    }

    return(0);
}



uint8_t
device::getSlot()
{
    uint8_t device_id;
    if(PciDevice_getDeviceID(m_device, &device_id) == PDA_SUCCESS)
    {
        return(device_id);
    }

    return(0);
}



uint8_t
device::getFunc()
{
    uint8_t function_id;
    if(PciDevice_getFunctionID(m_device, &function_id) == PDA_SUCCESS)
    {
        return(function_id);
    }

    return(0);
}


uint8_t
device::getDeviceId()
{
    return m_number;
}



uint8_t*
device::getBarMap
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
device::getBarSize
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



string*
device::deviceDescription()
{
    char description_buffer[1024];
    const char *description
        = (const char *)description_buffer;

    if
    (
        PciDevice_getDescription(m_device, &description )
            != PDA_SUCCESS
    )
    {
        return NULL;
    }

    return( new string(description_buffer) );
}



bool
device::DMAChannelIsImplemented
(
    int32_t channelId
)
{
    #ifndef SIM
    bar *bar1 = new rorc_bar(this, 1);

    if( channelId >= (int32_t)(bar1->get32(RORC_REG_TYPE_CHANNELS) & 0xffff) )
    {
        delete(bar1);
        return false;
    }

    delete(bar1);
    #endif

    return true;
}



}
