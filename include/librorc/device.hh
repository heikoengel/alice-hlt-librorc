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
 *
 * @section DESCRIPTION
 *
 * The librorc::device represents a RORC PCIe device
 */

#ifndef LIBRORC_DEVICE_H
#define LIBRORC_DEVICE_H

#include "include_ext.hh"
#include "defines.hh"


typedef struct DeviceOperator_struct DeviceOperator;
typedef struct PciDevice_struct PciDevice;

#define LIBRORC_DEVICE_ERROR_CONSTRUCTOR_FAILED    1
#define LIBRORC_DEVICE_ERROR_BUFFER_FREEING_FAILED 2

namespace LIBRARY_NAME
{
    /**
     * @brief represents a RORC PCIe device
     *
     * The librorc::device class is the base class for all device
     * IO. Create a librorc::device instance and initialize
     * (via init(int n)) with the device you want to bind to.
     * Once the device is sucessfully initialized you can attach
     * instances of librorc::bar.
     **/
    class device
    {
    friend class buffer;
    friend class bar;
    friend class rorc_bar;
    friend class sim_bar;

    public:
         device(int32_t device_index);
        ~device();

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

    uint64_t maxPayloadSize();
    uint64_t maxReadRequestSize();

    /**
     * get PCI Bar-Size
     * @return Bar-Size
     **/
    uint64_t getBarSize(uint8_t n);

    std::string *deviceDescription();

    uint8_t getDeviceId();

    void deleteAllBuffers();

    private:
        uint64_t m_maxPayloadSize;
        uint64_t m_maxReadRequestSize;

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

}

#endif /** LIBRORC_DEVICE_H */
