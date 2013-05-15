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

#include "rorcfs_device.hh"
#include "rorcfs_dma_channel.hh"
#include <librorc_registers.h>

using namespace std;

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

uint32_t
readDWfromSock
(
    int sock
);


sim_bar::sim_bar
(
    rorcfs_device *dev,
    int            n
)
: rorcfs_bar(dev, n)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
//    struct in_addr ipaddr;
//    int statusFlags;

    read_from_dev_done = 0;
    write_to_dev_done = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 )
    {
        perror("ERROR opening socket");
    }

    server = gethostbyname(MODELSIM_SERVER);
//    inet_pton(AF_INET, "10.0.52.10", &ipaddr);
//    server = gethostbyaddr(&ipaddr, sizeof(ipaddr), AF_INET);
    if( server == NULL )
    {
        perror("ERROR, no sich host");
    }

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
    pthread_mutex_lock(&m_mtx);
    {
        int32_t  buffersize = 4;
        uint32_t buffer[buffersize];
        buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
        buffer[1] = msgid;
        buffer[2] = addr<<2;
        buffer[3] = (m_number<<24) + (0x0f<<16) + 1; //BAR, BE, length

        if
        (
            write( sockfd, buffer, buffersize*sizeof(uint32_t) )
                != buffersize*sizeof(uint32_t)
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
                != buffersize*sizeof(uint32_t)
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
        int32_t  buffersize = 4;
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
                != buffersize*sizeof(uint32_t)
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
                != buffersize*sizeof(uint32_t)
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
                != buffersize*sizeof(uint32_t) )
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


/** Functions*/

uint32_t
readDWfromSock
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
