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

#ifndef _RORCLIB_SIM_BAR_H
#define _RORCLIB_SIM_BAR_H

#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>

#include "rorcfs_device.hh"
#include "rorcfs_bar.hh"

class sim_bar : public rorcfs_bar
{
public:
    sim_bar
    (
        rorcfs_device *dev,
        int            n
    );

    ~sim_bar();
    int init();

    unsigned int
    get
    (
        unsigned long addr
    );

    void
    set
    (
        unsigned long addr,
        unsigned int  data
    );

    void
    memcpy_bar
    (
        unsigned long addr,
        const void   *source,
        size_t        num
    );

    unsigned short
    get16
    (
        unsigned long addr
    );

    void
    set16
    (
        unsigned long  addr,
        unsigned short data
    );

    int
    gettime
    (
        struct timeval *tv,
        struct timezone *tz
    );

    void *sock_monitor();
    void *cmpl_handler();

private:
    int sockfd;
    int pipefd[2];
    int msgid;
    uint32_t read_from_dev_data;
    uint32_t read_from_dev_done;
    uint32_t write_to_dev_done;
    uint32_t cmpl_to_dev_done;

    pthread_t sock_mon_p;
    pthread_t cmpl_handler_p;



    static void *sock_monitor_helper(void * This)
    {
        ((sim_bar *)This)->sock_monitor();
        return 0;
    }

    static void *cmpl_handler_helper(void * This)
    {
        ((sim_bar *)This)->cmpl_handler();
        return 0;
    }


};



typedef struct
{
	uint64_t buffer_id;
	uint64_t offset;
	uint16_t length;
	uint8_t  tag;
	uint8_t  lower_addr;
	uint8_t  byte_enable;
	uint32_t requester_id;
} t_read_req;

typedef struct
{
	int          wr_ack;
	unsigned int data;
	int          id;
} flimsgT;

typedef struct
{
	unsigned int   addr;
	unsigned int   bar;
	unsigned char  byte_enable;
	unsigned char  type;
	unsigned short len;
	int id;
} flicmdT;


/**
 * FLI_CMD bit definitions
 * */
#define CMD_READ_FROM_DEVICE 1
#define CMD_WRITE_TO_DEVICE 2
#define CMD_GET_TIME 3
#define CMD_CMPL_TO_DEVICE 4
#define CMD_READ_FROM_HOST 5
#define CMD_WRITE_TO_HOST 6
#define CMD_CMPL_TO_HOST 7
#define CMD_ACK_TIME 8
#define CMD_ACK_WRITE 9
#define CMD_ACK_CMPL 10

#endif
