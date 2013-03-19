/**
 * @file rorcfs_bar.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
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

#include <iostream>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/time.h>
#ifdef SIM
#include "mti.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#endif
#include <dirent.h>
#include <sys/wait.h>

#include "rorcfs.h"
#include "rorcfs_bar.hh"
#include "rorcfs_device.hh"
#include "rorcfs_buffer.hh"
#include "rorcfs_dma_channel.hh"
#include "rorc_registers.h"

/**
 * usleep time for FLI read polling
 * */
#define USLEEP_TIME 50


/** Prototypes **/
char *getChOff(unsigned int addr);
int get_offset( uint64_t phys_addr, uint64_t *buffer_id, uint64_t *offset );


/**
 * scandir_filter:
 * return nonzero if a directory is found
 **/
static int scandir_filter(const struct dirent* entry) {
  if( entry->d_type == DT_DIR && strncmp(entry->d_name, ".", 1)!=0 )
    return 1;
  else
    return 0;
}



/**
 * rorcfs_bar::init()
 * Initialize and mmap BAR
 * */
int rorcfs_bar::init()
{
#ifndef SIM
  handle = open(fname, O_RDWR);
  if ( handle == -1 )
    return handle;

  if ( fstat(handle, &barstat) == -1 )
    return -1;

  bar = (unsigned int*) mmap(0, barstat.st_size, 
      PROT_READ|PROT_WRITE, MAP_SHARED, handle, 0);
  if ( bar == MAP_FAILED )
    return -1;
#endif
  return 0;
}


/**
 * read 1 DW from BAR
 * @param addr address (DW-aligned)
 * @return value read from BAR
 * */
unsigned int rorcfs_bar::get(unsigned long addr) {
#ifndef SIM
  unsigned int result;
  assert ( bar!=NULL );
  if ( (addr<<2)<barstat.st_size ) {
    result = bar[addr];
#ifdef DEBUG
    printf("rorcfs_bar::get(%lx)=%08x\n", addr, result);
    fflush(stdout);
#endif

    return result;
  } else
    return -1;
#else

  int n;	
  uint32_t *buffer;
  uint32_t data = 0;

  buffer = (uint32_t *)malloc(4*sizeof(uint32_t));
  if(buffer==NULL) 
  {
    perror("malloc CMD_READ_FROM_DEVICE buffer");
    pthread_mutex_unlock(&mtx);
    return 0;
  }

  buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
  buffer[1] = msgid;
  buffer[2] = addr<<2;
  buffer[3] = (number<<24) + (0x0f<<16) + 1; //BAR, BE, length

  pthread_mutex_lock(&mtx);

  n = write(sockfd, buffer, 4*sizeof(uint32_t));
  if ( n != 4*sizeof(uint32_t) )
    printf("ERROR writing to socket: %d\n", n);
  else {

    // wait for completion
    while( !read_from_dev_done )
      usleep(USLEEP_TIME);

    data = read_from_dev_data;
    read_from_dev_done = 0;

#ifdef DEBUG
    printf("%d: rorcfs_bar::get(%lx)=%08x %s\n", 
        msgid, addr, data, getChOff(addr));
    fflush(stdout);
#endif

    msgid++;
  }

  free(buffer);
  pthread_mutex_unlock(&mtx);
  return data;
#endif
}


/**
 * write 1 DW to BAR
 * @param addr Dw-aligned address
 * @param data DW data to be written
 * */
void rorcfs_bar::set(unsigned long addr, unsigned int data)
{

#ifndef SIM
  // access mmap'ed BAR region
  assert ( bar!=NULL );
  if ( (addr<<2)<barstat.st_size ) {
    pthread_mutex_lock(&mtx);
    bar[addr] = data;
    msync( (bar + ((addr<<2) & PAGE_MASK)), PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&mtx);
#ifdef DEBUG
    printf("rorcfs_bar::set(%lx, %08x)\n", addr, data);
    fflush(stdout);
#endif
  }

#else
  // semd write command to Modelsim FLI server
  unsigned int *buffer;
  int n, buffersize;

  pthread_mutex_lock(&mtx);

  buffersize = 5*sizeof(uint32_t);
  buffer = (uint32_t *)malloc(buffersize);
  if(buffer==NULL) 
  {
    perror("malloc CMD_WRITE_TO_DEVICE buffer");
  } else {

    buffer[0] = (5<<16) + CMD_WRITE_TO_DEVICE;
    buffer[1] = msgid;
    buffer[2] = addr<<2;
    buffer[3] = (number<<24) + (0x0f<<16) + 1; //BAR, BE, length
    buffer[4] = data;

    n = write(sockfd, buffer, buffersize);
    if ( n != buffersize )
      printf("ERROR writing to socket: %d\n", n);
    else {

      // wait for FLI acknowledgement
      while( !write_to_dev_done )
        usleep(USLEEP_TIME);
      write_to_dev_done=0;

#ifdef DEBUG
      printf("%d: rorcfs_bar::set(%lx, %08x) %s\n", 
          msgid, addr, data, getChOff(addr));
      fflush(stdout);
#endif

      msgid++;
    }
    free(buffer);
  }
  pthread_mutex_unlock(&mtx);
#endif
}


/**
 * copy a buffer to BAR via memcpy
 * @param addr DW-aligned address
 * @param source source buffer
 * @param num number of bytes to be copied from source to dest
 * */
void rorcfs_bar::memcpy_bar(unsigned long addr, const void *source, size_t num) {
#ifndef SIM
#ifdef DEBUG
  printf("rorcfs_bar:memcpy_bar(%lx, %p, %ld)\n", addr, source, num);
#endif
  pthread_mutex_lock(&mtx);
  memcpy((unsigned char *)bar + (addr<<2), source, num);
  msync( (bar + ((addr<<2) & PAGE_MASK)), PAGE_SIZE, MS_SYNC);
  pthread_mutex_unlock(&mtx);
#else

  uint32_t *buffer;
  int n, buffersize;
  size_t ndw = num>>2;

  buffersize = 4*sizeof(uint32_t) + num;
  buffer = (uint32_t *)malloc(buffersize);
  if(buffer==NULL) 
  {
    perror("malloc CMD_WRITE_TO_DEVICE buffer");
  }	else {
    buffer[0] = ((ndw+4)<<16) + CMD_WRITE_TO_DEVICE;
    buffer[1] = msgid;
    buffer[2] = addr<<2;
    if(ndw>1)
      buffer[3] = (number<<24) + (0xff<<16) + ndw; //BAR, BE, length
    else
      buffer[3] = (number<<24) + (0x0f<<16) + ndw; //BAR, BE, length
    memcpy(&(buffer[4]), source, num);

    n = write(sockfd, buffer, buffersize);
    if ( n != buffersize )
      printf("ERROR writing to socket: %d\n", n);
    else {

      // wait for FLI acknowledgement
      while( !write_to_dev_done )
        usleep(USLEEP_TIME);
      write_to_dev_done=0;

#ifdef DEBUG
      printf("%d: rorcfs_bar::memcpy() %ld bytes (%ld DWs) "
          "starting at %lx\n", 
          msgid, num, ndw, addr);
      fflush(stdout);
#endif

      msgid++;
    }
    free(buffer);
  }
  pthread_mutex_unlock(&mtx);
#endif
}


unsigned short rorcfs_bar::get16(unsigned long addr) 
{
#ifndef SIM
  unsigned short *sbar;
  sbar = (unsigned short *)bar;
  unsigned short result;
  assert ( sbar!=NULL );
  if ( (addr<<1)<barstat.st_size ) {
    result = sbar[addr];
#ifdef DEBUG
    printf("rorcfs_bar::get16(%lx)=%04x\n", addr, result);
    fflush(stdout);
#endif

    return result;
  }
  else
    return 0xffff;
#else

  pthread_mutex_lock(&mtx);

  int n;	
  uint32_t *buffer;
  uint16_t data = 0;

  buffer = (uint32_t *)malloc(4*sizeof(uint32_t));
  if(buffer==NULL) 
  {
    perror("malloc CMD_READ_FROM_DEVICE buffer");
    pthread_mutex_unlock(&mtx);
    return 0;
  }

  buffer[0] = (4<<16) + CMD_READ_FROM_DEVICE;
  buffer[1] = msgid;
  buffer[2] = (addr<<1) & ~(0x03);
  if ( addr & 0x01 )
    buffer[3] = (number<<24) + (0x0c<<16) + 1; //BAR, BE, length
  else
    buffer[3] = (number<<24) + (0x03<<16) + 1; //BAR, BE, length

  n = write(sockfd, buffer, 4*sizeof(uint32_t));
  if ( n != 4*sizeof(uint32_t) )
    printf("ERROR writing to socket: %d\n", n);
  else {

    // wait for completion
    while( !read_from_dev_done )
      usleep(USLEEP_TIME);

    data = read_from_dev_data & 0xffff;
    read_from_dev_done = 0;

#ifdef DEBUG
    printf("%d: rorcfs_bar::get16(%lx)=%08x\n", 
        msgid, addr, data);
    fflush(stdout);
#endif

    msgid++;
  }

  free(buffer);
  pthread_mutex_unlock(&mtx);
  return data;

#endif
}



void rorcfs_bar::set16(unsigned long addr, unsigned short data) 
{
#ifndef SIM	
  unsigned short *sbar;
  sbar = (unsigned short *)bar;
  // access mmap'ed BAR region
  assert ( sbar!=NULL );
  if ( (addr<<1)<barstat.st_size ) {
    pthread_mutex_lock(&mtx);
    sbar[addr] = data;
    msync( (sbar + ((addr<<1) & PAGE_MASK)), 
        PAGE_SIZE, MS_SYNC);
    pthread_mutex_unlock(&mtx);
#ifdef DEBUG
    printf("rorcfs_bar::set16(%lx, %04x)\n", addr, data);
    fflush(stdout);
#endif
  }

#else

  // semd write command to Modelsim FLI server
  unsigned int *buffer;
  int n, buffersize;

  pthread_mutex_lock(&mtx);

  buffersize = 5*sizeof(uint32_t);
  buffer = (uint32_t *)malloc(buffersize);
  if(buffer==NULL) 
  {
    perror("malloc CMD_WRITE_TO_DEVICE buffer");
  } else {

    buffer[0] = (5<<16) + CMD_WRITE_TO_DEVICE;
    buffer[1] = msgid;
    buffer[2] = (addr<<1) & ~(0x03);
    if ( addr & 0x01 )
      buffer[3] = (number<<24) + (0x0c<<16) + 1; //BAR, BE, length
    else
      buffer[3] = (number<<24) + (0x03<<16) + 1; //BAR, BE, length
    buffer[4] = (data<<16) + data;

    n = write(sockfd, buffer, buffersize);
    if ( n != buffersize )
      printf("ERROR writing to socket: %d\n", n);
    else {

      // wait for FLI acknowledgement
      while( !write_to_dev_done )
        usleep(USLEEP_TIME);
      write_to_dev_done=0;

#ifdef DEBUG
      printf("%d: rorcfs_bar::set16(%lx, %04x) %s\n", 
          msgid, addr, data, getChOff(addr));
      fflush(stdout);
#endif

      msgid++;
    }
    free(buffer);
  }
  pthread_mutex_unlock(&mtx);
#endif
}


int rorcfs_bar::gettime(struct timeval *tv, struct timezone *tz)
{
#ifndef SIM
  return gettimeofday(tv, tz);
#else
  int n;
  uint32_t buffer[2];
  uint32_t data;

  pthread_mutex_lock(&mtx);

  buffer[0] = (2<<16) + CMD_GET_TIME;
  buffer[1] = msgid;

  n = write(sockfd, &buffer, 2*sizeof(uint32_t));
  if ( n < 0 )
    perror("ERROR writing to socket");

  // wait for completion
  while( !read_from_dev_done )
    usleep(USLEEP_TIME);

  data =read_from_dev_data;
  read_from_dev_done = 0;

  /*#ifdef DEBUG
    printf("rorcfs_bar::gettime=%d\n", data );
    fflush(stdout);
#endif*/

  //mti_Now is in [ps]
  tv->tv_sec = (data/1000/1000/1000/1000);
  tv->tv_usec = (data/1000/1000)%1000000;

  msgid++;

  pthread_mutex_unlock(&mtx);

  return 0;
#endif
}

char *getChOff(unsigned int addr) {
  int channel = (addr>>15)&0xf;
  int comp = (addr>>RORC_DMA_CMP_SEL) & 0x01;
  int offset = addr & ((1<<RORC_DMA_CMP_SEL)-1);
  char *buffer = (char *)malloc(256);
  sprintf(buffer, "ch:%d comp:%d off:%d", 
      channel, comp, offset);
  return buffer;
}


#ifdef SIM
uint32_t readDWfromSock(int sock)
{
  int result = 0;
  uint32_t buffer;

  result = read(sock, &buffer, sizeof(uint32_t)); // read 1 DW
  if (result == 0)  {   // terminate if 0 characters received
    printf("rorcfs_bar::readDWfromSock: closing socket\n");
    close(sock);
  } else if (result!=sizeof(uint32_t)) {
    printf("ERROR: rorcfs_bar::readDWfromSock returned %d bytes\n", result);
  }
  return buffer;
}



/**
 * handle all incoming data on the socket
 *
 * */
void *rorcfs_bar::sock_monitor()
{
  int i;
  int result;
  uint16_t msgsize;
  uint16_t cmd;
  uint32_t tmpvar;
  uint64_t addr;
  uint32_t param;
  uint32_t reqid;
  uint32_t *buffer;
  uint64_t offset, buffer_id;
  rorcfs_buffer *buf = NULL;
  uint32_t *mem;
  t_read_req rdreq;

  while( (tmpvar = readDWfromSock(sockfd)) )
  {
    cmd = tmpvar & 0xffff;
    msgsize = (tmpvar >> 16) & 0xffff;
    msgid = readDWfromSock(sockfd);

    switch (cmd) {
      case CMD_CMPL_TO_HOST:

        // [msgsize,cmd][msgID]
        if (msgsize!=4)
          printf("Invalid message size for CMD_CMPL_TO_HOST:%d\n", 
              msgsize);
        param = readDWfromSock(sockfd);
        read_from_dev_data = readDWfromSock(sockfd);
        read_from_dev_done = 1;
        break;

      case CMD_WRITE_TO_HOST:
        // get_offset, write into buffer
        // [addr_high][addr_low][payload]
        addr = (uint64_t)readDWfromSock(sockfd)<<32;
        addr += readDWfromSock(sockfd);
        msgsize -= 4;

        buffer = (uint32_t *)malloc(msgsize*sizeof(uint32_t));
        if(!buffer)
          perror("failed to allocate CMD_WRITE_TO_HOST buffer");

        for(i=0;i<msgsize;i++){
          buffer[i] = readDWfromSock(sockfd);
        }

        if( get_offset(addr, &buffer_id, &offset) )
          printf("Could not find physical address %016lx\n", addr);
        else {
          buf = new rorcfs_buffer();
          if( buf->connect(parent_dev, buffer_id) )
            perror("failed to connect to buffer");
          else {
            mem = buf->getMem();
            memcpy(mem+(offset>>2), buffer, msgsize*sizeof(uint32_t));
            printf("CMD_WRITE_TO_HOST: %d DWs to buf %ld offset %ld\n",
                msgsize, buffer_id, offset);
          }
          delete buf;
        }

        free(buffer);
        break;

      case CMD_READ_FROM_HOST:
        // get offset, read from buffer, send to FLI
        // set private variables describing the request
        // another thread breaks the request down into
        // several CMPL_Ds and waits for CMD_ACK_CMPL

        addr = (uint64_t)readDWfromSock(sockfd)<<32;
        addr += readDWfromSock(sockfd);
        reqid = readDWfromSock(sockfd);
        param = readDWfromSock(sockfd);

        rdreq.length = (param>>16)<<2;
        rdreq.tag = ((param>>8)&0xff);
        rdreq.byte_enable = param&0xff;
        rdreq.lower_addr = addr & 0xff;
        rdreq.requester_id = reqid;

        printf("sock_monitor: CMD_READ_FROM_HOST %08x\n", param);
        if( get_offset(addr, &(rdreq.buffer_id), &(rdreq.offset)) )
          printf("Could not find physical address %016lx\n", addr);
        else {
          // push request into pipe
          result = write(pipefd[1], &rdreq, sizeof(rdreq));
          if ( result != sizeof(rdreq) )
            printf("write to pipe failed with %d\n", result);
        }
        break;

      case CMD_ACK_CMPL:
        printf("sock_monitor: CMD_ACK_CMPL\n");
        if (msgsize!=2)
          printf("Invalid message size for CMD_ACK_CMPL:%d\n", 
              msgsize);
        cmpl_to_dev_done = 1;
        break;

      case CMD_ACK_WRITE:
        if (msgsize!=2)
          printf("Invalid message size for CMD_ACK_WRITE:%d\n", 
              msgsize);
        write_to_dev_done = 1;
        break;

      case CMD_ACK_TIME:
        if (msgsize!=3)
          printf("Invalid message size for CMD_ACK_TIME:%d\n", 
              msgsize);
        read_from_dev_data = readDWfromSock(sockfd);
        read_from_dev_done = 1;
        break;


      default:
        break;
    } //switch(cmd)
  } //while

  return NULL;
}


/**
 * handle all completions to the device
 * */
void *rorcfs_bar::cmpl_handler()
{
  int result;
  t_read_req rdreq;
  rorcfs_buffer *buf = NULL;
  uint32_t *mem;
  uint32_t *buffer;
  uint32_t length, byte_count;
  int buffersize; 

  while( (result=read(pipefd[0], &rdreq, sizeof(rdreq))) )
  {
    if(result<0) 
    {
      printf("failed to read from pipe: %d\n", result);
      break;
    }

    // break request down into CMPL_Ds with size <= MAX_PAYLOAD
    // wait for cmpl_done via CMD_CMPL_DONE from sock_monitor
    // after each packet

    buf = new rorcfs_buffer();
    if ( buf->connect(parent_dev, rdreq.buffer_id) )
      printf("failed to connect to buffer %ld\n", rdreq.buffer_id);
    mem = buf->getMem();

    while ( rdreq.length ) {

      if ( rdreq.length>MAX_PAYLOAD ) {
        byte_count = rdreq.length - MAX_PAYLOAD;
        length = MAX_PAYLOAD;
      } else {
        byte_count = 0;
        length = rdreq.length;
      }
      buffersize = 4*sizeof(uint32_t) + length;

      buffer = (uint32_t *)malloc(buffersize);
      if(buffer==NULL)
        perror("failed to alloc CMD_CMPL_TO_DEVICE buffer");


      // prepare CMD_CMPL_TO_DEVICE
      buffer[0] = ((buffersize>>2)<<16) + CMD_CMPL_TO_DEVICE;
      buffer[1] = msgid;
      buffer[2] = rdreq.requester_id;
      buffer[3] = (0<<29) + //cmpl_status
        (byte_count<<16) + //remaining byte count
        (rdreq.tag<<8) +
        (rdreq.lower_addr);
      memcpy(&(buffer[4]), mem+(rdreq.offset>>2), length);

      pthread_mutex_lock(&mtx);

      //send to FLI
      result = write(sockfd, buffer, buffersize);
      free(buffer);

      if ( result!=buffersize )
        printf("CMD_CMPL_TO_DEVICE write failed with %d\n", result);
      else {

        // wait for FLI acknowledgement
        while( !cmpl_to_dev_done )
          usleep(USLEEP_TIME);
        cmpl_to_dev_done = 0;
        printf("cmpl_handler: CMD_CMPL_TO_DEVICE: tag=%x, length=%x, "
            "buffer=%ld offset=%lx MP=%x Req:%x\n", rdreq.tag, length, 
            rdreq.buffer_id, (rdreq.offset>>2), MAX_PAYLOAD, rdreq.length);
      }

      pthread_mutex_unlock(&mtx);

      rdreq.length -= length;
      rdreq.offset += length;
      rdreq.lower_addr += length;
      rdreq.lower_addr &= 0x7f; // only lower 6 bit
    } //while(rdreq.length)

    delete buf;


  } // while(read(pipefd)
  printf("Pipe has been closed, cmpl_handler stopping.\n");

  return 0;
}

#endif

/**
 * get file and offset for physical address
 * */
int get_offset( uint64_t phys_addr, uint64_t *buffer_id, uint64_t *offset )
{
  char *basedir;
  int i, j, bufn, nentries, fd, nbytes;
  struct dirent **namelist;
  struct rorcfs_dma_desc desc;
  struct stat filestat;
  int fname_size;
  char *fname;
  int err;

  // attach to device
  rorcfs_device *dev = new rorcfs_device();	
  if ( dev->init(0) == -1 ) {
    printf("failed to initialize device 0\n");
  }
  dev->getDName(&basedir);

  // get list of buffers
  bufn = scandir(basedir, &namelist, scandir_filter, alphasort);

  // iterate over all buffers
  for (i=0;i<bufn;i++) {

    // get filename of sglist file
    fname_size = snprintf(NULL, 0, "%s%s/sglist", 
        basedir, namelist[i]->d_name) + 1;
    fname = (char *)malloc(fname_size);
    snprintf(fname, fname_size, "%s%s/sglist", 
        basedir, namelist[i]->d_name);

    // get number of entries
    err = stat(fname, &filestat);
    if (err)
      printf("DMA_MON ERROR: failed to stat file %s\n", fname);
    nentries = filestat.st_size / sizeof( struct rorcfs_dma_desc);

    // open sglist file
    fd = open(fname, O_RDONLY);
    free(fname);
    if (fd==-1)
      perror("DMA_MON ERROR: open sglist");

    *offset = 0;

    // iterate over all sglist entries
    for(j=0; j<nentries; j++) {
      nbytes = read(fd, &desc, sizeof(struct rorcfs_dma_desc));
      if(nbytes!=sizeof(struct rorcfs_dma_desc))
        printf("DMA_MON ERROR: nbytes(=%d) != sizeof(struct "
            "rorcfs_dma_desc)\n",	nbytes);

      // check if this is the destination segment
      if ( desc.addr <= phys_addr && desc.addr + desc.len > phys_addr )
      {
        //adjust offset
        *offset += (phys_addr - desc.addr);
        *buffer_id = strtoul(namelist[i]->d_name, NULL, 0);
        close(fd);
        return 0;

      } // dest-segment found
      else
        *offset += desc.len;
    } //iterate over segments

    // close sglist file
    close(fd);
  } // iterate over buffers

  printf("ERROR: destination not found: addr=%lx\n", phys_addr);
  return -1;
}



/***********************************************
 * Constructor
 **********************************************/
rorcfs_bar::rorcfs_bar(rorcfs_device *dev, int n)
{
  char basedir[] = "/sys/module/rorcfs/drivers/pci:rorcfs/";
  char subdir[] = "/mmap/";
  int digits = snprintf(NULL, 0, "%d", n);

  bar = NULL;

  fname = (char *) malloc(strlen(basedir) + 12	+ 
      strlen(subdir) + 3 + digits);

  // get sysfs file name for selected bar
  sprintf(fname, "%s0000:%02x:%02x.%x%sbar%d", 
      basedir, dev->getBus(),
      dev->getSlot(), dev->getFunc(), subdir, n);

  number = n;
  parent_dev = dev;

  // initialize mutex
  pthread_mutex_init(&mtx, NULL);

#ifdef SIM
  struct sockaddr_in serv_addr;
  struct hostent *server;
  //struct in_addr ipaddr;
  //int statusFlags;
  //
  read_from_dev_done = 0;
  write_to_dev_done = 0;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if ( sockfd < 0 )
    perror("ERROR opening socket");

  server = gethostbyname(MODELSIM_SERVER);
  //inet_pton(AF_INET, "10.0.52.10", &ipaddr);
  //server = gethostbyaddr(&ipaddr, sizeof(ipaddr), AF_INET);
  if ( server == NULL ) 
    perror("ERROR, no sich host");

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
      (char *)&serv_addr.sin_addr.s_addr,
      server->h_length);
  serv_addr.sin_port = htons(2000);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0) 
    perror("ERROR connecting");


  // create pipe for redirecting read requests
  // to completion handler
  // pipefd[0]: read end
  // pipefd[1]: write end
  if ( pipe(pipefd) == -1 )
    perror("Failed to create PIPE");

  // completion handler
  pthread_create(&cmpl_handler_p, NULL,
      cmpl_handler_helper, this);

  // watch incoming packets
  pthread_create(&sock_mon_p, NULL, 
      sock_monitor_helper, this);
  msgid = 0;

#endif
}


/***********************************************
 * Destructor
 **********************************************/
rorcfs_bar::~rorcfs_bar()
{
#ifdef SIM
  close(pipefd[0]);
  close(pipefd[1]);
  pthread_cancel(sock_mon_p);
  pthread_cancel(cmpl_handler_p);
  close(sockfd);
#endif
  pthread_mutex_destroy(&mtx);
  if (fname)
    free(fname);
  if (bar)
    munmap( bar, barstat.st_size);
  if (handle)
    close(handle);
}

