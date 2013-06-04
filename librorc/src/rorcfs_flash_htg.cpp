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
#include "rorcfs_flash_htg.hh"

using namespace std;

rorcfs_flash_htg::rorcfs_flash_htg
(
    librorc_bar            *flashbar,
    librorc_verbosity_enum  verbose
)
{
    if(flashbar == NULL)
    {
        throw 1;
    }

    bar        = flashbar;
    read_state = 0;

    uint16_t status
        = getStatusRegister(0);

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << "Status               : " << hex
             << setw(4) << status << endl;
    }

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

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << "Status               : " << hex
             << setw(4) << status << endl;
        cout << "Manufacturer Code    : " << hex << setw(4)
             << getManufacturerCode() << endl;
        cout << "Device ID            : " << hex << setw(4)
             << getDeviceID() << endl;
        cout << "Read Config Register : " << hex << setw(4)
             << getReadConfigurationRegister() << endl;
    }

}



rorcfs_flash_htg::~rorcfs_flash_htg()
{
    bar = NULL;
    read_state = 0;
}



void
rorcfs_flash_htg::setReadState
(
    unsigned short state,
    unsigned int   addr
)
{
    bar->set16(addr, state);
    read_state = state;
}



unsigned short
rorcfs_flash_htg::getStatusRegister
(
    unsigned int blkaddr
)
{
    if(read_state != FLASH_READ_STATUS)
    {
        setReadState(FLASH_READ_STATUS, blkaddr);
    }

    return bar->get16(blkaddr);
}



void
rorcfs_flash_htg::clearStatusRegister
(
    unsigned int blkaddr
)
{
    bar->set16(blkaddr, FLASH_CMD_CLR_STS);
    setReadState(FLASH_READ_ARRAY, blkaddr);
}



unsigned short
rorcfs_flash_htg::getManufacturerCode()
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }

    return bar->get16(0x00);
}



unsigned short
rorcfs_flash_htg::getDeviceID()
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }

    return bar->get16(0x01);
}



unsigned short
rorcfs_flash_htg::getBlockLockConfiguration
(
    unsigned int blkaddr
)
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, blkaddr);
    }

    return bar->get16(blkaddr + 0x02);
}



unsigned short
rorcfs_flash_htg::getReadConfigurationRegister()
{
    if(read_state != FLASH_READ_IDENTIFIER)
    {
        setReadState(FLASH_READ_IDENTIFIER, 0x00);
    }

    return bar->get16(0x05);
}



unsigned short
rorcfs_flash_htg::get
(
    unsigned int addr
)
{
    setReadState(FLASH_READ_ARRAY, addr);
    return bar->get16(addr);
}



int
rorcfs_flash_htg::programWord
(
    unsigned int   addr,
    unsigned short data
)
{
    unsigned short status  = 0;
    unsigned int   timeout = CFG_FLASH_TIMEOUT;

    /** word program setup */
    bar->set16(addr, FLASH_CMD_PROG_SETUP);

    bar->set16(addr, data);
    read_state = FLASH_READ_STATUS;
    status = bar->get16(addr);

    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = bar->get16(addr);
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
        return -1;
    }
}



int
rorcfs_flash_htg::programBuffer
(
    unsigned int    addr,
    unsigned short  length,
    unsigned short *data
)
{
    int            i;
    unsigned short status;
    unsigned int   timeout = CFG_FLASH_TIMEOUT;
    unsigned int   blkaddr = addr & CFG_FLASH_BLKMASK;

    /** check if block is locked */
    if( getBlockLockConfiguration(blkaddr) )
    {
        unlockBlock(blkaddr);
    }

    /** write to buffer command */
    bar->set16(blkaddr, FLASH_CMD_BUFFER_PROG);
    read_state = FLASH_READ_STATUS;

    /** read status register */
    status = bar->get16(blkaddr);
    if( (status & FLASH_PEC_BUSY) == 0)                                           //
    {
        return -1;
    }

    /** write word count */
    bar->set16(blkaddr, length - 1);

    bar->set16(addr, data[0]);

    for(i=1; i < length; i++)
    {
        bar->set16(addr + i, data[i]);
    }

    bar->set16(blkaddr, FLASH_CMD_CONFIRM);
    status = bar->get16(blkaddr);

    /** Wait while device is busy */
    while( !(status & FLASH_PEC_BUSY) )
    {
        usleep(100);
        status = bar->get16(blkaddr);
        timeout--;
        if(timeout == 0)
        {
            cout << "programBuffer timed out " << status << endl;
            return -1;
        }
    }

    /** SR.5 or SR.4 nonzero -> program/erase/sequence error */
    if(status != FLASH_PEC_BUSY)
    {
        cout << "programBuffer failed: ";
        switch(status & 0x0030)
        {
            case 0x0030:
            {
                cout << "Command Sequence Error - command aborted!" << endl;
            }
            break;

            case 0x0010:
            {
                cout << "Program Error - operation aborted!" << endl;
                if(status & 0x0002)
                {
                    cout << "Block locked during program!" << endl;
                }
            }
            break;

            case 0x0020:
            {
                cout << "Erase Error - operation aborted!" << endl;
            }
            break;

            default:
            {
                cout << "Unknown Error - Software Bug!" << endl;
            }
            break;
        }
        return -1;
    }

    /** return to ReadArry mode */
    setReadState(FLASH_READ_ARRAY, blkaddr);
    return 0;
}



int
rorcfs_flash_htg::eraseBlock
(
    unsigned int blkaddr
)
{
    unsigned short status;
    unsigned int   timeout = CFG_FLASH_TIMEOUT;

    /** erase command */
    bar->set16(blkaddr, FLASH_CMD_BLOCK_ERASE_SETUP);

    /** confim command */
    bar->set16(blkaddr, FLASH_CMD_CONFIRM);
    read_state = FLASH_READ_STATUS;
    status = bar->get16(blkaddr);

    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = bar->get16(blkaddr);
        timeout--;
        if(timeout == 0)
        {
            cout << "eraseBlock timeout" << endl;
            return -1;
        }
    }

    if(status == FLASH_PEC_BUSY)
    {
        clearStatusRegister(blkaddr);
        return 0;
    }
    else
    {
        return -1;
    }
}



void
rorcfs_flash_htg::programSuspend
(
    unsigned int blkaddr
)
{
    bar->set16(blkaddr, FLASH_CMD_PROG_ERASE_SUSPEND);
}



void
rorcfs_flash_htg::programResume
(
    unsigned int blkaddr
)
{
    bar->set16(blkaddr, FLASH_CMD_CONFIRM);
}



void
rorcfs_flash_htg::lockBlock
(
    unsigned int blkaddr
)
{
    bar->set16(blkaddr, FLASH_CMD_BLOCK_LOCK_SETUP);
    bar->set16(blkaddr, FLASH_CMD_BLOCK_LOCK_CONFIRM);
}



void
rorcfs_flash_htg::unlockBlock
(
    unsigned int blkaddr
)
{
    bar->set16(blkaddr, FLASH_CMD_BLOCK_LOCK_SETUP);
    bar->set16(blkaddr, FLASH_CMD_CONFIRM);
}



void
rorcfs_flash_htg::setConfigReg
(
    unsigned short value
)
{
    bar->set16(value, FLASH_CMD_BLOCK_LOCK_SETUP);
    bar->set16(value, FLASH_CMD_CFG_REG_CONFIRM);
}



int
rorcfs_flash_htg::blankCheck
(
    unsigned int blkaddr
)
{
    unsigned short status;
    unsigned int   timeout = CFG_FLASH_TIMEOUT;

    bar->set16(blkaddr, FLASH_CMD_BC_SETUP);
    bar->set16(blkaddr, FLASH_CMD_BC_CONFIRM);
    read_state = FLASH_READ_STATUS;
    status = bar->get16(blkaddr);

    /** wait for blankCheck to complete */
    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = bar->get16(blkaddr);
        timeout--;
        if(timeout == 0)
        {
            cout << "blankCheck timeout: " << status << endl;
            return -1;
        }
    }
    clearStatusRegister(blkaddr);
    cout << "Blank Check addr " <<  hex << blkaddr
         << "=" << hex << status;

    /**
     * SR5==1: block not empty -> return 0
     * SR5==0: block empty -> return 1
     **/
    return ~(status >> 5) & 0x01;
}



unsigned int
rorcfs_flash_htg::getBlockAddress
(
    unsigned int addr
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



unsigned int
rorcfs_flash_htg::getBankAddress
(
    unsigned int addr
)
{
    /** bank 0: 0x000000 - 0x080000 **/
    return(addr & 0xff800000);
}



int64_t
rorcfs_flash_htg::dump
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
rorcfs_flash_htg::erase
(
    librorc_verbosity_enum verbose
)
{
    unsigned int addr = (1<<23); //start address: +16MB
    int block_count   = (unsigned int)((16<<20)>>17);
    for(uint64_t i=(addr>>16); i<((addr>>16)+block_count); i++)
    {
        uint64_t current_addr = (i<<16);
        if(verbose == LIBRORC_VERBOSE_ON)
        {
            cout << "\rErasing block " << dec << i << " ("
                 << hex << current_addr << ")...";
            fflush(stdout);
        }

        if( getBlockLockConfiguration(current_addr) & 0x01 )
        {
            unlockBlock(current_addr);
        }

        if( eraseBlock(current_addr)<0 )
        {
            return -1;
        }

        fflush(stdout);
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << endl << "Erase complete!" << endl;
    }

    return 0;
}



int64_t
rorcfs_flash_htg::flash
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

    /** Prequesits flash
     *  start address: +16MB
     **/
    uint64_t addr = (1<<23);
    uint64_t block_count
        = (unsigned int)(stat_buf.st_size>>17)+1;

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << "Bitfile Size         : "
             << (double)(stat_buf.st_size/1024.0/1024.0)
             << " MB (" << dec << stat_buf.st_size
             << " Bytes)" << endl;

        cout << "Bitfile will be written to Flash starting at addr "
             << addr << endl;

        cout << "Using " << (uint64_t)(block_count) << " Blocks ("
             << (uint64_t)(addr>>16) << " to "
             << (uint64_t)((addr>>16)+block_count-1) << ")" << endl;
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
    if( erase(verbose)!=0 )
    {
        cout << "CRORC flash erase failed!" << endl;
        return -1;
    }

    /** Program flash */
    size_t bytes_read = 0;
    size_t bytes_programmed = 0;
    uint16_t *buffer
        = (uint16_t *)malloc(32*sizeof(uint16_t));

    while ( (bytes_read=read(fd, buffer, 32*sizeof(unsigned short))) > 0 )
    {
        if(verbose == LIBRORC_VERBOSE_ON)
        {
            cout << "\rWriting " << dec << (uint64_t)bytes_read << " bytes to "
                 << (uint64_t)addr << " (" << hex << addr << ") : "
                 << dec << (uint64_t)((100*bytes_programmed)/stat_buf.st_size)
                 << "% ...";
            fflush(stdout);
        }

        if ( programBuffer(addr, bytes_read/2, buffer) < 0 )
        {
            cout << "programBuffer failed, STS: " << hex
                 << getStatusRegister(addr) << dec << endl;
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
