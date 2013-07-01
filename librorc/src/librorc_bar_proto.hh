#ifndef LIBRORC_BAR_PROTO_H
#define LIBRORC_BAR_PROTO_H

#include "include_ext.hh"
#include "librorc_device.hh"

namespace librorc
{

    class bar
    {
    public:

    /**
     * read DWORD from BAR address
     * @param addr (unsigned int) aligned address within the
     *              BAR to read from.
     * @return data read from BAR[addr]
     **/
    virtual
    uint32_t
    get
    (
        uint64_t addr
    ) = 0;

    /**
     * read WORD from BAR address
     * @param addr within the BAR to read from.
     * @return data read from BAR[addr]
     **/
    virtual
    uint16_t
    get16
    (
        uint64_t addr
    ) = 0;

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
        uint64_t    addr,
        const void *source,
        size_t      num
    ) = 0;

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
        uint64_t addr,
        uint32_t data
    ) = 0;

    /**
     * write WORD to BAR address
     * @param addr within the BAR to write to
     * @param data (unsigned int) data word to be written.
     **/
    virtual
    void
    set16
    (
        uint64_t addr,
        uint16_t data
    ) = 0;

    /**
     * get current time of day
     * @param tv pointer to struct timeval
     * @param tz pointer to struct timezone
     * @return return value from gettimeof day or zero for FLI simulation
     **/
    virtual
    int32_t
    gettime
    (
        struct timeval *tv,
        struct timezone *tz
    ) = 0;

    /**
     * initialize BAR mapping: open sysfs file, get file stats,
     * mmap file. This has to be done before using any other
     * member funtion. This function will fail if the requested
     * BAR does not exist.
     * @return 0 on sucess, -1 on errors
     **/
    virtual
    int32_t
    init() = 0;

    /**
     * get size of mapped BAR. This value is only valid after init()
     * @return size of mapped BAR in bytes
     **/
    virtual size_t getSize() = 0;

    protected:
        device          *m_parent_dev;
        PciDevice       *m_pda_pci_device;
        pthread_mutex_t  m_mtx;
        int32_t          m_number;
        uint8_t         *m_bar;
        size_t           m_size;

    };

}
#endif /** LIBRORC_BAR_PROTO_H */
