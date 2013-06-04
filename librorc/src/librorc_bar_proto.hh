#ifndef _RORCLIB_LIBRORC_BAR_PROTO_H
#define _RORCLIB_LIBRORC_BAR_PROTO_H

#include "rorcfs_device.hh"

class librorc_bar
{
public:

/**
 * read DWORD from BAR address
 * @param addr (unsigned int) aligned address within the
 *              BAR to read from.
 * @return data read from BAR[addr]
 **/
virtual
unsigned int
get
(
    unsigned long addr
);

/**
 * read WORD from BAR address
 * @param addr within the BAR to read from.
 * @return data read from BAR[addr]
 **/
virtual
unsigned short
get16
(
    unsigned long addr
);

/**
 * copy buffer range into BAR
 * @param addr address in current BAR
 * @param source pointer to source data field
 * @param num number of bytes to be copied to destination
 * */
virtual
void
memcpy_bar
(
    unsigned long addr,
    const void   *source,
    size_t        num
);

/**
 * write DWORD to BAR address
 * @param addr (unsigned int) aligned address within the
 *              BAR to write to
 * @param data (unsigned int) data word to be written.
 **/
virtual
void
set
(
    unsigned long addr,
    unsigned int  data
);

/**
 * write WORD to BAR address
 * @param addr within the BAR to write to
 * @param data (unsigned int) data word to be written.
 **/
virtual
void
set16
(
    unsigned long  addr,
    unsigned short data
);

/**
 * get current time of day
 * @param tv pointer to struct timeval
 * @param tz pointer to struct timezone
 * @return return valiue from gettimeof day or zero for FLI simulation
 **/
virtual
int
gettime
(
    struct timeval *tv,
    struct timezone *tz
);

/**
 * initialize BAR mapping: open sysfs file, get file stats,
 * mmap file. This has to be done before using any other
 * member funtion. This function will fail if the requested
 * BAR does not exist.
 * @return 0 on sucess, -1 on errors
 **/
virtual
int
init();

/**
 * get size of mapped BAR. This value is only valid after init()
 * @return size of mapped BAR in (unsigned long) bytes
 **/
virtual
size_t
getSize()
{
    return m_size;
}

protected:
    rorcfs_device   *m_parent_dev;
    PciDevice       *m_pda_pci_device;
    pthread_mutex_t  m_mtx;
    int              m_number;
    uint8_t         *m_bar;
    size_t           m_size;

};

#endif
