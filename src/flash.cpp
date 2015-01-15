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

#include <librorc/flash.hh>
#include <librorc/bar.hh>

namespace LIBRARY_NAME
{

flash::flash
(
    bar                    *flashbar,
    uint32_t                chip_select
)
{
    if(flashbar == NULL)
    { throw 1; }

    if(chip_select > 1)
    { throw 2; }

    m_bar           = flashbar;
    m_base_addr     = chip_select << FLASH_CHIP_SELECT_BIT;
    m_chip_select   = chip_select;
}



flash::~flash()
{
    m_bar = NULL;
}



void
flash::sendCommand
(
    uint32_t addr,
    uint16_t cmd
)
{
    m_bar->set16(m_base_addr + addr, cmd);
}



uint16_t
flash::getStatusRegister
(
    uint32_t blkaddr
)
{
    sendCommand(blkaddr, FLASH_CMD_READ_STATUS);
    return get(blkaddr);
}



void
flash::clearStatusRegister
(
    uint32_t blkaddr
)
{
    sendCommand(blkaddr, FLASH_CMD_CLR_STS);
}



uint16_t
flash::getManufacturerCode()
{
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);
    uint16_t code = get(0x00);
    resetBlock(0); /** return into read array mode */

    return code;
}



uint16_t
flash::getDeviceID()
{
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);
    uint16_t id = get(0x01);
    resetBlock(0); /** return into read array mode */

    return id;
}



uint16_t
flash::getBlockLockConfiguration
(
    uint32_t blkaddr
)
{
    sendCommand(blkaddr, FLASH_CMD_READ_IDENTIFIER);
    uint16_t cfg = get(blkaddr + 0x02);
    resetBlock(0); /** return into read array mode */

    return cfg;
}



uint16_t
flash::getReadConfigurationRegister()
{
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);
    uint16_t rcr = get(0x05);
    resetBlock(0); /** return into read array mode */

    return rcr;
}



uint64_t
flash::getUniqueDeviceNumber()
{
    uint64_t unique_device_number = 0;
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);
    for ( int i=0; i<4; i++ )
    { unique_device_number += ( get(0x81 + i) )<<(16*i); }
    resetBlock(0); /** return into read array mode */

    return unique_device_number;
}



uint16_t
flash::get
(
    uint32_t addr
)
{
    return m_bar->get16(m_base_addr + addr);
}



uint16_t
flash::resetBlock
(
    uint32_t blkaddr
)
{
    clearStatusRegister(blkaddr);
    uint16_t status = getStatusRegister(blkaddr);
    sendCommand(blkaddr, FLASH_CMD_RESET);
    sendCommand(blkaddr, FLASH_CMD_READ_ARRAY);

    return status;
}



uint16_t
flash::resetChip()
{
    uint32_t blkaddr = 0;
    struct flash_architecture arch;

    /** Iterate over all blocks */
    while( getFlashArchitecture(blkaddr, &arch) == 0 )
    {
        uint16_t status = resetBlock(blkaddr);
        if ( !(status & FLASH_PEC_BUSY) )
        {
            std::cout << "resetBlock failed, Block "
                 << arch.blknum << " is busy." << std::endl;
            return status;
        }
        blkaddr += (arch.blksize >> 1);
    }
    return 0;
}



int32_t
flash::programBuffer
(
    uint32_t  addr,
    uint16_t  length,
    uint16_t *data,
    librorc_verbosity_enum verbose
)
{
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    /** Get current block address */
    struct flash_architecture arch;
    if ( getFlashArchitecture(addr, &arch) )
    {
        std::cout << "Invalid flash address: "
             << std::hex << addr << std::endl;
        return -1;
    }

    /** Check if block is locked */
    if( getBlockLockConfiguration(arch.blkaddr) )
    {
        int32_t ret = unlockBlock(arch.blkaddr) ;
        if ( ret < 0 )
        {
            std::cout << "Failed to unlock block at addr" << std::hex
                 << arch.blkaddr << std::endl;
            return ret;
        }
    }

    sendCommand(addr, FLASH_CMD_BUFFER_PROG);
    uint16_t status = get(addr);

    while( !(status & FLASH_PEC_BUSY) )
    {
        usleep(100);
        status = get(addr);
        timeout--;
        if(timeout == 0)
        {
            if( verbose==LIBRORC_VERBOSE_ON )
            {
                std::cout << "programBuffer: Timeout waiting for buffer!"
                     << std::endl;
            }
            return -status;
        }
    }

    /** write word count */
    m_bar->set16(m_base_addr + addr, length - 1);
    for(int32_t i=0; i < length; i++)
    {
        m_bar->set16(m_base_addr + addr + i, data[i]);
    }

    sendCommand(arch.blkaddr, FLASH_CMD_CONFIRM);

    /** Wait while device is busy */
    timeout = CFG_FLASH_TIMEOUT;
    status = get(arch.blkaddr);
    while( (status & FLASH_PEC_BUSY) == 0 )
    {
        usleep(100);
        status = get(arch.blkaddr);
        timeout--;
        if(timeout == 0)
        {
            if( verbose==LIBRORC_VERBOSE_ON )
            {
                std::cout << "programBuffer: Timeout waiting for busy!"
                    << std::endl;
            }
            return -status;
        }
    }

    /** SR.5 or SR.4 nonzero -> program/erase/sequence error */
    if(status != FLASH_PEC_BUSY)
    {
        if( verbose==LIBRORC_VERBOSE_ON )
        {
            std::cout << "programBuffer: program/erase sequence error: "
                 << std::hex << status << std::endl;
        }
        return -status;
    }

    /** return to read array mode */
    resetBlock(arch.blkaddr);

    return 0;
}



int32_t
flash::eraseBlock
(
    uint32_t blkaddr
)
{
    uint16_t status;
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    sendCommand(blkaddr, FLASH_CMD_BLOCK_ERASE_SETUP);
    sendCommand(blkaddr, FLASH_CMD_CONFIRM);

    status = get(blkaddr);
    while( (status & FLASH_PEC_BUSY) == 0 || status==0xffff)
    {
        usleep(100);
        status = get(blkaddr);
        timeout--;
        if(timeout == 0)
        { return -status; }
    }

    resetBlock(blkaddr); /** return into read array mode */

    return 0;
}



void
flash::programSuspend
(
    uint32_t blkaddr
)
{
    sendCommand(blkaddr, FLASH_CMD_PROG_ERASE_SUSPEND);
}



void
flash::programResume
(
    uint32_t blkaddr
)
{
    sendCommand(blkaddr, FLASH_CMD_CONFIRM);
}



int32_t
flash::lockBlock
(
    uint32_t blkaddr
)
{
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    /** clear any latched status register contents */
    clearStatusRegister(blkaddr);

    sendCommand(blkaddr, FLASH_CMD_BLOCK_LOCK_SETUP);
    sendCommand(blkaddr, FLASH_CMD_BLOCK_LOCK_CONFIRM);

    /** wait for unlock to complete */
    uint16_t status = getStatusRegister(blkaddr);
    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = getStatusRegister(blkaddr);
        timeout--;
        if(timeout == 0)
        { return -status; }
    }
    clearStatusRegister(blkaddr);
    resetBlock(blkaddr); /** return into read array mode */

    return 0;
}



int32_t
flash::unlockBlock
(
    uint32_t blkaddr
)
{
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    clearStatusRegister(blkaddr);

    /** unlock block */
    sendCommand(blkaddr, FLASH_CMD_BLOCK_LOCK_SETUP);
    sendCommand(blkaddr, FLASH_CMD_CONFIRM);

    /** wait for unlock to complete */
    uint16_t status = getStatusRegister(blkaddr);
    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = getStatusRegister(blkaddr);
        timeout--;
        if(timeout == 0)
        { return -status; }
    }
    clearStatusRegister(blkaddr);
    resetBlock(blkaddr); /** return into read array mode */

    return 0;
}



void
flash::setConfigReg
(
    uint32_t value
)
{
    m_bar->set16(m_base_addr + value, FLASH_CMD_BLOCK_LOCK_SETUP);
    m_bar->set16(m_base_addr + value, FLASH_CMD_CFG_REG_CONFIRM);
}



int32_t
flash::blankCheck
(
    uint32_t blkaddr
)
{
    uint16_t status;
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    sendCommand(blkaddr, FLASH_CMD_BC_SETUP);
    sendCommand(blkaddr, FLASH_CMD_BC_CONFIRM);
    status = get(blkaddr);

    /** wait for blankCheck to complete */
    while( (status & FLASH_PEC_BUSY) == 0)
    {
        usleep(100);
        status = get(blkaddr);
        timeout--;
        if(timeout == 0)
        { return -status; }
    }
    clearStatusRegister(blkaddr);

    /**
     * SR5==1: block not empty -> return 0
     * SR5==0: block empty -> return 1
     **/
    return ~(status >> 5) & 0x01;
}



int32_t
flash::getFlashArchitecture
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
    else /** invalid flash address */
    { return -1; }

    *arch = fa;
    return 0;
}



int32_t
flash::dump
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
    { return -1; }

    for(uint64_t i=0; i<flash_words; i++)
    {
        flash_buffer[i] = get(i);
        if(verbose == LIBRORC_VERBOSE_ON)
        {
            std::cout << i << " : "  << std::hex << std::setw(4)
                 << flash_buffer[i] << std::dec << std::endl;
        }
    }

    if( fwrite(flash_buffer, FLASH_SIZE, 1, filep) != 1 )
    { return -1; }

    fclose(filep);
    free(flash_buffer);

    return 0;
}



int32_t
flash::erase
(
    int64_t byte_count,
    librorc_verbosity_enum verbose
)
{
    if(verbose == LIBRORC_VERBOSE_ON)
    {
        std::cout << "Erasing first " << byte_count
             << " bytes in the flash." << std::endl;
    }

    uint32_t current_addr = 0;
    int32_t  status;

    while ( byte_count > 0 )
    {
        struct flash_architecture arch;

        if ( getFlashArchitecture(current_addr, &arch) )
        {
            std::cout << "Invalid flash address: "
                 << current_addr << std::endl;
            return -1;
        }

        if(verbose == LIBRORC_VERBOSE_ON)
        {
            std::cout << "\rErasing block "
                 << std::dec << arch.blknum << " (0x"
                 << std::hex << arch.blkaddr << ")...";
            fflush(stdout);
        }

        /** check if block is locked */
        if( getBlockLockConfiguration(arch.blkaddr) )
        {
            /** try to unlock block */
            status = unlockBlock(arch.blkaddr) ;
            if ( status < 0 )
            {
                std::cout << "Failed to unlock block at addr" << std::hex
                     << arch.blkaddr << std::endl;
                return status;
            }
        }

        /** erase block */
        status = eraseBlock(arch.blkaddr);
        if( status < 0 )
        {
            std::cout << "Failed to erase block at addr" << std::hex
                 << arch.blkaddr << ": " << -status << std::endl;
            return status;
        }

        current_addr += (arch.blksize >> 1);
        byte_count -= arch.blksize;

        fflush(stdout);
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    { std::cout << "Erase complete!" << std::endl; }

    return 0;
}



int32_t
flash::flashWrite
(
    char                   *filename,
    librorc_verbosity_enum  verbose
)
{
    if(filename == NULL)
    {
        std::cout << "File was not given!" << std::endl;
        return -1;
    }

    struct stat stat_buf;
    if( stat(filename, &stat_buf) != 0)
    {
        std::cout << "Flash input file does not exist or is not accessable!"
             << std::endl;
        return -1;
    }

    if(stat_buf.st_size > FLASH_FILE_SIZE)
    {
        std::cout << "Flash file is to big!" << std::endl;
        return -1;
    }

    uint64_t addr = 0;

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        std::cout << "Bitfile Size         : "
             << (double)(stat_buf.st_size/1024.0/1024.0)
             << " MB (" << std::dec << stat_buf.st_size
             << " Bytes)" << std::endl;

        std::cout << "Bitfile will be written to Flash starting at addr "
             << addr << std::endl;
    }

    /** Open the flash file */
    int32_t fd = open(filename, O_RDONLY);
    if(fd == -1)
	{
        std::cout << "failed to open input file "
             << filename << "!"<< std::endl;
        return -1;
	}

    /** Erase the flash first */
    if( erase(stat_buf.st_size, verbose)!=0 )
    {
        std::cout << "CRORC flash erase failed!" << std::endl;
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
            std::cout << "\rWriting " << std::dec
                 << (uint64_t)bytes_read << " bytes to 0x"
                 << std::hex << addr << " : "
                 << std::dec
                 << (uint64_t)((100*bytes_programmed)/stat_buf.st_size)
                 << "% ...";
            fflush(stdout);
        }

        int32_t ret = programBuffer(addr, bytes_read/2, buffer, verbose);
        if (ret < 0)
        {
            std::cout << "programBuffer failed, STS: " << std::hex
                 << -ret << std::dec << std::endl;
            break;
        }

        for(uint64_t i=0; i<(bytes_read/2); i++)
        {
            uint16_t status = get(addr+i);
            if( buffer[i] != status )
            {
                std::cout << "write failed: written "
                     << std::hex << buffer[i]
                     << ", read " << status << ", addr " << std::hex
                     << (addr+i) << ", bytes_read "
                     << std::dec << bytes_read << std::endl;
                break;
            }
        }

        bytes_programmed += bytes_read;
        addr += bytes_read/2;
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    { std::cout << std::endl << "DONE!" << std::endl; }

    /* Close everything */
	free(buffer);
    close(fd);

    return 0;
}


uint32_t
flash::getChipSelect()
{
    return m_chip_select;
}


int32_t
flash::findFpgaSyncWord
(
    uint32_t startOffset,
    uint32_t searchLength
)
{
    /** ensure offset and length are even numbers */
    uint32_t offset = startOffset & ~(0x00000001);
    uint32_t end_offest = startOffset + (searchLength & ~(0x00000001));
    uint32_t cur_word = 0;
    while( cur_word!=FPGA_SYNCWORD && offset<end_offest)
    {
        /** 16bit flash interface: 2 bytes per read, so
         * increment offset by 2 */
        cur_word = (cur_word<<16) + get(offset>>1);
        offset += 2;
    }

    if( cur_word==FPGA_SYNCWORD )
    { return (offset-2); }
    else
    { return -1; }
}


void
flash::setAsynchronousReadMode()
{
    setConfigReg(0xbddf);
}

}
