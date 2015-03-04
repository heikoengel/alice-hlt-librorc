/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/
#include <pthread.h>
#include <pda.h>
#include <iomanip>
#include <string>

#include <librorc/device.hh>
#include <librorc/registers.h>

#include <librorc/bar.hh>

namespace LIBRARY_NAME
{

device::device(int32_t device_index)
{
    if(PDAInit() != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDA_KMOD_MISMATCH; }

    if(PDACheckVersion(8,1,4) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDA_VERSION_MISMATCH; }

    /** A list of PCI ID to which PDA has to attach. */
    const char *pci_ids[] =
    {
        "10dc 01a0", /* CRORC as registered at CERN */
#ifdef MODELSIM
        "17aa 1e22", /* Heikos SMBus Controller for modelsim*/
        "8086 1e22", /* Heikos SMBus Controller for modelsim*/
#endif
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    if( (m_dop = DeviceOperator_new(pci_ids) ) == NULL)
    { throw LIBRORC_DEVICE_ERROR_PDADOP_FAILED; }

    /** Get a device object device from the list. */
    if(DeviceOperator_getPciDevice(m_dop, &m_device, device_index) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDADEV_FAILED; }

    m_number = device_index;

    /** get MaxPayload and MaxReadRequest sizes from PDA */
    if(PciDevice_getmaxReadRequestSize(m_device, &m_maxReadRequestSize) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDAGET_FAILED; }
    if(PciDevice_getmaxPayloadSize(m_device, &m_maxPayloadSize) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDAGET_FAILED; }
}



device::~device()
{
    DeviceOperator_delete(m_dop, PDA_DELETE_PERSISTANT);
}



uint16_t
device::getDomain()
{
    uint16_t domain_id;
    if(PciDevice_getDomainID(m_device, &domain_id) == PDA_SUCCESS)
    { return(domain_id); }

    return(0);
}



uint8_t
device::getBus()
{
    uint8_t bus_id;
    if(PciDevice_getBusID(m_device, &bus_id) == PDA_SUCCESS)
    { return(bus_id); }

    return(0);
}



uint8_t
device::getSlot()
{
    uint8_t device_id;
    if(PciDevice_getDeviceID(m_device, &device_id) == PDA_SUCCESS)
    { return(device_id); }

    return(0);
}



uint8_t
device::getFunc()
{
    uint8_t function_id;
    if(PciDevice_getFunctionID(m_device, &function_id) == PDA_SUCCESS)
    { return(function_id); }

    return(0);
}


uint8_t
device::getDeviceId()
{
    return m_number;
}



uint8_t*
device::getBarMap(uint8_t n)
{
    uint8_t   *buffer = NULL;
    uint64_t   size;

    Bar *bar = NULL;
    if(PciDevice_getBar(m_device, &bar, n) != PDA_SUCCESS)
    { return(NULL); }

    if(Bar_getMap(bar, &buffer, &size) != PDA_SUCCESS)
    { return(NULL); }

    return(buffer);
}



uint64_t
device::getBarSize(uint8_t n)
{
    uint8_t   *buffer = NULL;
    uint64_t   size;

    Bar *bar = NULL;
    if(PciDevice_getBar(m_device, &bar, n) != PDA_SUCCESS)
    { return(0); }

    if(Bar_getMap(bar, &buffer, &size) != PDA_SUCCESS)
    { return(0); }

    return(size);
}


uint64_t
device::maxPayloadSize()
{
    return(m_maxPayloadSize);
}


uint64_t
device::maxReadRequestSize()
{
    return(m_maxReadRequestSize);
}


std::string*
device::deviceDescription()
{
    char description_buffer[1024];
    const char *description
        = (const char *)description_buffer;

    if(PciDevice_getDescription(m_device, &description ) != PDA_SUCCESS)
    { return NULL; }

    return( new std::string(description_buffer) );
}


void
device::deleteAllBuffers()
{
    if( PciDevice_freeAllBuffers(m_device) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_BUFFER_FREEING_FAILED; }
}

}
