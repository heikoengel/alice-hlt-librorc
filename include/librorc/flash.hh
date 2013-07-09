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

#ifndef LIBRORC_FLASH_H
#define LIBRORC_FLASH_H

#include "librorc/include_ext.hh"
#include "librorc/include_int.hh"

#include "librorc/defines.hh"


#define FLASH_SIZE 16777216
#define FLASH_FILE_SIZE 16777216


//TODO : refine this into an enum ...
/** Read array mode **/
#define FLASH_CMD_READ_ARRAY 0xffff

/** Some Intel CFI chips support 0xF0 command instead of 0xFF as
 * reset/read array command **/
#define FLASH_CMD_RESET 0xf0f0

/** Read identifier mode **/
#define FLASH_CMD_READ_IDENTIFIER 0x9090

/** Read status mode **/
#define FLASH_CMD_READ_STATUS 0x7070

/** Read CFI mode **/
#define FLASH_CMD_READ_CFI 0x9898
/** Block erase setup **/
#define FLASH_CMD_BLOCK_ERASE_SETUP 0x2020

/** confirm **/
#define FLASH_CMD_CONFIRM 0xd0d0

/** program/erase/suspend **/
#define FLASH_CMD_PROG_ERASE_SUSPEND 0xb0b0

/** block lock setup **/
#define FLASH_CMD_BLOCK_LOCK_SETUP 0x6060

/** block lock confirm **/
#define FLASH_CMD_BLOCK_LOCK_CONFIRM 0x0101

/** buffer progam **/
#define FLASH_CMD_BUFFER_PROG 0xe8e8

/** program setup **/
#define FLASH_CMD_PROG_SETUP 0x4040

/** configuration confirm **/
#define FLASH_CMD_CFG_REG_CONFIRM 0x0303

/** clear status **/
#define FLASH_CMD_CLR_STS 0x5050

/** BC setup **/
#define FLASH_CMD_BC_SETUP 0xbcbc

/** BC setup **/
#define FLASH_CMD_BC_CONFIRM 0xcbcb


/** Timeout Value for waiting for SR7=1: 10 seconds **/
#define CFG_FLASH_TIMEOUT 0x0010000


/** flash busy flag mask **/
#define FLASH_PEC_BUSY 1 << 7

/** address bit 23 selects flash chip **/
#define FLASH_CHIP_SELECT_BIT 23

//TODO: check this for packed and volatile
struct flash_architecture
{
    uint32_t blkaddr;
    uint32_t blksize;
    uint32_t blknum;
    uint32_t bankaddr;
    uint32_t banksize;
    uint32_t banknum;
};


/**
 * @class librorc_flash
 * @brief interface class to the StrataFlash Embedded
 * Memory P30-65nm on the HTG board
 **/
namespace librorc
{

    class flash
    {
    public:

    /**
     * constructor
     * @param flashbar bar instance representing the flash
     * @param chip_select flash chip select (0 or 1)
     * @param verbose verbose level
     * memory
     **/
        flash
        (
            bar                    *flashbar,
            uint64_t                chip_select,
            librorc_verbosity_enum  verbose
        );

    /**
     * deconstructor
     **/
        ~flash();

    /**
     * set read state
     * @param cmd command to be sent
     * @param addr address
     **/
        void
        sendCommand
        (
            uint32_t addr,
            uint16_t cmd
        );

    /**
     * read flash status register
     * @param blkaddr block address
     * @return status register
     **/
        uint16_t
        getStatusRegister
        (
            uint32_t blkaddr
        );

    /**
     * read flash status register
     * @param blkaddr block address
     **/
        void
        clearStatusRegister
        (
            uint32_t blkaddr
        );

    /**
     * get Manufacturer Code
     * @return manufacturer code
     **/
        uint16_t
        getManufacturerCode();

    /**
     * get Device ID
     * @return Device ID
     **/
        uint16_t
        getDeviceID();

    /**
     * get Block Lock Configuration
     * @param blkaddr block address
     * @return block lock configuration: 0=unlocked,
     * 1=locked but not locked down, 3=locked and locked down
     **/
        uint16_t
        getBlockLockConfiguration
        (
            uint32_t blkaddr
        );

    /**
     * get Read Configuration Register (RCR)
     * @return Read Configuraion Register
     **/
        uint16_t
        getReadConfigurationRegister();

    /**
     * get Unique Device Number
     * @return 64bit device number
    **/
        uint64_t
        getUniqueDeviceNumber();

    /**
     * Reset Block:
     * Clear Status, Read Status, set Read Array mode
     * @param blkaddr block address
     * @return status register
     **/
        uint16_t resetBlock
        (
            uint32_t blkaddr
        );

    /**
     * Reset Chip:
     * iterate over all blocks
     * @return 0 on sucess, status register on error
     **/
        uint16_t resetChip ();


    /** get WORD from flash
     * @param addr address
     * @return data word at specified address
     **/
        uint16_t
        get
        (
            uint32_t addr
        );

    /**
     * Buffer Program Mode
     * @param addr start address
     * @param length number of WORDs to be written
     * @param data pointer to data buffer
     * @return 0 on sucess, -1 on errors
     **/
        int32_t
        programBuffer
        (
            uint32_t  addr,
            uint16_t  length,
            uint16_t *data,
            librorc_verbosity_enum verbose
        );

    /**
     * Erase Block
     * @param blkaddr block address
     * @return 0 on sucess, -1 on errors
     **/
        int32_t
        eraseBlock
        (
            uint32_t blkaddr
        );

    /**
     * programSuspend
     * @param blkaddr block address
     **/
        void
        programSuspend
        (
            uint32_t blkaddr
        );

    /**
     * programResume
     * @param blkaddr block address
     **/
        void
        programResume
        (
            uint32_t blkaddr
        );

    /**
     * Lock Block
     **/
        int32_t
        lockBlock
        (
            uint32_t blkaddr
        );

    /**
     * unlock Block
     **/
        int32_t
        unlockBlock
        (
            uint32_t blkaddr
        );

    /**
     * set Configuration Register
     * */
        void
        setConfigReg
        (
            uint32_t value
        );

    /**
     * check if block is empty
     * NOTE: this will only work if VPP=VPPH (~8V)
     * this is not supported on C-RORC
     * @param blkaddr block address
     * @return -1 on error, 0 on empty, 1 on not empty
     * */
        int32_t
        blankCheck
        (
            uint32_t blkaddr
        );


    /**
     * Dump flash contents to file
     * @param filename destination filename
     * @param verbose verbose level
     * @return -1 on error, 0 on sucess
     * */
        int32_t
        dump
        (
            char                   *filename,
            librorc_verbosity_enum  verbose
        );

    /**
     * erase flash
     * @param verbose verbose level
     * @return -status on error, 0 on success
     * */
        int32_t
        erase
        (
            int64_t                byte_count,
            librorc_verbosity_enum verbose
        );

    /**
     * program flash with binary file
     * @param filename source file
     * @param verbose verbose level
     * @return -1 on error, 0 on success
     * */
        int32_t
        flashWrite
        (
            char                   *filename,
            librorc_verbosity_enum  verbose
        );

    /**
     * get Flash Architecture
     * @param flash address
     * @param pointer to destination struct
     * @return 0 on success, -1 on invalid address
     * */
        int32_t
        getFlashArchitecture
        (
            uint32_t                   addr,
            struct flash_architecture *arch
        );

    private:
        bar      *m_bar;
        uint32_t  m_base_addr;
    };

}
#endif /** LIBRORC_FLASH_H */
