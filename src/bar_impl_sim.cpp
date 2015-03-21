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

#include <pthread.h>
#include <netdb.h>
#include <sys/mman.h>
#include <cstring> //memcpy
#include <cstdlib> //abort
#include <cstdio> //perror

#include <librorc/device.hh>
#include <librorc/error.hh>
#include <librorc/bar_impl_sim.hh>
#include <librorc/buffer.hh>

#ifndef MODELSIM_SERVER
    #define MODELSIM_SERVER "localhost"
#endif

/**
 * usleep time for FLI read polling
 * */
#define USLEEP_TIME 50

namespace LIBRARY_NAME
{

bar_impl_sim::bar_impl_sim
(
    device  *dev,
    int32_t  n
)
{
    m_parent_dev = dev;
    m_number     = n;

#ifdef PDA
    m_pda_pci_device = dev->getPdaPciDevice();
#endif

    /** initialize mutex */
    pthread_mutex_init(&m_mtx, NULL);

    m_read_from_dev_done = 0;
    m_read_time_done = 0;
    m_write_to_dev_done  = 0;
    m_max_packet_size = DEFAULT_PACKET_SIZE;

    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( m_sockfd < 0 )
    {
        std::cout << "ERROR: failed to open socket" << std::endl;
        throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED;
    }

    struct hostent *server = gethostbyname(MODELSIM_SERVER);
    if( server == NULL )
    {
        std::cout << "ERROR: no such host" << std::endl;
        throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED;
    }

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy( (char *)server->h_addr,
           (char *)&serv_addr.sin_addr.s_addr,
           server->h_length
         );
    serv_addr.sin_port = htons(2000);
    if( 0 > connect(m_sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) )
    {
        std::cout << "ERROR connecting" << std::endl;
        throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED;
    }

    /** create pipe for redirecting read requests to completion handler
     *  pipefd[0]: read end
     *  pipefd[1]: write end
     */
    if( pipe(m_pipefd) == -1 )
    {
        std::cout << "ERROR: Failed to create PIPE" << std::endl;
        throw LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED;
    }

    /** completion handler */
    pthread_create(&cmpl_handler_p, NULL, cmpl_handler_helper, this);

    /** watch incoming packets */
    pthread_create(&sock_mon_p, NULL, sock_monitor_helper, this);
    m_msgid = 0;
}



bar_impl_sim::~bar_impl_sim()
{
    close(m_pipefd[0]);
    close(m_pipefd[1]);
    pthread_cancel(sock_mon_p);
    pthread_cancel(cmpl_handler_p);
    close(m_sockfd);

    pthread_mutex_destroy(&m_mtx);
    /** Further stuff here **/
}



void
bar_impl_sim::memcopy
(
    bar_address  target,
    const void          *source,
    size_t               num
)
{
    int      result;
    size_t   ndw = num>>2;
    int32_t  buffersize = 4 + ndw;
    uint32_t buffer[buffersize];
    buffer[0] = ((ndw+4)<<16) + CMD_WRITE_TO_DEVICE;
    buffer[2] = target<<2;
    if( ndw > 1 )
    {
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0xff<<16) + ndw;
    }
    else
    {
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0x0f<<16) + ndw;
    }
    memcpy(&(buffer[4]), source, num);

    pthread_mutex_lock(&m_mtx);
    {
        buffer[1] = m_msgid;
        result = write( m_sockfd, buffer, buffersize*sizeof(uint32_t) );
        m_msgid++;
    }
    pthread_mutex_unlock(&m_mtx);


    if( result != (ssize_t)(buffersize*sizeof(uint32_t)) )
    {
        std::cout << "ERROR writing to socket" << std::endl;
    }
    else
    {
        /** wait for FLI acknowledgement */
        while( !m_write_to_dev_done )
        {
            usleep(USLEEP_TIME);
        }
        m_write_to_dev_done=0;

        //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "%d: memcpy %ld DWs to %lx\n",
        //        buffer[1], ndw, target);
    }
}



void
bar_impl_sim::memcopy
(
    void                *target,
    bar_address  source,
    size_t               num
)
{
    pthread_mutex_lock(&m_mtx);
    //TODO : needs to be implemented!
    pthread_mutex_unlock(&m_mtx);
}



uint32_t bar_impl_sim::get32(bar_address address )
{
    int      result;
    uint32_t data = 0;
    uint32_t buffersize = 4;
    uint32_t buffer[buffersize];
    buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
    buffer[2] = address<<2;
    /** BAR, BE, length */
    buffer[3] = (m_number<<24) + (0x0f<<16) + 1;

    pthread_mutex_lock(&m_mtx);
    {
        buffer[1] = m_msgid;
        result = write( m_sockfd, buffer, sizeof(uint32_t)*buffersize );
        m_msgid++;
    }
    pthread_mutex_unlock(&m_mtx);

    if( result != (ssize_t)(sizeof(uint32_t)*buffersize) )
    {
        std::cout << "ERROR writing to socket" << std::endl;
    }
    else
    {
        /** wait for completion */
        while( !m_read_from_dev_done )
        {
            usleep(USLEEP_TIME);
        }

        data = m_read_from_dev_data;
        m_read_from_dev_done = 0;

        //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "%d: get32(0x%lx)=%08x\n",
        //        buffer[1], address, data);

    }
    return data;
}



uint16_t bar_impl_sim::get16(bar_address address )
{
    int      result;
    uint16_t data = 0;
    int32_t  buffersize = 4;
    uint32_t buffer[buffersize];
    buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
    buffer[2] = (address<<1) & ~(0x03);
    if( address & 0x01 )
    {
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0x0c<<16) + 1;
    }
    else
    {
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0x03<<16) + 1;
    }

    pthread_mutex_lock(&m_mtx);
    {
        buffer[1] = m_msgid;
        result = write(m_sockfd, buffer, buffersize*sizeof(uint32_t));
        m_msgid++;
    }

    if( result != (ssize_t)(buffersize*sizeof(uint32_t)) )
    {
        std::cout << "ERROR writing to socket" << std::endl;
    }
    else
    {
        /** wait for FLI completion */
        while( !m_read_from_dev_done )
        {
            usleep(USLEEP_TIME);
        }
        data = m_read_from_dev_data & 0xffff;
        m_read_from_dev_done = 0;

        //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "%d: get16(0x%lx)=%04x\n",
        //        buffer[1], address, data);
    }

    return data;
}



void
bar_impl_sim::set32
(
    bar_address address,
    uint32_t data
)
{
    /** send write command to Modelsim FLI server */
    int      result;
    int32_t  buffersize = 5;
    uint32_t buffer[buffersize];
    buffer[0] = (5<<16) + CMD_WRITE_TO_DEVICE;
    buffer[2] = address<<2;
    buffer[3] = (m_number<<24) + (0x0f<<16) + 1; //BAR, BE, length
    buffer[4] = data;

    pthread_mutex_lock(&m_mtx);
    {
        buffer[1] = m_msgid;
        result = write( m_sockfd, buffer, buffersize*sizeof(uint32_t) );
        m_msgid++;
    }
    pthread_mutex_unlock(&m_mtx);

    if( result != (ssize_t)(buffersize*sizeof(uint32_t)) )
    {
        std::cout << "ERROR writing to socket" << std::endl;
    }
    else
    {
        /** wait for FLI acknowledgement */
        while( !m_write_to_dev_done )
        {
            usleep(USLEEP_TIME);
        }
        m_write_to_dev_done=0;

        //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "%d: set32(0x%lx, %08x)\n",
        //        buffer[1], address, data);
    }
}



void
bar_impl_sim::set16
(
    bar_address address,
    uint16_t data
)
{
    /** send write command to Modelsim FLI server */
    int      result;
    uint32_t buffersize = 5;
    uint32_t buffer[buffersize];
    buffer[0] = (5<<16) + CMD_WRITE_TO_DEVICE;
    buffer[2] = (address<<1) & ~(0x03);
    if ( address & 0x01 )
    {
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0x0c<<16) + 1;
    }
    else
    {
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0x03<<16) + 1;
    }
    buffer[4] = (data<<16) + data;

    pthread_mutex_lock(&m_mtx);
    {
        buffer[1] = m_msgid;
        result = write(m_sockfd, buffer, buffersize*sizeof(uint32_t));
        m_msgid++;
    }
    pthread_mutex_unlock(&m_mtx);

    if ( result != (ssize_t)(buffersize*sizeof(uint32_t)) )
    {
        std::cout << "ERROR writing to socket" << std::endl;
    }
    else
    {
        /** wait for FLI acknowledgement */
        while( !m_write_to_dev_done )
        {
            usleep(USLEEP_TIME);
        }
        m_write_to_dev_done=0;

        //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "%d: set32(0x%lx, %04x)\n",
        //        buffer[1], address, data);
    }
}



int32_t
bar_impl_sim::gettime
(
    struct timeval *tv,
    struct timezone *tz
)
{
    int      result;
    int32_t  buffersize = 2;
    uint32_t buffer[buffersize];
    buffer[0] = (2<<16) + CMD_GET_TIME;

    pthread_mutex_lock(&m_mtx);
    {
        buffer[1] = m_msgid;
        result = write(m_sockfd, buffer, buffersize*sizeof(uint32_t));
        m_msgid++;
    }
    pthread_mutex_unlock(&m_mtx);

    if( result != (ssize_t)(buffersize*sizeof(uint32_t)) )
    {
        std::cout << "ERROR writing to socket" << std::endl;
    }

    /** wait for FLI completion */
    while( !m_read_time_done )
    {
        usleep(USLEEP_TIME);
    }

    uint64_t data = m_read_time_data;
    m_read_time_done = 0;

    /** mti_Now is in [ns] */
    tv->tv_sec  = (data/1000/1000/1000);
    tv->tv_usec = (data/1000)%1000000;

    return 0;
}



void*
bar_impl_sim::sockMonitor()
{
    uint32_t tmpvar = 0;
    while( (tmpvar=readDWfromSock(m_sockfd)) )
    {
        uint16_t msgsize   = (tmpvar >> 16) & 0xffff;
        readDWfromSock(m_sockfd); //msg_id

        switch(tmpvar & 0xffff)
        {
            case CMD_CMPL_TO_HOST:
                { doCompleteToHost(msgsize);        }
            break;

            case CMD_WRITE_TO_HOST:
                { doWriteToHost(msgsize);           }
            break;

            case CMD_READ_FROM_HOST:
                { doReadFromHost(msgsize);          }
            break;

            case CMD_ACK_CMPL:
                { doAcknowledgeCompletion(msgsize); }
            break;

            case CMD_ACK_WRITE:
                { doAcknowledgeWrite(msgsize);      }
            break;

            case CMD_ACK_TIME:
                { doAcknowledgeTime(msgsize);       }
            break;

            default: break;
        }
    }

    return NULL;
}


    uint32_t
    bar_impl_sim::readDWfromSock
    (
        int sock
    )
    {
        uint32_t buffer;

        int result = read(sock, &buffer, sizeof(uint32_t)); /** read 1 DW */
        if (result == 0)
        {
            /** terminate if 0 characters received */
            //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "readDWfromSock: closing socket\n");
            close(sock);
        }
        else if (result!=sizeof(uint32_t))
        {
            std::cout << "ERROR: librorc::bar_impl_sim::readDWfromSock returned "
                 << result << " bytes" << std::endl;
        }

        return buffer;
    }


    void
    bar_impl_sim::doCompleteToHost
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=4)
        {
            std::cout << "ERROR: Invalid message size for CMD_CMPL_TO_HOST: "
                 << msgsize << std::endl;
        }

        readDWfromSock(m_sockfd);
        m_read_from_dev_data = readDWfromSock(m_sockfd);
        m_read_from_dev_done = 1;
    }


    void
    bar_impl_sim::doWriteToHost
    (
        uint16_t msgsize
    )
    {
        /**
         * get_offset, write into buffer
         * [addr_high][addr_low][payload]
         */
        uint64_t addr  = (uint64_t)readDWfromSock(m_sockfd)<<32;
        addr          += readDWfromSock(m_sockfd);
        msgsize       -= 4;

        uint32_t msg_buffer[msgsize];
        for(uint64_t i=0; i<msgsize; i++)
        {
            msg_buffer[i] = readDWfromSock(m_sockfd);
        }

        uint64_t offset    = 0;
        uint64_t buffer_id = 0;
        if( getOffset(addr, &buffer_id, &offset) )
        {
            std::cout << "ERROR: Could not find physical address 0x"
                 << std::hex << addr << std::dec << std::endl;
            abort();
        }
        else
        {
            buffer *buf;

            try
            {
                buf = new buffer(m_parent_dev, buffer_id, 0);
            }
            catch(...)
            {
                perror("failed to connect to buffer");
                abort();
            }

            uint32_t *mem = buf->getMem();
            memcpy(mem+(offset>>2), msg_buffer, msgsize*sizeof(uint32_t));

            //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "CMD_WRITE_TO_HOST: %d DWs to buf %ld offset 0x%lx\n",
            //        msgsize, buffer_id, offset);

            delete buf;
        }
    }


    void
    bar_impl_sim::doReadFromHost
    (
        uint16_t msgsize
    )
    {
        /**
         * get offset, read from buffer, send to FLI
         * set private variables describing the request
         * another thread breaks the request down into
         * several CMPL_Ds and waits for CMD_ACK_CMPL
         */

        uint64_t addr   = (uint64_t)readDWfromSock(m_sockfd)<<32;
        addr           += readDWfromSock(m_sockfd);
        uint32_t reqid  = readDWfromSock(m_sockfd);
        uint32_t param  = readDWfromSock(m_sockfd);

        t_read_req rdreq;
        rdreq.length       = ((param>>16) << 2);
        rdreq.tag          = ((param>>8) & 0xff);
        rdreq.byte_enable  = (param & 0xff);
        rdreq.lower_addr   = (addr & 0xff);
        rdreq.requester_id = reqid;

        //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "CMD_READ_FROM_HOST: %d bytes from 0x%lx, tag=%d\n",
        //        rdreq.length, addr, rdreq.tag);

        if( getOffset(addr, &(rdreq.buffer_id), &(rdreq.offset)) )
        {
            std::cout << "ERROR: Could not find physical address 0x"
                  << std::hex << addr << std::dec << std::endl;
            abort();
        }
        else
        {
            /** push request into pipe */
            if
            (
                sizeof(rdreq)
                    != write(m_pipefd[1], &rdreq, sizeof(rdreq))
            )
            {
                std::cout << "ERROR: Write to pipe failed with" << std::endl;
            }
        }
    }


    void
    bar_impl_sim::doAcknowledgeCompletion
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=2)
        {
            std::cout << "ERROR: Invalid message size for CMD_ACK_CMPL: "
                 << msgsize << std::endl;
        }
        m_cmpl_to_dev_done = 1;
    }


    void
    bar_impl_sim::doAcknowledgeWrite
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=2)
        {
            std::cout << "ERROR: Invalid message size for CMD_ACK_WRITE: "
                 << msgsize << std::endl;
        }
        m_write_to_dev_done = 1;
    }


    void
    bar_impl_sim::doAcknowledgeTime
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=4)
        {
            std::cout << "ERROR: Invalid message size for CMD_ACK_TIME: "
                 << msgsize << std::endl;
        }
        m_read_time_data = ((uint64_t)readDWfromSock(m_sockfd)<<32);
        m_read_time_data += readDWfromSock(m_sockfd);
        m_read_time_done = 1;
    }


    int
    bar_impl_sim::getOffset
    (
        uint64_t  phys_addr,
        uint64_t *buffer_id,
        uint64_t *offset
    )
    {
#ifdef PDA
        DMABuffer *first_buffer;

        if
        (
            PDA_SUCCESS !=
                PciDevice_getDMABuffer(m_pda_pci_device, 0, &first_buffer)
        )
        {
            return 1;
        }

        /** Iterate buffers */
        PdaDebugReturnCode ret = PDA_SUCCESS;
        for
        (
            DMABuffer *buffer = first_buffer;
            buffer != NULL;
            ret = DMABuffer_getNext(buffer, &buffer)
        )
        {
            *offset = 0;
            if(ret != PDA_SUCCESS)
            {
                return 1;
            }
            DMABuffer_SGNode *sglist;

            if( DMABuffer_getSGList(buffer, &sglist ) != PDA_SUCCESS )
            {
                return 1;
            }

            /** Iterate sglist entries of the current buffer */
            for
            (
                DMABuffer_SGNode *node = sglist;
                node != NULL;
                node = node->next
            )
            {
                if(   ( (uint64_t)(node->d_pointer) <= phys_addr)
                   && (((uint64_t)(node->d_pointer) + node->length) > phys_addr) )
                {
                    *offset += ( phys_addr - (uint64_t)(node->d_pointer) );
                    if(DMABuffer_getIndex(buffer, buffer_id) != PDA_SUCCESS)
                    {
                        return 1;
                    }
                    return 0;
                }
                else
                {
                    *offset += (node->length)/sizeof(uint64_t);
                }


            }
        }
        return 1;
#else
        sysfs_handler *hd = m_parent_dev->getHandler();
        std::vector<uint64_t> bufferlist = hd->list_all_buffers();
        std::vector<uint64_t>::iterator iter, end;
        iter = bufferlist.begin();
        end = bufferlist.end();
        while( iter != end )
        {
            buffer *buf = new buffer(m_parent_dev, *iter, 0);
            uint64_t buf_offset;
            if( buf->physAddrToOffset(phys_addr, &buf_offset)) {
                 *offset = buf_offset;
                 *buffer_id = *iter;
                 delete buf;
                 return 0;
            }                 
            delete buf;
            ++iter;
        }
        return 1;
#endif
    }

void
bar_impl_sim::simSetPacketSize
(
    uint32_t packet_size
)
{
    m_max_packet_size = packet_size;
}


void*
bar_impl_sim::cmplHandler()
{
    t_read_req rdreq;
    while( int result = read(m_pipefd[0], &rdreq, sizeof(t_read_req)) )
    {
        if(result<0)
        {
            std::cout << "ERROR: Failed to read from pipe: "
                 << result << std::endl;
            break;
        }

        /**
         * break request down into CMPL_Ds with size <= m_max_packet_size
         * wait for cmpl_done via CMD_CMPL_DONE from sock_monitor
         * after each packet
         */
        buffer *buf;
        try
        {
            buf = new buffer(m_parent_dev, rdreq.buffer_id, 1);
        }
        catch(...)
        {
            std::cout << "ERROR: Failed to connect to buffer "
                 << rdreq.buffer_id << std::endl;
            abort();
        }

        uint32_t *mem = buf->getMem();

        while ( rdreq.length )
        {
            uint32_t length     = 0;
            uint32_t byte_count = rdreq.length;

            if( rdreq.length>m_max_packet_size)
            {
                length = m_max_packet_size;
            }
            else
            {
                length = rdreq.length;
            }

            int32_t buffersize
                = 4*sizeof(uint32_t) + length;

            uint32_t buffer[((buffersize/sizeof(uint32_t))+1)];

            /** prepare CMD_CMPL_TO_DEVICE */
            buffer[0] = ((buffersize>>2)<<16) + CMD_CMPL_TO_DEVICE;
            buffer[2] = rdreq.requester_id;
            buffer[3] =   (0<<29)          /** cmpl_status */
                        + (byte_count<<16) /** remaining byte count */
                        + (rdreq.tag<<8)
                        + (rdreq.lower_addr);

            memcpy(&(buffer[4]), mem+(rdreq.offset>>2), length);

            pthread_mutex_lock(&m_mtx);
            {
                buffer[1] = m_msgid;
                /** send to FLI */
                result = write(m_sockfd, buffer, buffersize);
                m_msgid++;
            }
            pthread_mutex_unlock(&m_mtx);

            if( result!=buffersize )
            {
                std::cout << "ERROR: CMD_CMPL_TO_DEVICE write failed with "
                    << result << std::endl;
            }
            else
            {
                /** wait for FLI acknowledgement */
                while( !m_cmpl_to_dev_done )
                {
                    usleep(USLEEP_TIME);
                }
                m_cmpl_to_dev_done = 0;

                //DEBUG_PRINTF(PDADEBUG_EXTERNAL,
                //        "CMD_CMPL_TO_DEVICE tag=%d, length=%d B, buffer=%ld, offset=0x%x\n",
                //        rdreq.tag, length, rdreq.buffer_id, (rdreq.offset>>2));
            }

            rdreq.length     -= length;
            rdreq.offset     += length;
            rdreq.lower_addr += length;
            rdreq.lower_addr &= 0x7f; /** only lower 7 bit */
        }
        delete buf;
    }

    //DEBUG_PRINTF(PDADEBUG_EXTERNAL, "Pipe has been closed, cmpl_handler stopping.\n");

    return 0;
}



size_t
bar_impl_sim::size()
{
    return m_size;
}

}

