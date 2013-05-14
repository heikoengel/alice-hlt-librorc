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



/**
 * rorcfs_bar::init()
 * Initialize and mmap BAR
 * */

int
sim_bar::init()
{
    return 0;
}



///**
// * read 1 DW from BAR
// * @param addr address (DW-aligned)
// * @return value read from BAR
// * */
//
//unsigned int
//rorcfs_bar::get
//(
//    unsigned long addr
//)
//{
//    assert( m_bar != NULL );
//
//    unsigned int *bar = (unsigned int *)m_bar;
//    unsigned int result;
//    if( (addr << 2) < m_size)
//    {
//        result = bar[addr];
//        return result;
//    }
//    else
//    {
//        return -1;
//    }
//}
//
//
//
///**
// * write 1 DW to BAR
// * @param addr DW-aligned address
// * @param data DW data to be written
// * */
//
//void
//rorcfs_bar::set
//(
//    unsigned long addr,
//    unsigned int  data
//)
//{
//    assert( m_bar != NULL );
//    unsigned int *bar = (unsigned int *)m_bar;
//    if( (addr << 2) < m_size)
//    {
//        pthread_mutex_lock(&m_mtx);
//        bar[addr] = data;
//        msync( (bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
//        pthread_mutex_unlock(&m_mtx);
//    }
//}
//
//
//
///**
// * copy a buffer to BAR via memcpy
// * @param addr DW-aligned address
// * @param source source buffer
// * @param num number of bytes to be copied from source to dest
// * */
//
//void
//rorcfs_bar::memcpy_bar
//(
//    unsigned long addr,
//    const void   *source,
//    size_t        num
//)
//{
//    pthread_mutex_lock(&m_mtx);
//    memcpy( (unsigned char*)m_bar + (addr << 2), source, num);
//    msync( (m_bar + ( (addr << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);
//    pthread_mutex_unlock(&m_mtx);
//}
//
//
//
//unsigned short
//rorcfs_bar::get16
//(
//    unsigned long addr
//)
//{
//    unsigned short *sbar;
//    sbar = (unsigned short*)m_bar;
//    unsigned short result;
//    assert( sbar != NULL );
//    if( (addr << 1) < m_size)
//    {
//        result = sbar[addr];
//        return result;
//    }
//    else
//    {
//        return 0xffff;
//    }
//}
//
//
//
//void
//rorcfs_bar::set16
//(
//    unsigned long  addr,
//    unsigned short data
//)
//{
//    unsigned short *sbar;
//    sbar = (unsigned short*)m_bar;
//
//    assert( sbar != NULL );
//    if( (addr << 1) < m_size)
//    {
//        pthread_mutex_lock(&m_mtx);
//        sbar[addr] = data;
//        msync( (sbar + ( (addr << 1) & PAGE_MASK) ),
//               PAGE_SIZE, MS_SYNC);
//        pthread_mutex_unlock(&m_mtx);
//    }
//}
