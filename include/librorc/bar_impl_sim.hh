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

#ifndef LIBRORC_BAR_IMPL_SIM_H
#define LIBRORC_BAR_IMPL_SIM_H

#include <librorc/defines.hh>
#include <librorc/bar.hh>
#ifdef PDA
#include <pda.h>
#endif

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

namespace LIBRARY_NAME
{
    class device;

    /**
     * @brief Represents a simulated Base Address Register
     * (BAR) file mapping of the RORCs PCIe address space
     */
    class bar_impl_sim
    {
        public:
            bar_impl_sim
            (
                device  *dev,
                int32_t  n
            );

             virtual
            ~bar_impl_sim();

            void
            memcopy
            (
                bar_address  target,
                const void  *source,
                size_t       num
            );

            void
            memcopy
            (
                void        *target,
                bar_address  source,
                size_t       num
            );

            uint32_t get32(bar_address address );

            uint16_t get16(bar_address address );

            void
            set32
            (
                bar_address address,
                uint32_t data
            );

            void
            set16
            (
                bar_address address,
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
            device *m_parent_dev;
#ifdef PDA
            PciDevice *m_pda_pci_device;
#endif
            pthread_mutex_t m_mtx;
            int32_t m_number;
            uint8_t *m_bar;
            size_t m_size;

            int      m_sockfd;
            int      m_pipefd[2];
            int32_t  m_msgid;
            uint32_t m_read_from_dev_data;
            uint32_t m_read_from_dev_done;
            uint64_t m_read_time_data;
            uint32_t m_read_time_done;
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
                ((bar_impl_sim *)This)->sockMonitor();
                return 0;
            }

            static void*
            cmpl_handler_helper(void * This)
            {
                ((bar_impl_sim *)This)->cmplHandler();
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

#endif /** LIBRORC_BAR_IMPL_SIM_H */
