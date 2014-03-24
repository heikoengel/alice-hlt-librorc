/**
 * @file crorc-smbus-ctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-11-08
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
        DEBUG_PRINTF(PDADEBUG_ERROR, "error opening i2c device %d", chain);
        throw LIBRORC_SMBUS_OPEN_FAILED;
    }

    /** Get adapter functionality */
    unsigned long funcs;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0) 
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "ioctl() I2C_FUNCS failed");
        throw LIBRORC_SMBUS_FUNCS_FAILED;
    }

    /** make sure adapter supports read_byte and write_byte */
    if( !(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA) ||
            !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA) )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "Adapter does not support "
                "read_byte_data or write_byte_data!");
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
        DEBUG_PRINTF(PDADEBUG_ERROR, "Failed to select slave address\n");
        perror("ioctl");
        throw LIBRORC_SMBUS_SLA_SELECT_FAILED;
    }

    usleep(100000);

    /** read byte */
    int result = i2c_smbus_read_byte_data(fd, mem_addr);
    if ( result < 0 )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "i2c_smbus_read_byte failed %x:%x\n",
                slv_addr, mem_addr);
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
