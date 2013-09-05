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
 * The librorc::sim_bar class represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 */

#ifndef SIM_BAR_H
#define SIM_BAR_H

#include <librorc/include_ext.hh>
#include <librorc/bar_proto.hh>

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


#define DEFAULT_PACKET_SIZE 128

/**
 * @class sim_bar
 * @brief Represents a simulated Base Address Register
 * (BAR) file mapping of the RORCs PCIe address space
 *
 * Create a new sim_bar object after initializing your
 * librorc::device instance. <br>Once your sim_bar instance is
 * initialized (with init()) you can use get() and set() to
 * read from and/or write to the device.
 */

namespace librorc
{
class device;

    class sim_bar : public bar
    {
        public:
            sim_bar
            (
                device  *dev,
                int32_t  n
            );

             virtual
            ~sim_bar();

            void
            memcopy
            (
                librorc_bar_address  target,
                const void          *source,
                size_t               num
            );

            void
            memcopy
            (
                void                *target,
                librorc_bar_address  source,
                size_t               num
            );

            uint32_t get32(librorc_bar_address address );

            uint16_t get16(librorc_bar_address address );

            void
            set32
            (
                librorc_bar_address address,
                uint32_t data
            );

            void
            set16
            (
                librorc_bar_address address,
                uint16_t data
            );

            int32_t
            gettime
            (
                struct timeval *tv,
                struct timezone *tz
            );

            size_t size();

            void
            simSetPacketSize
            (
                uint32_t packet_size
            );


        private:

            int      m_sockfd;
            int      m_pipefd[2];
            int32_t  m_msgid;
            uint32_t m_read_from_dev_data;
            uint32_t m_read_from_dev_done;
            uint32_t m_write_to_dev_done;
            uint32_t m_cmpl_to_dev_done;
            uint32_t m_max_packet_size;

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


}


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
