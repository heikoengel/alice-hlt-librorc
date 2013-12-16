/**
 * @file crorc-smbus-ctrl.hh
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
#ifndef CRORC_SMBUS_CTRL_H
#define CRORC_SMBUS_CTRL_H

#include <librorc.h>
#include <pda.h>

#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define LIBRORC_SMBUS_OPEN_FAILED 1
#define LIBRORC_SMBUS_FUNCS_FAILED 2
#define LIBRORC_SMBUS_CAPS_FAILED 3
#define LIBRORC_SMBUS_SLA_SELECT_FAILED 4
#define LIBRORC_SMBUS_READ_FAILED 5


/**
 * initialize SMBus chain
 * @param chain use /dev/i2c-devX, where X=chain
 * @return file descriptor on success, <0 on error
 **/
int
crorc_smbus_init
(
    uint8_t chain
);


/**
 * read byte from SMBus chain
 * @param fd file descriptor from crorc_smbus_init
 * @param slv_addr slave address of the device to read from
 * @param mem_addr memory address in the slave device to read from
 * @param signed int, <0 on error, read byte on success
 **/
int
crorc_smbus_read_byte
(
   int fd,
   uint8_t slv_addr,
   uint8_t mem_addr
);


/**
 * close SMBus chain
 * @param fd file descriptor
 **/
void
crorc_smbus_close
(
    int fd
);



/****************************************
 *        Code from LM_SENSORS
 ***************************************/

static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command, 
                                     int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}


static inline __s32 i2c_smbus_write_quick(int file, __u8 value)
{
	return i2c_smbus_access(file,value,0,I2C_SMBUS_QUICK,NULL);
}
	
static inline __s32 i2c_smbus_read_byte(int file)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,0,I2C_SMBUS_BYTE,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte(int file, __u8 value)
{
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,value,
	                        I2C_SMBUS_BYTE,NULL);
}

static inline __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_BYTE_DATA,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte_data(int file, __u8 command, 
                                              __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                        I2C_SMBUS_BYTE_DATA, &data);
}

static inline __s32 i2c_smbus_read_word_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_WORD_DATA,&data))
		return -1;
	else
		return 0x0FFFF & data.word;
}

static inline __s32 i2c_smbus_write_word_data(int file, __u8 command, 
                                              __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                        I2C_SMBUS_WORD_DATA, &data);
}

static inline __s32 i2c_smbus_process_call(int file, __u8 command, __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	if (i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                     I2C_SMBUS_PROC_CALL,&data))
		return -1;
	else
		return 0x0FFFF & data.word;
}


/* Returns the number of read bytes */
static inline __s32 i2c_smbus_read_block_data(int file, __u8 command, 
                                              __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_BLOCK_DATA,&data))
		return -1;
	else {
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

static inline __s32 i2c_smbus_write_block_data(int file, __u8 command, 
                                               __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                        I2C_SMBUS_BLOCK_DATA, &data);
}

/* Returns the number of read bytes */
/* Until kernel 2.6.22, the length is hardcoded to 32 bytes. If you
   ask for less than 32 bytes, your code will only work with kernels
   2.6.23 and later. */
static inline __s32 i2c_smbus_read_i2c_block_data(int file, __u8 command,
                                                  __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	int i;

	if (length > 32)
		length = 32;
	data.block[0] = length;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
	                      I2C_SMBUS_I2C_BLOCK_DATA,&data))
		return -1;
	else {
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

static inline __s32 i2c_smbus_write_i2c_block_data(int file, __u8 command,
                                               __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                        I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

/* Returns the number of read bytes */
static inline __s32 i2c_smbus_block_process_call(int file, __u8 command,
                                                 __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	if (i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
	                     I2C_SMBUS_BLOCK_PROC_CALL,&data))
		return -1;
	else {
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

#endif
