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

#ifndef _RORCLIB_RORCFS_FLASH_HTG
#define _RORCLIB_RORCFS_FLASH_HTG

#include "librorc_defines.hh"

#define FLASH_SIZE 16777216
#define FLASH_FILE_SIZE 16777216


//TODO : refine this into an enum ...
/** Read array mode **/
#define FLASH_READ_ARRAY 0x00ff

/** Read identifier mode **/
#define FLASH_READ_IDENTIFIER 0x0090

/** Read status mode **/
#define FLASH_READ_STATUS 0x0070

/** Read CFI mode **/
#define FLASH_READ_CFI 0x0098

/** Timeout Value for waiting for SR7=1: 10 seconds **/
#define CFG_FLASH_TIMEOUT 0x0010000

/** bit mask for flash memory blocks **/
#define CFG_FLASH_BLKMASK 0xffff0000

/** Block erase setup **/
#define FLASH_CMD_BLOCK_ERASE_SETUP 0x0020

/** confirm **/
#define FLASH_CMD_CONFIRM 0x00d0

/** program/erase/suspend **/
#define FLASH_CMD_PROG_ERASE_SUSPEND 0x00b0

/** block lock setup **/
#define FLASH_CMD_BLOCK_LOCK_SETUP 0x0060

/** block lock confirm **/
#define FLASH_CMD_BLOCK_LOCK_CONFIRM 0x0001

/** buffer progam **/
#define FLASH_CMD_BUFFER_PROG 0x00e8

/** program setup **/
#define FLASH_CMD_PROG_SETUP 0x0040

/** configuration confirm **/
#define FLASH_CMD_CFG_REG_CONFIRM 0x0003

/** clear status **/
#define FLASH_CMD_CLR_STS 0x0050

/** BC setup **/
#define FLASH_CMD_BC_SETUP 0x00bc

/** BC setup **/
#define FLASH_CMD_BC_CONFIRM 0x00cb

/** flash busy flag mask **/
#define FLASH_PEC_BUSY 1 << 7

/** address bit 23 selects flash chip **/
#define FLASH_CHIP_SELECT_BIT 23


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
class librorc_flash
{
public:

/**
 * constructor
 * @param flashbar librorc_bar instance representing the flash
 * @param chip_select flash chip select (0 or 1)
 * @param verbose verbose level
 * memory
 **/
    librorc_flash
    (
        librorc_bar            *flashbar,
        uint64_t                chip_select,
        librorc_verbosity_enum  verbose
    );

/**
 * deconstructor
 **/
    ~librorc_flash();

/**
 * set read state
 * @param state one of FLASH_READ_ARRAY, FLASH_READ_IDENTIFIER,
 * FLASH_READ_STATUS, FLASH_READ_CFI
 * @param addr address
 **/
    void
    setReadState
    (
        uint16_t state,
        uint32_t addr
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
 * Program single Word to destination address
 * @param addr address to be written to
 * @param data data to be written
 * @return 0 on success, -1 on errors
 **/
    int
    programWord
    (
        uint32_t addr,
        uint16_t data
    );

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
    int
    programBuffer
    (
        uint32_t  addr,
        uint16_t  length,
        uint16_t *data
    );

/**
 * Erase Block
 * @param blkaddr block address
 * @return 0 on sucess, -1 on errors
 **/
    int
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
    void
    lockBlock
    (
        uint32_t blkaddr
    );

/**
 * unlock Block
 **/
    void
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
    int
    blankCheck
    (
        uint32_t blkaddr
    );

/**
 * get Block Address
 * @param addr address
 * @return associated block address
 * */
    uint32_t
    getBlockAddress
    (
        uint32_t addr
    );

/**
 * get Bank Address
 * @param addr address
 * @return associated bank address
 * */
    uint32_t
    getBankAddress
    (
        uint32_t addr
    );

/**
 * Dump flash contents to file
 * @param filename destination filename
 * @param verbose verbose level
 * @return -1 on error, 0 on sucess
 * */
    int64_t
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
    int64_t
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
    int64_t
    flash
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
    int64_t
    getFlashArchitecture
    (
        uint32_t                   addr,
        struct flash_architecture *arch
    );

private:
    librorc_bar    *bar;
    uint16_t    read_state;
    uint32_t    base_addr;
};

#endif