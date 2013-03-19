#ifndef CRORC_FLASH_H
#define CRORC_FLASH_H

/** CRORC status codes **/
#define CRORC_STATUS_READY 0x0080

/** Read array mode **/
#define FLASH_READ_ARRAY 0x00ff
/** Read identifier mode **/
#define FLASH_READ_IDENTIFIER 0x0090
/** Read status mode **/
#define FLASH_READ_STATUS 0x0070
/** Read CFI mode **/
#define FLASH_READ_CFI 0x0098

/** File and flash sizes */
#define FLASH_SIZE 33554432
#define FLASH_FILE_SIZE 16777216

/** Timeout Value for waiting for SR7=1: 10 seconds **/
#define CFG_FLASH_TIMEOUT 0x0010000

/** bit mask for flash memory blocks **/
#define CFG_FLASH_BLKMASK 0xffff0000

typedef struct
{
    uint8_t  *flash;
    uint64_t  flash_size;
    uint16_t  read_state;
}crorc_flash_t;

/* FUNCTIONS */

uint16_t
get_manufacturer_code
(
    crorc_flash_t *flash
);

uint16_t
get_device_id
(
    crorc_flash_t *flash
);

uint16_t
get_read_configuration_register
(
    crorc_flash_t *flash
);

uint16_t
get_status_register
(
    crorc_flash_t *flash
);

void
clear_status_register
(
    crorc_flash_t *flash
);

void
set_read_state
(
    crorc_flash_t *flash,
    uint16_t       state,
    uint64_t       addr
);

uint16_t
get16
(
    crorc_flash_t *flash,
    uint64_t       addr
);

void
set16
(
    crorc_flash_t *flash,
    uint64_t       addr,
    uint16_t       data
);

#define GET( flash, addr ) ((uint16_t*)(flash.flash))[addr]

uint16_t
get
(
    crorc_flash_t *flash,
    uint64_t       addr
);

void
program_suspend
(
    crorc_flash_t *flash
);

void
program_resume
(
    crorc_flash_t *flash
);

uint16_t
get_block_lock_configuration
(
    crorc_flash_t *flash,
    uint64_t       blkaddr
);

void
unlock_block
(
    crorc_flash_t *flash,
    uint64_t       blkaddr
);

int64_t
erase_block
(
    crorc_flash_t *flash,
    uint64_t       blkaddr
);

int64_t
program_buffer
(
    crorc_flash_t *flash,
    uint64_t       addr,
    uint16_t       length,
    uint16_t      *data
);

#endif
