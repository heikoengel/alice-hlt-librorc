/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
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
 *
 * @section DESCRIPTION
 *
 * The rorcfs_device represents a RORC PCIe device
 */

#ifndef LIBRORC_DEVICE_H
#define LIBRORC_DEVICE_H

#include "include_ext.hh"


typedef struct DeviceOperator_struct DeviceOperator;
typedef struct PciDevice_struct PciDevice;

#define LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED  1

/** conditional debug printout command **/
#ifdef DEBUG
    #define librorc_debug(fmt, args ...) printf(fmt, ## args)
#else
    #define librorc_debug(fmt, args ...)
#endif

/**
 * @class rorcfs_device
 * @brief represents a RORC PCIe device
 *
 * The rorcfs_device class is the base class for all device
 * IO. Create a rorcfs_device instance and initialize
 * (via init(int n)) with the device you want to bind to.
 * Once the device is sucessfully initialized you can attach
 * instances of rorcfs_bar.
 **/
class rorcfs_device
{
friend class rorcfs_buffer;
friend class librorc_bar;
friend class rorc_bar;
friend class sim_bar;

public:
    rorcfs_device(int32_t device_index);
    ~rorcfs_device();

uint16_t getDomain();

/**
 * get PCIe Bus-ID
 * @return Bus-ID
 **/
uint8_t getBus();

/**
 * get PCIe Slot-ID
 * @return Slot-ID
 **/
uint8_t getSlot();

/**
 * get PCIe Function-ID
 * @return Function-ID
 **/
uint8_t getFunc();

/**
 * get PCI Bar
 * @return Bar
 **/
uint8_t *getBarMap(uint8_t n);

/**
 * get PCI Bar-Size
 * @return Bar-Size
 **/
uint64_t getBarSize(uint8_t n);

string* deviceDescription();


private:

    /**
     * get PCI-Device
     * @return PCI-Device-Pointer
     **/
    PciDevice *getPdaPciDevice()
    {
        return(m_device);
    }

    DeviceOperator *m_dop;
    PciDevice      *m_device;
    uint8_t         m_number;

};

#endif /** LIBRORC_DEVICE_H */
