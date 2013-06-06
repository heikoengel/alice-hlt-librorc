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
 */

#include "sim_bar.hh"

#include "rorcfs_bar_includes.hh"

#include "mti.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pda.h>

//#include "rorcfs_bar.hh"
#include "rorcfs_device.hh"
#include "rorcfs_dma_channel.hh"
#include <librorc_registers.h>

using namespace std;

/**
 * usleep time for FLI read polling
 * */
#define USLEEP_TIME 50



sim_bar::sim_bar
(
    rorcfs_device *dev,
    int            n
)

{
    m_parent_dev = dev;
    m_number     = n;

    m_pda_pci_device = dev->getPdaPciDevice();

    /** initialize mutex */
    pthread_mutex_init(&m_mtx, NULL);

    read_from_dev_done = 0;
    write_to_dev_done  = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 )
    {
        perror("ERROR opening socket");
    }

    struct hostent *server = gethostbyname(MODELSIM_SERVER);
    if( server == NULL )
    {
        perror("ERROR, no sich host");
    }

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy( (char *)server->h_addr,
           (char *)&serv_addr.sin_addr.s_addr,
           server->h_length
         );
    serv_addr.sin_port = htons(2000);
    if( 0 > connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) )
    {
        perror("ERROR connecting");
    }

    /** create pipe for redirecting read requests to completion handler
     *  pipefd[0]: read end
     *  pipefd[1]: write end
     */
    if( pipe(pipefd) == -1 )
    {
        perror("Failed to create PIPE");
    }

    /** completion handler */
    pthread_create(&cmpl_handler_p, NULL, cmpl_handler_helper, this);

    /** watch incoming packets */
    pthread_create(&sock_mon_p, NULL, sock_monitor_helper, this);
    msgid = 0;
}



sim_bar::~sim_bar()
{
    close(pipefd[0]);
    close(pipefd[1]);
    pthread_cancel(sock_mon_p);
    pthread_cancel(cmpl_handler_p);
    close(sockfd);

    pthread_mutex_destroy(&m_mtx);
    /** Further stuff here **/
}



int
sim_bar::init()
{
    return 0;
}



unsigned int
sim_bar::get
(
    unsigned long addr
)
{
    uint32_t  data = 0;
    DEBUG_PRINTF("get(0x%lx)\n", addr);
    pthread_mutex_lock(&m_mtx);
    {
        int32_t  buffersize = 4;
        uint32_t buffer[buffersize];
        buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
        buffer[1] = msgid;
        buffer[2] = addr<<2;
        /** BAR, BE, length */
        buffer[3] = (m_number<<24) + (0x0f<<16) + 1;

        if
        (
            write( sockfd, buffer, buffersize*sizeof(uint32_t) )
                != (size_t)(buffersize*sizeof(uint32_t))
        )
        {
            cout << "ERROR writing to socket" << endl;
        }
        else
        {
            /** wait for completion */
            while( !read_from_dev_done )
            {
                usleep(USLEEP_TIME);
            }

            data = read_from_dev_data;
            read_from_dev_done = 0;

            msgid++;
        }
    }
    pthread_mutex_unlock(&m_mtx);
    return data;
}



void
sim_bar::set
(
    unsigned long addr,
    unsigned int  data
)
{
    /** send write command to Modelsim FLI server */
    pthread_mutex_lock(&m_mtx);
    {
        int32_t  buffersize = 5;
        uint32_t buffer[buffersize];
        buffer[0] = (5<<16) + CMD_WRITE_TO_DEVICE;
        buffer[1] = msgid;
        buffer[2] = addr<<2;
        buffer[3] = (m_number<<24) + (0x0f<<16) + 1; //BAR, BE, length
        buffer[4] = data;

        if
        (
            write( sockfd, buffer, buffersize*sizeof(uint32_t) )
                != (size_t)(buffersize*sizeof(uint32_t))
        )
        {
            cout << "ERROR writing to socket" << endl;
        }
        else
        {
            /** wait for FLI acknowledgement */
            while( !write_to_dev_done )
            {
                usleep(USLEEP_TIME);
            }
            write_to_dev_done=0;
            msgid++;
        }
    }
    pthread_mutex_unlock(&m_mtx);
}



void
sim_bar::memcpy_bar
(
    unsigned long addr,
    const void   *source,
    size_t        num
)
{
    pthread_mutex_lock(&m_mtx);
    {
        size_t   ndw = num>>2;
        int32_t  buffersize = 4 + ndw;
        uint32_t buffer[buffersize];
        buffer[0] = ((ndw+4)<<16) + CMD_WRITE_TO_DEVICE;
        buffer[1] = msgid;
        buffer[2] = addr<<2;
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

        if
        (
            write( sockfd, buffer, buffersize*sizeof(uint32_t) )
                != (size_t)(buffersize*sizeof(uint32_t))
        )
        {
            cout << "ERROR writing to socket" << endl;
        }
        else
        {
            /** wait for FLI acknowledgement */
            while( !write_to_dev_done )
            {
                usleep(USLEEP_TIME);
            }
            write_to_dev_done=0;
            msgid++;
        }

    }
    pthread_mutex_unlock(&m_mtx);
}



unsigned short
sim_bar::get16
(
    unsigned long addr
)
{
    uint16_t data = 0;
    pthread_mutex_lock(&m_mtx);
    {
        int32_t  buffersize = 4;
        uint32_t buffer[buffersize];
        buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
        buffer[1] = msgid;
        buffer[2] = (addr<<1) & ~(0x03);
        if ( addr & 0x01 )
        {
            /** BAR, BE, length */
            buffer[3] = (m_number<<24) + (0x0c<<16) + 1;
        }
        else
        {
            /** BAR, BE, length */
            buffer[3] = (m_number<<24) + (0x03<<16) + 1;
        }

        if
        (
            write(sockfd, buffer, buffersize*sizeof(uint32_t))
                != (size_t)(buffersize*sizeof(uint32_t))
        )
        {
            cout << "ERROR writing to socket" << endl;
        }
        else
        {
            /** wait for FLI completion */
            while( !read_from_dev_done )
            {
                usleep(USLEEP_TIME);
            }
            data = read_from_dev_data & 0xffff;
            read_from_dev_done = 0;
            msgid++;
        }
    }
    pthread_mutex_unlock(&m_mtx);

    return data;
}



void
sim_bar::set16
(
    unsigned long  addr,
    unsigned short data
)
{
    /** send write command to Modelsim FLI server */
    pthread_mutex_lock(&m_mtx);
    {
        int buffersize = 5;
        uint32_t buffer[buffersize];
        buffer[0] = (5<<16) + CMD_WRITE_TO_DEVICE;
        buffer[1] = msgid;
        buffer[2] = (addr<<1) & ~(0x03);
        if ( addr & 0x01 )
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

        if
        (
            write(sockfd, buffer, buffersize*sizeof(uint32_t))
                != (size_t)(buffersize*sizeof(uint32_t))
        )
        {
            cout << "ERROR writing to socket" << endl;
        }
        else
        {
            /** wait for FLI acknowledgement */
            while( !write_to_dev_done )
            {
                usleep(USLEEP_TIME);
            }
            write_to_dev_done=0;
            msgid++;
        }
    }
    pthread_mutex_unlock(&m_mtx);
}



int sim_bar::gettime(struct timeval *tv, struct timezone *tz)
{
    pthread_mutex_lock(&m_mtx);
    {
        int32_t  buffersize = 2;
        uint32_t buffer[buffersize];
        buffer[0] = (2<<16) + CMD_GET_TIME;
        buffer[1] = msgid;

        if( 0 > write(sockfd, &buffer, buffersize*sizeof(uint32_t)) )
        {
            cout << "ERROR writing to socket" << endl;
        }

        /** wait for FLI completion */
        while( !read_from_dev_done )
        {
            usleep(USLEEP_TIME);
        }

        uint32_t data = read_from_dev_data;
        read_from_dev_done = 0;

        /** mti_Now is in [ps] */
        tv->tv_sec  = (data/1000/1000/1000/1000);
        tv->tv_usec = (data/1000/1000)%1000000;
        msgid++;
    }
    pthread_mutex_unlock(&m_mtx);

    return 0;
}



void*
sim_bar::sockMonitor()
{
    uint32_t tmpvar = 0;
    while( (tmpvar=readDWfromSock(sockfd)) )
    {
        uint16_t msgsize = (tmpvar >> 16) & 0xffff;
        msgid            = readDWfromSock(sockfd);

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
    sim_bar::readDWfromSock
    (
        int sock
    )
    {
        int result = 0;
        uint32_t buffer;

        result = read(sock, &buffer, sizeof(uint32_t)); // read 1 DW
        if (result == 0)
        {
            /** terminate if 0 characters received */
            cout << "rorcfs_bar::readDWfromSock: closing socket" << endl;
            close(sock);
        }
        else if (result!=sizeof(uint32_t))
        {
            cout << "ERROR: rorcfs_bar::readDWfromSock returned "
                 << result << " bytes" << endl;
        }

        return buffer;
    }


    void
    sim_bar::doCompleteToHost
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=4)
        {
            cout << "Invalid message size for CMD_CMPL_TO_HOST : "
                 << msgsize << endl;
        }

        readDWfromSock(sockfd);
        read_from_dev_data = readDWfromSock(sockfd);
        read_from_dev_done = 1;
    }


    void
    sim_bar::doWriteToHost
    (
        uint16_t msgsize
    )
    {
        /**
         * get_offset, write into buffer
         * [addr_high][addr_low][payload]
         */
        uint64_t addr  = (uint64_t)readDWfromSock(sockfd)<<32;
        addr          += readDWfromSock(sockfd);
        msgsize       -= 4;

        uint32_t buffer[msgsize];
        for(uint64_t i=0; i<msgsize; i++)
        {
            buffer[i] = readDWfromSock(sockfd);
        }

        uint64_t offset    = 0;
        uint64_t buffer_id = 0;
        if( getOffset(addr, &buffer_id, &offset) )
        {
            cout << "Could not find physical address "
                 << addr << endl;
        }
        else
        {
            rorcfs_buffer *buf = new rorcfs_buffer();
            if( buf->connect(m_parent_dev, buffer_id) )
            {
                perror("failed to connect to buffer");
            }
            else
            {
                uint32_t *mem = buf->getMem();
                memcpy(mem+(offset>>2), buffer, msgsize*sizeof(uint32_t));
                cout << "CMD_WRITE_TO_HOST: " << msgsize << " DWs to buf "
                     << buffer_id << " offset " << offset << endl;
            }
            delete buf;
        }
    }


    void
    sim_bar::doReadFromHost
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

        uint64_t addr   = (uint64_t)readDWfromSock(sockfd)<<32;
        addr           += readDWfromSock(sockfd);
        uint32_t reqid  = readDWfromSock(sockfd);
        uint32_t param  = readDWfromSock(sockfd);

        t_read_req rdreq;
        rdreq.length       = ((param>>16) << 2);
        rdreq.tag          = ((param>>8) & 0xff);
        rdreq.byte_enable  = (param & 0xff);
        rdreq.lower_addr   = (addr & 0xff);
        rdreq.requester_id = reqid;

        cout << "sock_monitor: CMD_READ_FROM_HOST " << param << endl;

        if( getOffset(addr, &(rdreq.buffer_id), &(rdreq.offset)) )
        {
            cout << "Could not find physical address " << addr << endl;
        }
        else
        {
            /** push request into pipe */
            if
            (
                sizeof(rdreq)
                    != write(pipefd[1], &rdreq, sizeof(rdreq))
            )
            {
                cout << "Write to pipe failed with" << endl;
            }
        }
    }


    void
    sim_bar::doAcknowledgeCompletion
    (
        uint16_t msgsize
    )
    {
        cout << "sock_monitor: CMD_ACK_CMPL" << endl;
        if (msgsize!=2)
        {
            cout << "Invalid message size for CMD_ACK_CMPL: "
                 << msgsize << endl;
        }
        cmpl_to_dev_done = 1;
    }


    void
    sim_bar::doAcknowledgeWrite
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=2)
        {
            cout << "Invalid message size for CMD_ACK_WRITE: "
                 << msgsize << endl;
        }
        write_to_dev_done = 1;
    }


    void
    sim_bar::doAcknowledgeTime
    (
        uint16_t msgsize
    )
    {
        if (msgsize!=3)
        {
            cout << "Invalid message size for CMD_ACK_TIME: "
                 << msgsize << endl;
        }
        read_from_dev_data = readDWfromSock(sockfd);
        read_from_dev_done = 1;
    }


    int
    sim_bar::getOffset
    (
        uint64_t  phys_addr,
        uint64_t *buffer_id,
        uint64_t *offset
    )
    {
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
        *offset = 0;
        PdaDebugReturnCode ret = PDA_SUCCESS;
        for
        (
            DMABuffer *buffer = first_buffer;
            buffer != NULL;
            ret = DMABuffer_getNext(buffer, &buffer)
        )
        {
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
                    *offset += (node->length);
                }
            }
        }

        return 1;
    }


void*
sim_bar::cmplHandler()
{
    int result;
    t_read_req rdreq;
    while( (result=read(pipefd[0], &rdreq, sizeof(t_read_req))) )
    {
        if(result<0)
        {
            cout << "Failed to read from pipe: "
                 << result << endl;
            break;
        }

        /**
         * break request down into CMPL_Ds with size <= MAX_PAYLOAD
         * wait for cmpl_done via CMD_CMPL_DONE from sock_monitor
         * after each packet
         */

        rorcfs_buffer *buf = new rorcfs_buffer();
        if( buf->connect(m_parent_dev, rdreq.buffer_id) )
        {
            cout << "Failed to connect to buffer "
                 << rdreq.buffer_id << endl;
        }

        uint32_t *mem = buf->getMem();

        while ( rdreq.length )
        {
            uint32_t length     = 0;
            uint32_t byte_count = 0;
            if( rdreq.length>MAX_PAYLOAD )
            {
                byte_count = rdreq.length - MAX_PAYLOAD;
                length = MAX_PAYLOAD;
            }
            else
            {
                byte_count = 0;
                length = rdreq.length;
            }

            int buffersize
                = 4*sizeof(uint32_t) + length;

            uint32_t buffer[((buffersize/sizeof(uint32_t))+1)];

            /** prepare CMD_CMPL_TO_DEVICE */
            buffer[0] = ((buffersize>>2)<<16) + CMD_CMPL_TO_DEVICE;
            buffer[1] = msgid;
            buffer[2] = rdreq.requester_id;
            buffer[3] =   (0<<29)          /** cmpl_status */
                        + (byte_count<<16) /** remaining byte count */
                        + (rdreq.tag<<8)
                        + (rdreq.lower_addr);

            memcpy(&(buffer[4]), mem+(rdreq.offset>>2), length);

            pthread_mutex_lock(&m_mtx);
            {
                /** send to FLI */
                result = write(sockfd, buffer, buffersize);
                if( result!=buffersize )
                {
                    cout << "CMD_CMPL_TO_DEVICE write failed with "
                         << result;
                }
                else
                {
                    /** wait for FLI acknowledgement */
                    while( !cmpl_to_dev_done )
                    {
                        usleep(USLEEP_TIME);
                    }
                    cmpl_to_dev_done = 0;

                    cout << "cmpl_handler: CMD_CMPL_TO_DEVICE: tag=" << rdreq.tag
                         << ", length=" << length << "buffer=" << rdreq.buffer_id
                         << "offset=" << (rdreq.offset>>2) << "MP=" << MAX_PAYLOAD
                         << "Req:" << rdreq.length << endl;
                }
            }
            pthread_mutex_unlock(&m_mtx);

            rdreq.length     -= length;
            rdreq.offset     += length;
            rdreq.lower_addr += length;
            rdreq.lower_addr &= 0x7f; /** only lower 6 bit */
        }
        delete buf;
    }

    cout << "Pipe has been closed, cmpl_handler stopping." << endl;

    return 0;
}



size_t
sim_bar::getSize()
{
    return m_size;
}
