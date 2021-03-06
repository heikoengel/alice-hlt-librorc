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

#ifndef LIBRORC_FLASH_H
#define LIBRORC_FLASH_H

#include <librorc/defines.hh>


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


/** FPGA synchronization word in a bit/binfile */
#define FPGA_SYNCWORD 0x5599aa66

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



namespace LIBRARY_NAME
{
    class bar;

    /**
     * @brief Interface class to the onboard flash memories
     **/
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
            uint32_t                chip_select
        );

    /**
     * deconstructor
     **/
        ~flash();

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

        /**
         * get current chip select
         * @return 0 for flash0, 1 for flash1
         **/
        uint32_t
        getChipSelect();

        /**
         * search for FPGA synchronization word in the first
         * bytes of the flash content starting from an offset.
         * The synchronization word is an indication for a
         * valid FPGA configuration in the flash partition
         * @param startOffset byte offset in the flash to start searching
         * @param searchLength maximum number of bytes to analyze
         * @return -1 if no synchronization word was found, else the byte
         * offset of the beginning of the synchronization word
         **/
        int32_t
        findFpgaSyncWord
        (
            uint32_t startOffset,
            uint32_t searchLength
        );

        /**
         * set flash to asynchronous read mode. After power-on the flashes
         * are in a synchronous read mode for automatic FPGA configuration.
         * In order to read from or write to the flashes from software they
         * have to be set in asynchronous read mode. If the flash is already
         * in asynchronous read mode, this method has no effect.
         **/
        void
        setAsynchronousReadMode();

    private:
        bar      *m_bar;
        uint32_t  m_base_addr;
        uint32_t  m_chip_select;


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
     * set Configuration Register
     * */
        void
        setConfigReg
        (
            uint32_t value
        );

    };

}
#endif /** LIBRORC_FLASH_H */
