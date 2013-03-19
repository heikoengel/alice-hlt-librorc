#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#include <inttypes.h>
#include <stdio.h>

#include <pda.h>

#include "definitions.h"
#include "crorc_flash.h"

uint16_t
get_manufacturer_code
(
    crorc_flash_t *flash
)
{
	if(flash->read_state!=FLASH_READ_IDENTIFIER)
	{
		set_read_state(flash, FLASH_READ_IDENTIFIER, 0x00);
	}

	return get16(flash, 0x00);
}


uint16_t
get_device_id
(
    crorc_flash_t *flash
)
{
	if(flash->read_state!=FLASH_READ_IDENTIFIER)
	{
		set_read_state(flash, FLASH_READ_IDENTIFIER, 0x00);
	}

	return get16(flash, 0x01);
}


uint16_t
get_read_configuration_register
(
    crorc_flash_t *flash
)
{
	if(flash->read_state!=FLASH_READ_IDENTIFIER)
	{
		set_read_state(flash, FLASH_READ_IDENTIFIER, 0x00);
	}

	return get16(flash, 0x05);
}


uint16_t
get_status_register
(
    crorc_flash_t *flash
)
{
	if (flash->read_state!=FLASH_READ_STATUS)
	{
        set_read_state(flash, FLASH_READ_STATUS, 0x00);
	}

	return get16(flash, 0x00);
}


void
clear_status_register
(
    crorc_flash_t *flash
)
{
    set16(flash, 0, 0x0050);
    flash->read_state = 0;
}


void
set_read_state
(
    crorc_flash_t *flash,
    uint16_t       state,
    uint64_t       addr
)
{
	set16(flash, addr, state);
    flash->read_state = state;
}


uint16_t
get16
(
    crorc_flash_t *flash,
    uint64_t       addr
)
{
    uint16_t *sbar = (uint16_t*)(flash->flash);
    assert(sbar!=NULL);

    if((addr<<1) < flash->flash_size)
    {
        return(sbar[addr]);
    }

    return(0xffff);
}


void
set16
(
    crorc_flash_t *flash,
    uint64_t       addr,
    uint16_t       data
)
{
    uint16_t *sbar = (uint16_t*)(flash->flash);
    assert(sbar!=NULL);

    if((addr<<1) < flash->flash_size)
    {
        sbar[addr] = data;
        msync( (flash->flash + ((addr<<2) & PAGE_MASK)), PAGE_SIZE, MS_SYNC);
    }
}


uint16_t
get
(
    crorc_flash_t *flash,
    uint64_t       addr
)
{
	if(flash->read_state!=FLASH_READ_ARRAY)
	{
		set_read_state(flash, FLASH_READ_ARRAY, 0x00);
	}

	return get16(flash, addr);
}


void
program_suspend
(
    crorc_flash_t *flash
)
{
	set_read_state(flash, FLASH_READ_STATUS, 0);
	set16(flash, 0x00, 0x00b0);
}


void
program_resume
(
    crorc_flash_t *flash
)
{
	set16(flash, 0x00, 0x00d0);
	flash->read_state = 0;
}


uint16_t
get_block_lock_configuration
(
    crorc_flash_t *flash,
    uint64_t       blkaddr
)
{
	if(flash->read_state!=FLASH_READ_IDENTIFIER)
	{
        set_read_state(flash, FLASH_READ_IDENTIFIER, blkaddr);
	}
	return get16(flash, (blkaddr+0x02));
}


void
unlock_block
(
    crorc_flash_t *flash,
    uint64_t       blkaddr
)
{
	set16(flash, blkaddr, 0x60); // block lock setup
	set16(flash, blkaddr, 0xd0); // block unlock
}

int64_t
erase_block
(
    crorc_flash_t *flash,
    uint64_t       blkaddr
)
{
	/* erase command */
	set16(flash, blkaddr, 0x0020);

	/* confim command */
    set16(flash, blkaddr, 0x00d0);
    flash->read_state = FLASH_READ_STATUS;

	uint16_t status
        = get16(flash, blkaddr);
//    printf("erase STS: %04x\n", status);

    uint64_t timeout = CFG_FLASH_TIMEOUT;
    while( (status & 0x0080)==0 )
    {
        usleep(100);
        status = get16(flash, blkaddr);
        timeout--;
        if(timeout==0)
        {
            printf("eraseBlock timeout\n");
            return -1;
		}
    }

//    printf("erase time: %" PRIu64 " x 100us\n",
//           CFG_FLASH_TIMEOUT - timeout);

    if(status == 0x80)
    {
        clear_status_register(flash);
        return 0;
    }

    return -1;
}



int64_t
program_buffer
(
    crorc_flash_t *flash,
    uint64_t       addr,
    uint16_t       length,
    uint16_t      *data
)
{
    /* write the buffer command */
    uint64_t blkaddr = addr & CFG_FLASH_BLKMASK;
    set16(flash, blkaddr, 0x00e8);
    flash->read_state = FLASH_READ_STATUS;

    /* read the status register */
    uint16_t status = get16(flash, blkaddr);
    /* check if WSM is ready */
    if( (status & 0x0080) == 0 )
    {
        return -1;
    }

    /* write the word count */
    set16(flash, blkaddr, (length-1));

    /* write {buffer-data, start-addr} */
    set16(flash, addr, data[0]);

    for( uint64_t i=1; i<length; i++ )
    {
        set16(flash, addr+i, data[i]);
		//printf("writing %04x to addr %x\n", data[i], addr+i);
    }

    /* write {confirm, blkaddr} */
    set16(flash, blkaddr, 0x00d0);

    /* read status register */
    status = get16(flash, blkaddr);
    //printf("programBuffer STS: %04x\n", status);

    uint32_t timeout = CFG_FLASH_TIMEOUT;
    while(status != 0x0080)
    {
        usleep(100);
        status = get16(flash, blkaddr);
        timeout--;
        if( timeout==0 )
        {
            printf("programBuffer timed out (%04x)\n", status);
            return -1;
        }
    }

    return 0;
}
