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

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fcntl.h>
#include "crorc-smbus-ctrl.hh"

using namespace std;

void
checkSmBus
(
    librorc::bar *bar,
    uint32_t verbose
)
{
    uint8_t dipswitch = getDipSwitch( bar );
    if (dipswitch==0 || dipswitch>0x7f)
    {
        cout << "ERROR: invalid dip switch setting "
             << HEXSTR(dipswitch, 1)
             << " - SMBus access is likely to fail!"
             << endl;
    }
    else
    {
        cout << "INFO: uC using slave address "
             << HEXSTR((int)dipswitch, 1) << endl;
    }

    int fd = 0;
    try
    {
        fd = crorc_smbus_init(0);
    }
    catch (...)
    {
        cout << "ERROR: failed to init SMBus!" << endl;
    }

    try
    {
        uint32_t byte = crorc_smbus_read_byte(fd, dipswitch, 0);
        if ( byte != UC_FW_REV_LSB )
        {
            cout << "ERROR: unexpected FW-Rev LSB via SMBus: "
                 << HEXSTR(byte, 2) << endl;
        }
    }
    catch(...)
    {
        cout << "ERROR: Failed to read FW revision from SMBus!"
             << endl;
    }

    crorc_smbus_close(fd);
}


int
crorc_smbus_init
(
    uint8_t chain
)
{
    char filename[15];
    sprintf(filename, "/dev/i2c-%d", chain);

    /** open i2c device file */
    int fd = open(filename, O_RDWR);
    if ( fd < 0 )
    {
        throw LIBRORC_SMBUS_OPEN_FAILED;
    }

    /** Get adapter functionality */
    unsigned long funcs;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0) 
    {
        throw LIBRORC_SMBUS_FUNCS_FAILED;
    }

    /** make sure adapter supports read_byte and write_byte */
    if( !(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA) ||
            !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA) )
    {
        throw LIBRORC_SMBUS_CAPS_FAILED;
    }

    return fd;
}


int
crorc_smbus_read_byte
(
   int fd,
   uint8_t slv_addr,
   uint8_t mem_addr
)
{
    /** select target slave address */
    if (ioctl(fd, I2C_SLAVE, slv_addr) < 0) 
    {
        perror("ioctl");
        throw LIBRORC_SMBUS_SLA_SELECT_FAILED;
    }

    usleep(100000);

    /** read byte */
    int result = i2c_smbus_read_byte_data(fd, mem_addr);
    if ( result < 0 )
    {
        perror("i2c_smbus_read_byte_data");
        throw LIBRORC_SMBUS_READ_FAILED;
    }
    return result;
}



void
crorc_smbus_close
(
    int fd
)
{
    close(fd);
}
