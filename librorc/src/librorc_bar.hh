/**
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
 *
 * @section DESCRIPTION
 *
 * The rorc_bar class represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 */

#ifndef LIBRORC_BAR_H
#define LIBRORC_BAR_H

#include "include_ext.hh"

#include "librorc_bar_proto.hh"

/**
 * @class rorc_bar
 * @brief Represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 *
 * Create a new crorc_bar object after initializing your
 * rorcfs_device instance. <br>Once your rorc_bar instance is
 * initialized (with init()) you can use get() and set() to
 * read from and/or write to the device.
 */
class rorc_bar : public librorc_bar
{
    public:

        /**
         * Constructor that sets fname accordingly. No mapping is
         * performed at this point.
         * @param dev parent rorcfs_device
         * @param n number of BAR to be mapped [0-6]
         **/
        rorc_bar
        (
            rorcfs_device *dev,
            int32_t        n
        );

        /**
         * Deconstructor: free fname, unmap BAR, close file
         **/
        ~rorc_bar();

        /**
         * initialize BAR mapping: open sysfs file, get file stats,
         * mmap file. This has to be done before using any other
         * member funtion. This function will fail if the requested
         * BAR does not exist.
         * @return 0 on sucess, -1 on errors
         **/
        int32_t init();

        /**
         * read DWORD from BAR address
         * @param addr (unsigned int) aligned address within the
         *              BAR to read from.
         * @return data read from BAR[addr]
         **/
        uint32_t
        get
        (
            uint64_t addr
        );

        /**
         * read WORD from BAR address
         * @param addr within the BAR to read from.
         * @return data read from BAR[addr]
         **/
        uint16_t
        get16
        (
            uint64_t addr
        );

        /**
         * copy buffer range into BAR
         * @param addr address in current BAR
         * @param source pointer to source data field
         * @param num number of bytes to be copied to destination
         * */
        void
        memcpy_bar
        (
            uint64_t    addr,
            const void *source,
            size_t      num
        );

        /**
         * write DWORD to BAR address
         * @param addr (unsigned int) aligned address within the
         *              BAR to write to
         * @param data (unsigned int) data word to be written.
         **/
        void
        set
        (
            uint64_t addr,
            uint32_t data
        );

        /**
         * write WORD to BAR address
         * @param addr within the BAR to write to
         * @param data (unsigned int) data word to be written.
         **/
        void
        set16
        (
            uint64_t addr,
            uint16_t data
        );

        /**
         * get current time of day
         * @param tv pointer to struct timeval
         * @param tz pointer to struct timezone
         * @return return valiue from gettimeof day or zero for FLI simulation
         **/
        int32_t
        gettime
        (
            struct timeval *tv,
            struct timezone *tz
        );

        /**
         * get size of mapped BAR. This value is only valid after init()
         * @return size of mapped BAR in bytes
         **/

        size_t getSize();

};

#endif /** LIBRORC_BAR_H */
