/**
 * @file rorcfs_buffer.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2011-08-17
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
 * @section DESCRIPTION
 *
 */

#include <stdio.h>
#include <sys/mman.h> //remove
#include <sys/stat.h> //remove
#include <unistd.h>
#include <errno.h> //remove
#include <fcntl.h> //remove

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <pda.h>

#include "rorcfs_device.hh"
#include "rorcfs_buffer.hh"

using namespace std;

/** errno */
extern int errno; //remove ?

/**
 * Constructor
 **/
rorcfs_buffer::rorcfs_buffer()
{
    m_overmapped   = 0;
    m_id           = 0;
    m_size         = 0;
    m_dmaDirection = 0;

    //MappingSize = 0;
    nSGEntries = 0;
    mem = NULL;

    dname = NULL;//remove
    dname_size = 0;//remove
    //PhysicalSize = 0;//remove
    base_name_size = 0;//remove
    base_name = NULL;//remove
}



/**
 * Destructor:
 * release() buffer if not done already
 **/
rorcfs_buffer::~rorcfs_buffer()
{
}



/**
 * Allocate Buffer: initiate memory allocation,
 * connect to new buffer & retrieve actual buffer sizes
 **/

int
rorcfs_buffer::allocate
(
    rorcfs_device *dev,
    unsigned long  size,
    unsigned long  id,
    int            overmap,
    int            dma_direction
)
{
    m_device =
        dev->getPdaPciDevice();

    //TODO: convert direction specifier

    m_buffer = NULL;
    if(PDA_SUCCESS != PciDevice_getDMABuffer(m_device, id, &m_buffer) )
    {
        if
        (
            PDA_SUCCESS !=
                PciDevice_allocDMABuffer
                    (m_device, size, PDABUFFER_DIRECTION_BI, &m_buffer)
        )
        {
            cout << "DMA Buffer allocation failed!" << endl;
            return -1;
        }
    }

    m_dmaDirection = dma_direction;

    if(overmap == 1)
    {
        if(PDA_SUCCESS != DMABuffer_overmap(m_buffer) )
        {
            cout << "Overmapping failed!" << endl;
            return(-1);
        }
        m_overmapped = 1;
    }

    if(DMABuffer_getLength(m_buffer, &m_size)!=PDA_SUCCESS)
    {
        cout << "Size lookup failed!" << endl;
        return -1;
    }

    /** connect to allocated buffer */
    if(connect(dev, id) == -1)
    {
        return -1;
    }

    m_id = id;

    return 0;
}



/**
 * Release buffer
 **/

int
rorcfs_buffer::deallocate()
{
    return 0;
}



int
rorcfs_buffer::connect
(
    rorcfs_device *dev,
    unsigned long  id
)
{

/** */
//    char       *fname;
//    int         fname_size;
//    struct stat filestat;
//
//    int nbytes = 0;
//
//    if(mem != NULL)  //already connected, unmap first!
//    {
//        errno = EPERM;
//        return -1;
//    }
//
//    // get sysfs base directory name and size
//
//    /*** base_name_size = dev->getDName( &base_name ); ***/
//
//    // get MappingSize from sysfs attribute
//    fname_size = snprintf(NULL, 0, "%s%03ld/mem", base_name, id);
//    fname_size++;
//    fname = (char*) malloc(fname_size);
//    if(!fname)
//    {
//        errno = ENOMEM;
//        return -1;
//    }
//
//    snprintf(fname, fname_size, "%s%03ld/mem", base_name, id);
//
//    fdEB = open(fname, O_RDWR);
//    if(fdEB == -1)
//    {
//        perror("open mem");
//        free(fname);
//        return -1;
//    }
//    free(fname);
//
//    if(fstat(fdEB, &filestat) == -1)
//    {
//        close(fdEB);
//        return -1;
//    }
/** */

/** __ */
//    int fd;
//    // set MappingSize to the size of the sysfs file
//    MappingSize = filestat.st_size;
//
//    // calculate PhysicalSize with "overmapped" sysfs attribute
//    fname_size = snprintf(NULL, 0, "%s%03ld/overmapped", base_name, id);
//    fname_size++;
//    fname = (char*) malloc(fname_size);
//    if(!fname)
//    {
//        errno = ENOMEM;
//        close(fdEB);
//        return -1;
//    }
//
//    snprintf(fname, fname_size, "%s%03ld/overmapped", base_name, id);
//    fd = open(fname, O_RDONLY);
//    if(fd == -1)
//    {
//        perror("open overmapped");
//        free(fname);
//        close(fdEB);
//        return -1;
//    }
//    free(fname);
//
//    // read from "overmapped" - returns 1 or 0
//    nbytes = read( fd, &overmapped, sizeof(int) );
//    if(nbytes != sizeof(int) )
//    {
//        perror("read overmapped");
//        close(fd);
//        close(fdEB);
//        return -1;
//    }
//    close(fd);
//
//    // Set PhysicalSize attribute according to the contents of
//    // the sysfs file "overmapped"
//    if(overmapped)
//        PhysicalSize = MappingSize / 2;
//    else
//        PhysicalSize = MappingSize;
//
//    // MMap Buffer
//    mem = (unsigned int*)mmap(0, MappingSize, PROT_READ | PROT_WRITE,
//                              MAP_SHARED, fdEB, 0);
//    if(mem == MAP_FAILED)
//    {
//        close(fdEB);
//        perror("mmap mem");
//        return -1;
//    }
//
//    // get nSGEntries from sysfs attribute
//    fname_size = snprintf(NULL, 0, "%s%03ld/sglist", base_name, id);
//    fname_size++;
//    fname = (char*) malloc(fname_size);
//    if(!fname)
//    {
//        errno = ENOMEM;
//        return -1;
//    }
//
//    snprintf(fname, fname_size, "%s%03ld/sglist", base_name, id);
//    fd = open(fname, O_RDONLY);
//    if(fd == -1)
//    {
//        free(fname);
//        perror("open sglist");
//        return -1;
//    }
//    free(fname);
//
//    if(fstat(fd, &filestat) == -1)
//    {
//        close(fd);
//        return -1;
//    }
//
//    nSGEntries = filestat.st_size / sizeof(struct rorcfs_dma_desc);
//
//    close(fd);
//
//    // store buffer id
//    this->id = id;
//
//    // save sysfs directory name of created buffer
//    // e.g. /sys/module/rorcfs/drivers/pci:rorcfs/0000:03:00.0/mmap/001/
//    dname_size = snprintf(NULL, 0, "%s%03ld/", base_name, id);
//    dname_size++;
//    dname = (char*) malloc(dname_size);
//    if(!dname)
//    {
//        errno = ENOMEM;
//        return -1;
//    }
//
//    snprintf(dname, dname_size, "%s%03ld/", base_name, id);

    return 0;
}
