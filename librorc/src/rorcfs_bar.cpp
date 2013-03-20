/**
 * @file rorcfs_bar.cpp
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @version 0.2
 * @date 2011-08-16
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
 */

#include "rorcfs_bar.hh"

#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rorc_registers.h"
#include "rorcfs_device.hh"

#include "rorcfs_buffer.hh"
#include "rorcfs_dma_channel.hh"

/**
 * usleep time for FLI read polling
 * */
#define USLEEP_TIME 50

/** Prototypes **/
char*
getChOff
(
    unsigned int addr
);

int
get_offset
(
    uint64_t  phys_addr,
    uint64_t *buffer_id,
    uint64_t *offset
);



rorcfs_bar::rorcfs_bar
(
    rorcfs_device *dev,
    int            n
)
{
    m_parent_dev = dev;
    m_number     = n;

    /** initialize mutex */
    pthread_mutex_init(&m_mtx, NULL);
}



rorcfs_bar::~rorcfs_bar()
{
    pthread_mutex_destroy(&m_mtx);
    /** Further stuff here **/
}



/**
 * rorcfs_bar::init()
 * Initialize and mmap BAR
 * */

int
rorcfs_bar::init()
{
    m_bar = m_parent_dev->getBarMap(m_number);

    if(m_bar == NULL)
    {
        return -1;
    }

    return 0;
}



/**
 * read 1 DW from BAR
 * @param addr address (DW-aligned)
 * @return value read from BAR
 * */

unsigned int
rorcfs_bar::get
(
    unsigned long addr
)
{
    assert( m_bar != NULL );

    unsigned int *bar = (unsigned int *)m_bar;
    unsigned int result;
    if( (addr << 2) < m_size)
    {
        result = bar[addr];
        return result;
    }
    else
    {
        return -1;
    }
}



/**
 * write 1 DW to BAR
 * @param addr DW-aligned address
 * @param data DW data to be written
 * */

void
rorcfs_bar::set
(
    unsigned long addr,
    unsigned int  data
)
{
    assert( m_bar != NULL );
    unsigned int *bar = (unsigned int *)m_bar;
    if( (addr << 2) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        bar[addr] = data;
        msync( (bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }
}



/**
 * copy a buffer to BAR via memcpy
 * @param addr DW-aligned address
 * @param source source buffer
 * @param num number of bytes to be copied from source to dest
 * */

void
rorcfs_bar::memcpy_bar
(
    unsigned long addr,
    const void   *source,
    size_t        num
)
{
    pthread_mutex_lock(&m_mtx);
    memcpy( (unsigned char*)m_bar + (addr << 2), source, num);
    msync( (m_bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&m_mtx);
}



unsigned short
rorcfs_bar::get16
(
    unsigned long addr
)
{
    unsigned short *sbar;
    sbar = (unsigned short*)m_bar;
    unsigned short result;
    assert( sbar != NULL );
    if( (addr << 1) < m_size)
    {
        result = sbar[addr];
        return result;
    }
    else
    {
        return 0xffff;
    }
}



void
rorcfs_bar::set16
(
    unsigned long  addr,
    unsigned short data
)
{
    unsigned short *sbar;
    sbar = (unsigned short*)m_bar;

    assert( sbar != NULL );
    if( (addr << 1) < m_size)
    {
        pthread_mutex_lock(&m_mtx);
        sbar[addr] = data;
        msync( (sbar + ( (addr << 1) & PAGE_MASK) ),
               PAGE_SIZE, MS_SYNC);
        pthread_mutex_unlock(&m_mtx);
    }
}


//TODO: turn this into a method
char*
getChOff
(
    unsigned int addr
)
{
    int   channel = (addr >> 15) & 0xf;
    int   comp = (addr >> RORC_DMA_CMP_SEL) & 0x01;
    int   offset = addr & ( (1 << RORC_DMA_CMP_SEL) - 1);
    char *buffer = (char*)malloc(256);
    sprintf(buffer, "ch:%d comp:%d off:%d",
            channel, comp, offset);
    return buffer;
}



/**
 * get file and offset for physical address
 * */

int
get_offset
(
    uint64_t  phys_addr,
    uint64_t *buffer_id,
    uint64_t *offset
)
{
    char           *basedir;
    int             i, j, bufn, nentries, fd, nbytes;
    struct dirent **namelist;

    struct rorcfs_dma_desc desc;

    struct stat filestat;

    int   fname_size;
    char *fname;
    int   err;

    // attach to device
    rorcfs_device *dev = new rorcfs_device();
    if(dev->init(0) == -1)
    {
        printf("failed to initialize device 0\n");
    }

    /**
    dev->getDName(&basedir);

    // get list of buffers
    bufn = scandir(basedir, &namelist, scandir_filter, alphasort);
    **/

    // iterate over all buffers
    for(i = 0; i < bufn; i++)
    {
        // get filename of sglist file
        fname_size = snprintf(NULL, 0, "%s%s/sglist",
                              basedir, namelist[i]->d_name) + 1;
        fname = (char*)malloc(fname_size);
        snprintf(fname, fname_size, "%s%s/sglist",
                 basedir, namelist[i]->d_name);

        // get number of entries
        err = stat(fname, &filestat);
        if(err)
            printf("DMA_MON ERROR: failed to stat file %s\n", fname);
        nentries = filestat.st_size / sizeof( struct rorcfs_dma_desc);

        // open sglist file
        fd = open(fname, O_RDONLY);
        free(fname);
        if(fd == -1)
            perror("DMA_MON ERROR: open sglist");
        *offset = 0;

        // iterate over all sglist entries
        for(j = 0; j < nentries; j++)
        {
            nbytes = read(fd, &desc, sizeof(struct rorcfs_dma_desc) );
            if(nbytes != sizeof(struct rorcfs_dma_desc) )
            {
                printf("DMA_MON ERROR: nbytes(=%d) != sizeof(struct "
                       "rorcfs_dma_desc)\n", nbytes);
            }

            // check if this is the destination segment
            if( ( desc.addr <= phys_addr) && ( desc.addr + desc.len > phys_addr) )
            {
                //adjust offset
                *offset += (phys_addr - desc.addr);
                *buffer_id = strtoul(namelist[i]->d_name, NULL, 0);
                close(fd);
                return 0;

            } // dest-segment found
            else
            {
                *offset += desc.len;
            }
        } //iterate over segments

        // close sglist file
        close(fd);
    } // iterate over buffers

    printf("ERROR: destination not found: addr=%lx\n", phys_addr);
    return -1;
}
