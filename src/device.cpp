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
#ifdef PDA
  #include <pda.h>
#endif

#include <stdlib.h>
#include <iomanip>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <fcntl.h>


#include <librorc/device.hh>
#include <librorc/error.hh>
#include <librorc/sysfs_handler.hh>
#include <librorc/registers.h>

#include <librorc/bar.hh>

namespace LIBRARY_NAME
{

#define SYSFS_ATTR_MAX_READ_REQ_SIZE "dma/max_read_request_size"
#define SYSFS_ATTR_MAX_PAYLOAD_SIZE "dma/max_payload_size"
#define SYSFS_ATTR_DEVICE_ID "device"
#define SYSFS_ATTR_VENDOR_ID "vendor"
#define SYSFS_ATTR_NUMA_NODE "numa_node"
#define SYSFS_ATTR_BAR "bar"

device::device(int32_t device_index)
{
    m_number = device_index;

#ifdef PDA
    if(PDAInit() != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDA_KMOD_MISMATCH; }

    if(PDACheckVersion(10,1,6) != PDA_SUCCESS)
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


    /** get MaxPayload and MaxReadRequest sizes from PDA */
    if(PciDevice_getmaxReadRequestSize(m_device, &m_maxReadRequestSize) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDAGET_FAILED; }
    if(PciDevice_getmaxPayloadSize(m_device, &m_maxPayloadSize) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_PDAGET_FAILED; }

#else

    m_hdl = new sysfs_handler(device_index);
    for(int i=0; i<LIBRORC_MAX_BARS; i++) {
        m_bar_map[i] = NULL;
        m_bar_size[i] = 0;
    }
#endif
}



device::~device()
{
#ifdef PDA
    DeviceOperator_delete(m_dop, PDA_DELETE_PERSISTANT);
#else
    for (int i = 0; i < LIBRORC_MAX_BARS; i++) {
        if( m_bar_map[i] != NULL ) {
            munmap(m_bar_map[i], m_bar_size[i]);
        }
    }
    delete m_hdl;
#endif
}



uint16_t
device::getDomain()
{
#ifdef PDA
    uint16_t domain_id;
    if(PciDevice_getDomainID(m_device, &domain_id) == PDA_SUCCESS)
    { return(domain_id); }

    return(0);
#else
    return strtoul(m_hdl->get_pci_slot_str().substr(0, 4).c_str(), NULL, 16);
#endif
}



uint8_t
device::getBus()
{
#ifdef PDA
    uint8_t bus_id;
    if(PciDevice_getBusID(m_device, &bus_id) == PDA_SUCCESS)
    { return(bus_id); }

    return(0);
#else
    return strtoul(m_hdl->get_pci_slot_str().substr(5, 2).c_str(), NULL, 16);
#endif
}



uint8_t
device::getSlot()
{
#ifdef PDA
    uint8_t device_id;
    if(PciDevice_getDeviceID(m_device, &device_id) == PDA_SUCCESS)
    { return(device_id); }

    return(0);
#else
    return strtoul(m_hdl->get_pci_slot_str().substr(8, 2).c_str(), NULL, 16);
#endif
}



uint8_t
device::getFunc()
{
#ifdef PDA
    uint8_t function_id;
    if(PciDevice_getFunctionID(m_device, &function_id) == PDA_SUCCESS)
    { return(function_id); }

    return(0);
#else
    return strtoul(m_hdl->get_pci_slot_str().substr(11, 1).c_str(), NULL, 16);
#endif
}


uint8_t
device::getDeviceId()
{
    return m_number;
}



uint8_t*
device::getBarMap(uint8_t n)
{
#ifdef PDA
    uint8_t   *buffer = NULL;
    uint64_t   size;

    Bar *bar = NULL;
    if(PciDevice_getBar(m_device, &bar, n) != PDA_SUCCESS)
    { return(NULL); }

    if(Bar_getMap(bar, (void**)&buffer, &size) != PDA_SUCCESS)
    { return(NULL); }

    return(buffer);
#else
  if (m_bar_map[n] == NULL) {
    char attr_name[strlen(SYSFS_ATTR_BAR)+2];
    snprintf(attr_name, sizeof(attr_name), "%s%d", SYSFS_ATTR_BAR, n);
    m_bar_size[n] = m_hdl->get_attribute_size(attr_name);
    if (m_bar_size[n] < 0) {
        return NULL;
    }
    if (m_hdl->mmap_file((void **)&m_bar_map[n], attr_name, m_bar_size[n], O_RDWR,
                           PROT_READ | PROT_WRITE) < 0) {
      m_bar_map[n] = NULL;
      return NULL;
    }
  }
  return m_bar_map[n];
#endif
}



uint64_t
device::getBarSize(uint8_t n)
{
#ifdef PDA
    uint8_t   *buffer = NULL;
    uint64_t   size;

    Bar *bar = NULL;
    if(PciDevice_getBar(m_device, &bar, n) != PDA_SUCCESS)
    { return(0); }

    if(Bar_getMap(bar, (void **)&buffer, &size) != PDA_SUCCESS)
    { return(0); }

    return(size);
#else
    return m_bar_size[n];
#endif
}

uint64_t device::maxPayloadSize() {
#ifdef PDA
  return (m_maxPayloadSize);
#else
  return m_hdl->get_bin_attr(SYSFS_ATTR_MAX_PAYLOAD_SIZE);
#endif
}

uint64_t device::maxReadRequestSize() {
#ifdef PDA
  return (m_maxReadRequestSize);
#else
  return m_hdl->get_bin_attr(SYSFS_ATTR_MAX_READ_REQ_SIZE);
#endif
}

std::string*
device::deviceDescription()
{
#ifdef PDA
    char description_buffer[1024];
    const char *description
        = (const char *)description_buffer;

    if(PciDevice_getDescription(m_device, &description ) != PDA_SUCCESS)
    { return NULL; }

    return( new std::string(description_buffer) );
#else
    return( new std::string("noDescription") );
#endif
}


void
device::deleteAllBuffers()
{
#ifdef PDA
    if( PciDevice_freeAllBuffers(m_device) != PDA_SUCCESS)
    { throw LIBRORC_DEVICE_ERROR_BUFFER_FREEING_FAILED; }
#else
    std::vector<uint64_t> bufferlist = m_hdl->list_all_buffers();
    std::vector<uint64_t>::iterator iter, end;
    iter = bufferlist.begin();
    end = bufferlist.end();
    while( iter != end ) {
        m_hdl->deallocate_buffer(*iter);
        ++iter;
    }
#endif
}

}
