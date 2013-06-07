/**
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2012-02-29
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <iomanip>

#include "rorcfs_bar.hh"
#include "librorc_flash.hh"

using namespace std;

librorc_flash::librorc_flash
(
    librorc_bar            *flashbar,
    uint64_t                chip_select,
    librorc_verbosity_enum  verbose
)
{
    if(flashbar == NULL)
    {
        throw 1;
    }

    if(chip_select & 0xfffffffffffffffe)
    {
        throw 2;
    }

    bar        = flashbar;
    read_state = 0;
    base_addr = chip_select << FLASH_CHIP_SELECT_BIT;

    uint16_t status = getStatusRegister(0);

    if( status != 0x0080 )
    {
        clearStatusRegister(0);
        usleep(100);
        if ( status & 0x0084 )
        {
            programResume(0);
        }
        status = getStatusRegister(0);
    }
}



librorc_flash::~librorc_flash()
{
    bar = NULL;
    read_state = 0;
}



void
librorc_flash::setReadState
(
    uint16_t state,
    uint32_t addr
)
{
    bar->set16(base_addr + addr, state);
    read_state = state;
}



uint16_t
librorc_flash::getStatusRegister
(
    uint32_t blkaddr
)
{
    if(read_state != FLASH_READ_STATUS)
    {
        setReadState(FLASH_READ_STATUS, blkaddr);
    }

    return bar->get16(base_addr + blkaddr);
}



void
librorc_flash::clearStatusRegister
(
    uint32_t blkaddr
)
{
    bar->set16(base_addr + blkaddr, FLASH_CMD_CLR_STS);
    setReadState(FLASH_READ_ARRAY, blkaddr);
}



uint16_t
librorc_flash::getManufacturerCode()
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }

    return bar->get16(base_addr + 0x00);
}



uint16_t
librorc_flash::getDeviceID()
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }

    return bar->get16(base_addr + 0x01);
}



uint16_t
librorc_flash::getBlockLockConfiguration
(
    uint32_t blkaddr
)
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, blkaddr);
    }

    return bar->get16(base_addr + blkaddr + 0x02);
}



uint16_t
librorc_flash::getReadConfigurationRegister()
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }

    return bar->get16(base_addr + 0x05);
}


uint64_t librorc_flash::getUniqueDeviceNumber()
{
    uint64_t udn = 0;
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }
    for ( int i=0; i<4; i++ )
    {
        udn += ( bar->get16(base_addr + 0x81 + i) )<<(16*i);
    }
    return udn;
}


uint16_t
librorc_flash::get
(
    uint32_t addr
)
{
    if(read_state != FLASH_READ_ARRAY)
    {
        setReadState(FLASH_READ_ARRAY, addr);
    }
    return bar->get16(base_addr + addr);
}



int
librorc_flash::programWord
(
    uint32_t addr,
    uint16_t data
)
{
    uint16_t status  = 0;
    uint32_t   timeout = CFG_FLASH_TIMEOUT;

    /** word program setup */
    bar->set16(base_addr + addr, FLASH_CMD_PROG_SETUP);

    bar->set16(base_addr + addr, data);
    read_state = FLASH_READ_STATUS;
    status = bar->get16(base_addr + addr);

    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = bar->get16(base_addr + addr);
        timeout--;
        if(timeout == 0)
        {
            return -1;
        }
    }

    if(status == FLASH_PEC_BUSY)
    {
        clearStatusRegister(addr);
        return 0;
    }
    else
    {
        return -status;
    }
}



int
librorc_flash::programBuffer
(
    uint32_t  addr,
    uint16_t  length,
    uint16_t *data
)
{
    int      i;
    uint16_t status;
    uint32_t timeout = CFG_FLASH_TIMEOUT;
    uint32_t blkaddr = addr & CFG_FLASH_BLKMASK;

    /** check if block is locked */
    if( getBlockLockConfiguration(blkaddr) )
    {
        unlockBlock(blkaddr);
    }

    /** write to buffer command */
    bar->set16(base_addr + blkaddr, FLASH_CMD_BUFFER_PROG);
    read_state = FLASH_READ_STATUS;

    /** read status register */
    status = bar->get16(base_addr + blkaddr);
    if( (status & FLASH_PEC_BUSY) == 0)                                           //
    {
        return -1;
    }

    /** write word count */
    bar->set16(base_addr + blkaddr, length - 1);

    bar->set16(base_addr + addr, data[0]);

    for(i=1; i < length; i++)
    {
        bar->set16(base_addr + addr + i, data[i]);
    }

    bar->set16(base_addr + blkaddr, FLASH_CMD_CONFIRM);
    status = bar->get16(base_addr + blkaddr);

    /** Wait while device is busy */
    while( !(status & FLASH_PEC_BUSY) )
    {
        usleep(100);
        status = bar->get16(base_addr + blkaddr);
        timeout--;
        if(timeout == 0)
        {
            return -status;
        }
    }

    /** SR.5 or SR.4 nonzero -> program/erase/sequence error */
    if(status != FLASH_PEC_BUSY)
    {
        return -status;
    }

    /** return to ReadArry mode */
    setReadState(FLASH_READ_ARRAY, blkaddr);
    return 0;
}



int
librorc_flash::eraseBlock
(
    uint32_t blkaddr
)
{
    uint16_t status;
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    /** erase command */
    bar->set16(base_addr + blkaddr, FLASH_CMD_BLOCK_ERASE_SETUP);

    /** confim command */
    bar->set16(base_addr + blkaddr, FLASH_CMD_CONFIRM);
    read_state = FLASH_READ_STATUS;
    status = bar->get16(base_addr + blkaddr);

    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = bar->get16(base_addr + blkaddr);
        timeout--;
        if(timeout == 0)
        {
            return -status;
        }
    }

    if(status == FLASH_PEC_BUSY)
    {
        clearStatusRegister(blkaddr);
        return 0;
    }
    else
    {
        return -status;
    }
}



void
librorc_flash::programSuspend
(
    uint32_t blkaddr
)
{
    bar->set16(base_addr + blkaddr, FLASH_CMD_PROG_ERASE_SUSPEND);
}



void
librorc_flash::programResume
(
    uint32_t blkaddr
)
{
    bar->set16(base_addr + blkaddr, FLASH_CMD_CONFIRM);
}



void
librorc_flash::lockBlock
(
    uint32_t blkaddr
)
{
    bar->set16(base_addr + blkaddr, FLASH_CMD_BLOCK_LOCK_SETUP);
    bar->set16(base_addr + blkaddr, FLASH_CMD_BLOCK_LOCK_CONFIRM);
}



void
librorc_flash::unlockBlock
(
    uint32_t blkaddr
)
{
    bar->set16(base_addr + blkaddr, FLASH_CMD_BLOCK_LOCK_SETUP);
    bar->set16(base_addr + blkaddr, FLASH_CMD_CONFIRM);
}



void
librorc_flash::setConfigReg
(
    uint32_t value
)
{
    bar->set16(base_addr + value, FLASH_CMD_BLOCK_LOCK_SETUP);
    bar->set16(base_addr + value, FLASH_CMD_CFG_REG_CONFIRM);
}



int
librorc_flash::blankCheck
(
    uint32_t blkaddr
)
{
    uint16_t status;
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    bar->set16(base_addr + blkaddr, FLASH_CMD_BC_SETUP);
    bar->set16(base_addr + blkaddr, FLASH_CMD_BC_CONFIRM);
    read_state = FLASH_READ_STATUS;
    status = bar->get16(base_addr + blkaddr);

    /** wait for blankCheck to complete */
    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = bar->get16(base_addr + blkaddr);
        timeout--;
        if(timeout == 0)
        {
            return -status;
        }
    }
    clearStatusRegister(blkaddr);

    /**
     * SR5==1: block not empty -> return 0
     * SR5==0: block empty -> return 1
     **/
    return ~(status >> 5) & 0x01;
}


int64_t
librorc_flash::getFlashArchitecture
(
    uint32_t addr,
    struct flash_architecture *arch
)
{
    struct flash_architecture fa;
     /** 8Mbit (1MByte) for all Banks */
    fa.banksize = 0x100000;
    fa.bankaddr = addr & 0xfff80000;
    fa.banknum = 15 - (addr >> 19);

    /** check if we are within the 127 main blocks */
    if( addr <= 0x7effff )
    {
        fa.blkaddr = (addr & 0xffff0000);
        fa.blksize = 0x20000; /** 1Mbit = 128kByte */
        fa.blknum = 130 - (addr>>16);
    }
    /** check if we are at the upper 4 blocks */
    else if( addr <= 0x7fffff)
    {
        fa.blkaddr = (addr & 0xffffc000);
        fa.blksize = 0x8000; /** 256 kbit  = 32kByte*/
        fa.blknum = 3 - ((addr>>14) & 0x03);
    }
    else
    {
        /** invalid flash address */
        return -1;
    }
    *arch = fa;
    return 0;
}



uint32_t
librorc_flash::getBlockAddress
(
    uint32_t addr
)
{
    /** memory organization:
     * block 130: 0x000000 - 0x00ffff
     * block 129: 0x001000 - 0x01ffff
     * ....
     * block 4: 0x7e0000 - 0x7effff
     * block 3: 0x7f0000 - 0x7f3fff
     * block 2: 0x7f4000 - 0x7f7fff
     * block 1: 0x7f8000 - 0x7fbfff
     * block 0: 0x7fc000 - 0x7fffff
     * */
    if(addr <= 0x7effff)
    {
        return (addr & 0xffff0000);
    }
    else
    {
        return (addr & 0xffffc000);
    }
}



uint32_t
librorc_flash::getBankAddress
(
    uint32_t addr
)
{
    /** bank 0: 0x000000 - 0x080000 **/
    return(addr & 0xff800000);
}



int64_t
librorc_flash::dump
(
    char                   *filename,
    librorc_verbosity_enum  verbose
)
{
    uint64_t flash_words = (FLASH_SIZE/2);
    uint16_t *flash_buffer =
        (uint16_t*)calloc(flash_words, sizeof(uint16_t));

    FILE *filep = fopen(filename, "w");
    if(filep == NULL)
    {
        return -1;
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        for(uint64_t i=0; i<flash_words; i++)
        {
            flash_buffer[i] = get(i);
            cout << i << " : "  << hex << setw(4)
                 << flash_buffer[i] << dec << endl;
        }
    }
    else
    {
        for(uint64_t i=0; i<flash_words; i++)
        {
            flash_buffer[i] = get(i);
        }
    }

    size_t bytes_written =
       fwrite(flash_buffer, FLASH_SIZE, 1, filep);
    if(bytes_written != 1)
    {
        return -1;
    }

    fclose(filep);
    free(flash_buffer);

    return 0;
}



int64_t
librorc_flash::erase
(
    int64_t byte_count,
    librorc_verbosity_enum verbose
)
{
    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << "Erasing first " << byte_count << " bytes in the flash." << endl;
    }

    uint32_t current_addr = 0;

    while ( byte_count > 0 )
    {
        struct flash_architecture arch;

        if ( getFlashArchitecture(current_addr, &arch) )
        {
            cout << "Invalid flash address: "<< current_addr << endl;
            return -1;
        }

        if(verbose == LIBRORC_VERBOSE_ON)
        {
            cout << "\rErasing block " << dec << arch.blknum << " (0x"
                 << hex << arch.blkaddr << ")...";
            fflush(stdout);
        }

        if( getBlockLockConfiguration(arch.blkaddr) & 0x01 )
        {
            unlockBlock(arch.blkaddr);
        }

        int ret = eraseBlock(arch.blkaddr);
        if( ret < 0 )
        {
            cout << "Failed to erase block at addr" << hex << current_addr << endl;
            return ret;
        }

        current_addr += (arch.blksize >> 1);
        byte_count -= arch.blksize;

        fflush(stdout);
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << endl << "Erase complete!" << endl;
    }

    return 0;
}



int64_t
librorc_flash::flash
(
    char                   *filename,
    librorc_verbosity_enum  verbose
)
{
    if(filename == NULL)
    {
        cout << "File was not given!" << endl;
        return -1;
    }

    struct stat stat_buf;
    if( stat(filename, &stat_buf) != 0)
    {
        cout << "Flash input file does not exist or is not accessable!"
             << endl;
        return -1;
    }

    if(stat_buf.st_size > FLASH_FILE_SIZE)
    {
        cout << "Flash file is to big!" << endl;
        return -1;
    }

    uint64_t addr = 0;

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << "Bitfile Size         : "
             << (double)(stat_buf.st_size/1024.0/1024.0)
             << " MB (" << dec << stat_buf.st_size
             << " Bytes)" << endl;

        cout << "Bitfile will be written to Flash starting at addr "
             << addr << endl;
    }

    /** Open the flash file */
    int fd = open(filename, O_RDONLY);
    if(fd == -1)
	{
        cout << "failed to open input file "
             << filename << "!"<< endl;
        return -1;
	}

    /** Erase the flash first */
    if( erase(stat_buf.st_size, verbose)!=0 )
    {
        cout << "CRORC flash erase failed!" << endl;
        return -1;
    }

    /** Program flash */
    size_t bytes_read = 0;
    size_t bytes_programmed = 0;
    uint16_t *buffer
        = (uint16_t *)malloc(32*sizeof(uint16_t));

    while ( (bytes_read=read(fd, buffer, 32*sizeof(uint16_t))) > 0 )
    {
        if(verbose == LIBRORC_VERBOSE_ON)
        {
            cout << "\rWriting " << dec << (uint64_t)bytes_read << " bytes to 0x"
                 << hex << addr << " : "
                 << dec << (uint64_t)((100*bytes_programmed)/stat_buf.st_size)
                 << "% ...";
            fflush(stdout);
        }

        int ret = programBuffer(addr, bytes_read/2, buffer);
        if (ret < 0)
        {
            cout << "programBuffer failed, STS: " << hex
                 << ret << dec << endl;
            break;
        }

        for(uint64_t i=0; i<(bytes_read/2); i++)
        {
            uint16_t status = get(addr+i);
            if( buffer[i] != status )
            {
                cout << "write failed: written " << hex << buffer[i]
                     << ", read " << status << ", addr " << hex
                     << (addr+i) << ", bytes_read " << dec << bytes_read
                     << endl;
                break;
            }
        }

        bytes_programmed += bytes_read;
        addr += bytes_read/2;
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << endl << "DONE!" << endl;
    }

    /* Close everything */
	free(buffer);
    close(fd);

    /* TODO : Use dump_device to verify */

    return 0;
}