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

#include <librorc/flash.hh>
#include <librorc/bar_proto.hh>
#include <librorc/sim_bar.hh>
#include <librorc/bar.hh>

namespace librorc
{

flash::flash
(
    bar                    *flashbar,
    uint64_t                chip_select,
    librorc_verbosity_enum  verbose
)
{
    if(flashbar == NULL)
    {
        throw 1;
    }

    if(chip_select > 1)
    {
        throw 2;
    }

    m_bar           = flashbar;
    m_base_addr     = chip_select << FLASH_CHIP_SELECT_BIT;
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

    /** get code */
    uint16_t code = get(0x00);

    /** return into read array mode */
    resetBlock(0);

    return code;
}



uint16_t
flash::getDeviceID()
{
    /** send read id code command */
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);

    /** get ID */
    uint16_t id = get(0x01);

    /** return into read array mode */
    resetBlock(0);

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

    /** return into read array mode */
    resetBlock(0);

    return cfg;
}



uint16_t
flash::getReadConfigurationRegister()
{
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);
    uint16_t rcr = get(0x05);

    /** return into read array mode */
    resetBlock(0);

    return rcr;
}



uint64_t
flash::getUniqueDeviceNumber()
{
    uint64_t udn = 0;
    sendCommand(0x00, FLASH_CMD_READ_IDENTIFIER);
    for ( int i=0; i<4; i++ )
    {
        udn += ( get(0x81 + i) )<<(16*i);
    }

    /** return into read array mode */
    resetBlock(0);

    return udn;
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
    /** Send Clear Status Command **/
    clearStatusRegister(blkaddr);

    /** Sample the Status **/
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

    /** iterate over all blocks */
    while( getFlashArchitecture(blkaddr, &arch) == 0 )
    {
        uint16_t status = resetBlock(blkaddr);
        if ( !(status & FLASH_PEC_BUSY) )
        {
            cout << "resetBlock failed, Block "
                 << arch.blknum << " is busy." << endl;
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

    /** get current block address */
    struct flash_architecture arch;
    if ( getFlashArchitecture(addr, &arch) )
    {
        cout << "Invalid flash address: "<< hex << addr << endl;
        return -1;
    }

    /** check if block is locked */
    if( getBlockLockConfiguration(arch.blkaddr) )
    {
        /** try to unlock block */
        int32_t ret = unlockBlock(arch.blkaddr) ;
        if ( ret < 0 )
        {
            cout << "Failed to unlock block at addr" << hex
                 << arch.blkaddr << endl;
            return ret;
        }
    }

    /** send write buffer command */
    sendCommand(addr, FLASH_CMD_BUFFER_PROG);
    /** read Status register */
    uint16_t status = get(addr);

    while( !(status & FLASH_PEC_BUSY) )
    {
        usleep(100);
        /** read Status register */
        status = get(addr);
        timeout--;
        if(timeout == 0)
        {
            if ( verbose==LIBRORC_VERBOSE_ON )
            {
                cout << "programBuffer: Timeout waiting for buffer!"
                     << endl;
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

    /** send write confirm command */
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
            if ( verbose==LIBRORC_VERBOSE_ON )
            {
                cout << "programBuffer: Timeout waiting for busy!" << endl;
            }
            return -status;
        }
    }

    /** SR.5 or SR.4 nonzero -> program/erase/sequence error */
    if(status != FLASH_PEC_BUSY)
    {
        if ( verbose==LIBRORC_VERBOSE_ON )
        {
            cout << "programBuffer: program/erase sequence error: "
                 << hex << status << endl;
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

    /** erase command */
    sendCommand(blkaddr, FLASH_CMD_BLOCK_ERASE_SETUP);

    /** confim command */
    sendCommand(blkaddr, FLASH_CMD_CONFIRM);

    status = get(blkaddr);
    while( (status & FLASH_PEC_BUSY) == 0 || status==0xffff)
    {
        usleep(100);
        status = get(blkaddr);
        timeout--;
        if(timeout == 0)
        {
            return -status;
        }
    }

    /** return into read array mode */
    resetBlock(blkaddr);

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
        {
            return -status;
        }
    }
    clearStatusRegister(blkaddr);

    /** return into read array mode */
    resetBlock(blkaddr);

    return 0;
}



int32_t
flash::unlockBlock
(
    uint32_t blkaddr
)
{
    uint32_t timeout = CFG_FLASH_TIMEOUT;

    /** clear any latched status register contents */
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
        {
            return -status;
        }
    }
    clearStatusRegister(blkaddr);

    /** return into read array mode */
    resetBlock(blkaddr);

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
    else
    {
        /** invalid flash address */
        return -1;
    }
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



int32_t
flash::erase
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
    int32_t  status;

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

        /** check if block is locked */
        if( getBlockLockConfiguration(arch.blkaddr) )
        {
            /** try to unlock block */
            status = unlockBlock(arch.blkaddr) ;
            if ( status < 0 )
            {
                cout << "Failed to unlock block at addr" << hex
                     << arch.blkaddr << endl;
                return status;
            }
        }

        /** erase block */
        status = eraseBlock(arch.blkaddr);
        if( status < 0 )
        {
            cout << "Failed to erase block at addr" << hex
                 << arch.blkaddr << ": " << -status << endl;
            return status;
        }

        current_addr += (arch.blksize >> 1);
        byte_count -= arch.blksize;

        fflush(stdout);
    }

    if(verbose == LIBRORC_VERBOSE_ON)
    {
        cout << "Erase complete!" << endl;
    }

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
    int32_t fd = open(filename, O_RDONLY);
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

        int32_t ret = programBuffer(addr, bytes_read/2, buffer, verbose);
        if (ret < 0)
        {
            cout << "programBuffer failed, STS: " << hex
                 << -ret << dec << endl;
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

}

