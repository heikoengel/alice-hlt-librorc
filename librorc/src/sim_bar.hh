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
 * The rorcfs_bar class represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 */

#ifndef SIM_BAR_H
#define SIM_BAR_H

#include "include_ext.hh"
#include "include_int.hh"

#include "librorc_bar_proto.hh"



/**
 * FLI_CMD bit definitions
 * */
#define CMD_READ_FROM_DEVICE 1
#define CMD_WRITE_TO_DEVICE  2
#define CMD_GET_TIME         3
#define CMD_CMPL_TO_DEVICE   4
#define CMD_READ_FROM_HOST   5
#define CMD_WRITE_TO_HOST    6
#define CMD_CMPL_TO_HOST     7
#define CMD_ACK_TIME         8
#define CMD_ACK_WRITE        9
#define CMD_ACK_CMPL         10

#ifdef DEBUG
    #define DEBUG_PRINTF( ... ) {       \
      printf( __VA_ARGS__ );           \
    }
#else
#define DEBUG_PRINTF( ... ) {           \
    }
#endif

/**
 * @class sim_bar
 * @brief Represents a simulated Base Address Register
 * (BAR) file mapping of the RORCs PCIe address space
 *
 * Create a new sim_bar object after initializing your
 * rorcfs_device instance. <br>Once your sim_bar instance is
 * initialized (with init()) you can use get() and set() to
 * read from and/or write to the device.
 */
class sim_bar : public bar
{
    public:

        /**
         * Constructor that sets fname accordingly. No mapping is
         * performed at this point.
         * @param dev parent rorcfs_device
         * @param n number of BAR to be mapped [0-6]
         */
        sim_bar
        (
            rorcfs_device *dev,
            int32_t        n
        );

        /**
         * Deconstructor: free fname, unmap BAR, close file
         */
        ~sim_bar();

        /**
         * Initialize BAR Mapping: open sysfs file, get file stats,
         * mmap file. This has to be done before using any other
         * member function. This function will fail if the requested
         * BAR does not exist
         * @return 0 on sucess, -1 on errors
         */
        int32_t init();

        /**
         * read DWORD from BAR address
         * @param addr (unsigned int) aligned address within the
         *              BAR to read from.
         * @return data read from BAR[addr]
         */
        unsigned int
        get
        (
            uint64_t addr
        );

        /**
         * write DWORD to BAR address
         * @param addr (unsigned int) aligned address within the
         *              BAR to write to
         * @param data (unsigned int) data word to be written.
         */
        void
        set
        (
            uint64_t addr,
            uint32_t data
        );

        /**
         * copy buffer range into BAR
         * @param addr address in current BAR
         * @param source pointer to source data field
         * @param num number of bytes to be copied to destination
         */
        void
        memcpy_bar
        (
            uint64_t    addr,
            const void *source,
            size_t      num
        );

        /**
         * read DWORD from BAR address
         * @param addr (unsigned int) aligned address within the
         *              BAR to read from.
         * @return data read from BAR[addr]
         */
        unsigned short
        get16
        (
            uint64_t addr
        );

        /**
         * write WORD to BAR address
         * @param addr within the BAR to write to
         * @param data (unsigned int) data word to be written.
         */
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
         */
        int32_t
        gettime
        (
            struct timeval *tv,
            struct timezone *tz
        );

        /**
         * get size of mapped BAR. This value is only valid after init()
         * @return size of mapped BAR in bytes
         */
        size_t getSize();


    private:

        /**
         * Initialize member variables
         */
        int      m_sockfd;
        int      m_pipefd[2];
        int32_t  m_msgid;
        uint32_t m_read_from_dev_data;
        uint32_t m_read_from_dev_done;
        uint32_t m_write_to_dev_done;
        uint32_t m_cmpl_to_dev_done;

        pthread_t sock_mon_p;
        pthread_t cmpl_handler_p;


        void *sockMonitor();
        void *cmplHandler();

        static void*
        sock_monitor_helper(void * This)
        {
            ((sim_bar *)This)->sockMonitor();
            return 0;
        }

        static void*
        cmpl_handler_helper(void * This)
        {
            ((sim_bar *)This)->cmplHandler();
            return 0;
        }

        uint32_t
        readDWfromSock
        (
            int sock
        );

        void
        doCompleteToHost
        (
            uint16_t msgsize
        );

        void
        doWriteToHost
        (
            uint16_t msgsize
        );

        void
        doReadFromHost
        (
            uint16_t msgsize
        );

        void
        doAcknowledgeCompletion
        (
            uint16_t msgsize
        );

        void
        doAcknowledgeWrite
        (
            uint16_t msgsize
        );

        void
        doAcknowledgeTime
        (
            uint16_t msgsize
        );

        int32_t
        getOffset
        (
            uint64_t  phys_addr,
            uint64_t *buffer_id,
            uint64_t *offset
        );
};


/**
 * A structure to represent FLI read request
 */
typedef struct

__attribute__((__packed__))
{
	uint64_t buffer_id;     /**< Buffer-ID*/
	uint64_t offset;        /**< Request offset*/
	uint16_t length;        /**< Lenght of request*/
	uint8_t  tag;           /**< Request tag */
	uint8_t  lower_addr;    /**< Lower alligned address */
	uint8_t  byte_enable;   /**< Request byte-enable*/
	uint32_t requester_id;  /**< Requester-ID*/
} t_read_req;

/**
 * A structure to represent FLI Messaging
 */
typedef struct
__attribute__((__packed__))
{
	uint32_t  wr_ack;   /**< Message write acknowledge */
	uint32_t  data;     /**< Message data */
	int32_t   id;       /**< Message-ID*/
} flimsgT;

/**
 * A structure to represent FLI Command
 */
typedef struct
__attribute__((__packed__))
{
	uint32_t addr;          /**< Alligned address */
	uint32_t bar;           /**< Command base address registers */
	uint8_t  byte_enable;   /**< Command byte-enable */
	uint8_t  type;          /**< Command type */
	uint16_t len;           /**<  */
	int32_t  id;            /**< Command-ID */
} flicmdT;

#endif /** SIM_BAR_H */
