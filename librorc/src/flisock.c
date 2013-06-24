//==============================================================================
// FLI CRORC Interface
//
// (previously: FLI TX/RX UART Interface
// Set up socket, transmit/receive string to/from UART. )
//
// This library is free software; you can redistribute it and/or modify it 
// under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation; either version 2.1 of the License, or 
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.   See the GNU Lesser General Public 
// License for more details.   See http://www.gnu.org/copyleft/lesser.txt
//
//------------------------------------------------------------------------------
// Version   Author          Date          Changes
// 0.1       Hans Tiggeler   03 Aug 2003   Tested on Modelsim SE 5.7e
// 0.2       Heiko Engel     22 Sep 2011   Adjusted for CRORC, Modelsim SE 10.0c_1
//==============================================================================

#ifdef SIM

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "mti.h"                    // MTI Headers & Prototypes

//========================== Sockets Definitions ======================
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>              // defines gethostbyname()

#include "flisock.h"

/**
 * Message encoding for TCP communication with host app:
 *
 * Messages from host:
 * * CMD_READ_FROM_DEVICE
 * 	[short msgsize=4, short cmd=CMD_READ_FROM_DEVICE]
 * 	[msgID]
 * 	[addr]
 * 	[params] where params={8bit BAR, 8bit byte_enable, 16bit length}
 *
 * * CMD_WRITE_TO_DEVICE
 * 	[short msgsize>4, short cmd=CMD_WRITE_TO_DEVICE]
 * 	[msgID]
 * 	[addr]
 * 	[params] where params={8bit BAR, 8bit byte_enable, 16bit length}
 * 	[payload] where payload consists of (msgsize-4) DWs
 *
 * * CMD_GET_TIME
 * 	[short msgsize=2, short cmd=CMD_GET_TIME]
 * 	[msgID]
 *
 * * CMD_CMPL_TO_DEVICE
 * 	[short msgsize>5, short cmd=CMD_CMPL_TO_DEVICE]
 * 	[msgID]
 * 	[requesterID]
 * 	[params] where params={cmpl_status,BCM,bytecount,tag,lower_addr}
 * 	[payload] where payload consists of (msgsize-3) DWs
 *
 *
 * Messages to host:
 * * CMD_READ_FROM_HOST
 * 	[short msgsize=6, short cmd=CMD_READ_FROM_HOST]
 * 	[msgID]
 * 	[addr_high]
 * 	[addr_low]
 * 	[requesterID]
 * 	[params] where params={16bit length, 8bit tag, 8bit byte_enable}
 * 			
 * * CMD_WRITE_TO_HOST
 * 	[short msgsize>4, short cmd=CMD_WRITE_TO_HOST]
 * 	[msgID]
 * 	[addr_high]
 * 	[addr_low]
 * 	[payload] where payload consists of (msgsize-4) DWs
 *
 * * CMD_CMPL_TO_HOST
 * 	[short msgsize=4, short cmd=CMD_CMPL_TO_HOST]
 * 	[msgID]
 * 	[params] where params={cmpl_status,BCM,bytecount,tag,lower_addr}
 * 	[payload] where payload consists of 1 DW
 *
 * * CMD_ACK_TIME
 * 	[short msgsize=3, short cmd=CMD_ACK_TIME]
 * 	[msgID]
 * 	[simulation time]
 *
 * * CMD_ACK_CMPL
 * 	[short msgsize=2, short cmd=CMD_ACK_TIME]
 * 	[msgID]
 * */

uint32_t msgid;

// DMA Monitor globals
int rx_frame64, rx_frame32=0, rx_dvld=0;
int rx_ndw = 0;
uint64_t rx_data = 0;
uint64_t rx_addr = 0;
uint8_t *buffer_ptr;
uint32_t *buffer;
int buffer_size;

// Prototypes
static void trn_monitor( void *param );
static void sockreader( void *param );
int init_sockets(char *hostname, int port, int *sd_srv, int *sd_srv_acc);
void close_socket(int *sd);

void restartCallback(void *param);  // User types quit/restart 


/**
 * FLI code entry point
 * this function is called from FOREIGN attribute in fli.vhd
 **/
void fli_init( 
    mtiRegionIdT region, 
    char *param, 
    mtiInterfaceListT *generics, 
    mtiInterfaceListT *ports)
{
  inst_rec    *ip;       // Declare ports            
  mtiProcessIdT trnproc, sockreaderproc; // process id
  char        hostname[] = "localhost";
  int         port=2000;
  int i;

  // allocate memory for ports
  ip = (inst_rec *)mti_Malloc(sizeof(inst_rec));  
  fli_debug("malloc ip\n");
  if (ip==NULL) {
    mti_PrintMessage("Failed to alloc struct\n");
    close_socket(&ip->sock);
    close_socket(&ip->sock_acc);
    mti_Break();
  }

  // bind Signals/Drivers to VHDL signals
  ip->clk = mti_FindSignal( "system_tb/FLI_SIM/CIO/clk");
  ip->wr_en = mti_CreateDriver( 
      mti_FindSignal( "system_tb/FLI_SIM/CIO/wr_en") );
  ip->wr_done = mti_FindSignal( "system_tb/FLI_SIM/CIO/wr_done");
  ip->addr = mti_GetDriverSubelements(
      mti_CreateDriver(	
        mti_FindSignal( "system_tb/FLI_SIM/CIO/addr") ), 0);
  ip->bar = mti_GetDriverSubelements(
      mti_CreateDriver(	mti_FindSignal( "system_tb/FLI_SIM/CIO/bar") ), 0);	
  ip->byte_enable = mti_GetDriverSubelements(
      mti_CreateDriver(	
        mti_FindSignal( "system_tb/FLI_SIM/CIO/byte_enable") ), 0);
  ip->tag = mti_GetDriverSubelements(
      mti_CreateDriver(	
        mti_FindSignal( "system_tb/FLI_SIM/CIO/tag") ), 0);
  ip->wr_len = mti_GetDriverSubelements(
      mti_CreateDriver(	
        mti_FindSignal( "system_tb/FLI_SIM/CIO/wr_len") ), 0);

  // drive pWrData array (0 to 31) of std_logic_vector(31 downto 0)
  ip->pWrDataId = mti_FindSignal("system_tb/FLI_SIM/CIO/pWrData");
  int ticklen = mti_TickLength(	mti_GetSignalType(ip->pWrDataId));
  mtiSignalIdT *elem_list = mti_GetSignalSubelements(ip->pWrDataId, 0);
  if(ticklen < (MAX_PAYLOAD_BYTES>>2)) {
    printf("FLI_WARNING: flisock MAX_PAYLOAD_BYTES is larger than VHDL"
        "array size\n");
  }
  for(i=0; i<ticklen; i++) {
    ip->pWrData[i] = mti_GetDriverSubelements(
        mti_CreateDriver(
          elem_list[i]), 0);
  }

  ip->rd_done = mti_FindSignal( "system_tb/FLI_SIM/CIO/rd_done");
  ip->rd_en   = mti_CreateDriver( 
      mti_FindSignal( "system_tb/FLI_SIM/CIO/rd_en") );

  ip->id      = mti_GetDriverSubelements(
      mti_CreateDriver( 
        mti_FindSignal( "system_tb/FLI_SIM/CIO/id") ), 0);

  // DMA monitoring
  ip->td         = mti_FindSignal( "system_tb/FLI_SIM/CIO/trn_td" );
  ip->tsrc_rdy_n = mti_FindSignal( "system_tb/FLI_SIM/CIO/trn_tsrc_rdy_n" );
  ip->tdst_rdy_n = mti_FindSignal( "system_tb/FLI_SIM/CIO/trn_tdst_rdy_n" );
  ip->trem_n     = mti_FindSignal( "system_tb/FLI_SIM/CIO/trn_trem_n" );
  ip->tsof_n     = mti_FindSignal( "system_tb/FLI_SIM/CIO/trn_tsof_n" );
  ip->teof_n     = mti_FindSignal( "system_tb/FLI_SIM/CIO/trn_teof_n" );

  // completion handling
  ip->cmpl_en = mti_CreateDriver( 
      mti_FindSignal( "system_tb/FLI_SIM/CIO/cmpl_en") );
  ip->cmpl_status = mti_GetDriverSubelements(
      mti_CreateDriver(
        mti_FindSignal("system_tb/FLI_SIM/CIO/cmpl_status") ), 0);
  ip->bytecount = mti_GetDriverSubelements(
      mti_CreateDriver(
        mti_FindSignal("system_tb/FLI_SIM/CIO/bytecount") ), 0);
  ip->lower_addr = mti_GetDriverSubelements(
      mti_CreateDriver(
        mti_FindSignal("system_tb/FLI_SIM/CIO/lower_addr") ), 0);
  ip->requester_id = mti_GetDriverSubelements(
      mti_CreateDriver(
        mti_FindSignal("system_tb/FLI_SIM/CIO/requester_id") ), 0);
  ip->cmpl_done = mti_FindSignal("system_tb/FLI_SIM/CIO/cmpl_done");

  // trn-monitor: watch trn_td, trn_tsof, trn_tsrc_rdy, ...
  trnproc = mti_CreateProcess("trn_monitor", trn_monitor, ip);
  mti_Sensitize(trnproc, ip->clk, MTI_EVENT);

  mti_PrintFormatted("Opening socket %d on %s\nWaiting for connection...\n",
      port,hostname);

  // Get socket descriptor
  if ((init_sockets(hostname, port, &ip->sock_acc, &ip->sock))==-1) {   
    mti_PrintMessage("*** Socket init error ***\n");
    mti_FatalError(); 
  }     

  // sockreader
  sockreaderproc = mti_CreateProcess("sockreader", sockreader, ip);
  mti_Sensitize(sockreaderproc, ip->clk, MTI_EVENT);



  mti_AddQuitCB(restartCallback, ip);
  mti_AddRestartCB(restartCallback, ip); // Add quit or restart CallBack
}


void restartCallback(void *param)  // User types quit/restart 
{

  inst_rec *ip = (inst_rec *)param;
  rx_frame64 = 0;
  rx_frame32 = 0;
  rx_dvld = 0;
  buffer = NULL;
  close_socket(&ip->sock);
  close_socket(&ip->sock_acc);
  mti_Free(param);
  fli_debug("free param\n");
}


/**
 * get one DW from socket
 * TODO: add timeout
 * */
int readDWfromSock(int sock, uint32_t *data)
{
  int result = 0;
  struct timeval start_time, cur_time;
  gettimeofday(&start_time, 0);
  cur_time = start_time;

  while(result!=4) {
    result = read(sock, data, sizeof(uint32_t)); // read 1 DW
    if (result == 0)  {   // terminate if 0 characters received
      mti_PrintMessage("closing socket\n");
      close(sock);
      mti_Break();
    } else if (result == -1) {
      gettimeofday(&cur_time, 0);
      if(gettimeofday_diff(start_time, cur_time)>SOCKET_TIMEOUT) {
        return -1;
      }
    }
  }
  return 0;
}



/**
 * sockreader: check if data is available on the socket
 * triggered by CallBack
 * */
static void sockreader( void *arg )
{
  inst_rec *ip = (inst_rec *)arg;
  int i;

  uint16_t msgsize;
  uint16_t cmd;
  uint32_t tmpvar;
  uint64_t addr;
  uint32_t param, requester_id;
  uint32_t *outbuf = NULL;
  int result;

  if (mti_GetSignalValue(ip->clk)==STD_LOGIC_1) { //rising_edge(clk)
    // check if a new command is available
    result = read(ip->sock, &tmpvar, sizeof(uint32_t)); // read 1 DW
    if (result == 0)  {   // terminate if 0 characters received
      mti_PrintMessage("closing socket\n");
      close(ip->sock );
      close(ip->sock_acc);
      mti_Break();
    } else if (result!=-1)	{

      cmd = tmpvar & 0xffff;
      msgsize = (tmpvar >> 16) & 0xffff;

      // get msgid and set it in Modelsim
      if(readDWfromSock(ip->sock, &msgid))
        mti_PrintMessage("failed to read msgid\n");
      set_slv( ip->id, msgid, 32 );

      switch (cmd) {
        case CMD_READ_FROM_DEVICE:
          // remaining: 32bit addr, 
          // 32bit param={8bit BAR, 8bit BE, 16bit length}

          if(msgsize!=4)
            mti_PrintMessage("FLI Warning: Illegal message size "
                "for CMD_READ_FROM_DEVICE\n");

          if (readDWfromSock(ip->sock, &tmpvar))
            mti_PrintFormatted("CMD_READ_FROM_DEVICE: failed to read addr\n");
          addr = tmpvar; //cast to 64bit
          if (readDWfromSock(ip->sock, &param))
            mti_PrintFormatted("CMD_READ_FROM_DEVICE: "
                "failed to read param\n");

          set_slv( ip->addr, addr + ADDR_OFFSET, 32 );
          set_slv( ip->byte_enable, (param>>16)&0xff, 8 );
          set_slv( ip->tag, msgid & 0xff, 8 );
          set_bar( ip->bar, (param>>24)&0xff );

          // read request
          mti_ScheduleDriver(ip->rd_en, STD_LOGIC_1, 0, MTI_INERTIAL);

          fli_debug("%d CMD_READ_FROM_DEVICE(%x)\n", msgid, addr);
          break;

        case CMD_WRITE_TO_DEVICE:
          // remaining: 32bit addr, 32bit param{BAR, BE, len}, n x 32bit DW

          if(msgsize<=4)
            mti_PrintMessage("FLI Warning: Illegal message size "
                "for CMD_WRITE_TO_DEVICE\n");

          if (readDWfromSock(ip->sock, &tmpvar))
            mti_PrintFormatted("CMD_WRITE_TO_DEVICE: failed to read addr\n");
          addr = tmpvar; //cast to 64bit
          if (readDWfromSock(ip->sock, &param))
            mti_PrintFormatted("CMD_WRITE_TO_DEVICE: failed to read param\n");
          //fli_debug("CMD_WRITE_TO_DEVICE: param=%08x\n", param);

          set_slv( ip->addr, addr + ADDR_OFFSET, 32 );
          set_slv( ip->byte_enable, (param>>16)&0xff, 8 );
          set_slv( ip->tag, msgid & 0xff, 8 );
          set_bar( ip->bar, (param>>24)&0xff );
          set_slv( ip->id, msgid, 32 );

          msgsize -=4;
          for(i=0;i<msgsize;i++)
          {
            if(readDWfromSock(ip->sock, &tmpvar))
              mti_PrintFormatted("failed to read DW[%d]\n", i);
            //fli_debug("CMD_WRITE_TO_DEVICE: DW[%d]=%08x\n", i, tmpvar);
            set_slv(ip->pWrData[i], tmpvar, 32);
          }

          mti_ScheduleDriver(ip->wr_en, STD_LOGIC_1, 0, MTI_INERTIAL);
          set_slv( ip->wr_len, (param & 0xffff), 10);
          fli_debug("%d CMD_WRITE_TO_DEVICE(%x, %d DWs)\n", 
              msgid, addr, msgsize);
          break;

        case CMD_GET_TIME:
          if(msgsize!=2)
            mti_PrintMessage("FLI Warning: Illegal message size "
                "for CMD_GET_TIME\n");
          outbuf = (uint32_t *)mti_Malloc(3*sizeof(uint32_t));
          if(!outbuf) {
            mti_PrintMessage("Failed to allocate outbuf memory\n");
            close_socket(&ip->sock);
            close_socket(&ip->sock_acc);
            mti_Break();
          }
          outbuf[0] = (3<<16) + CMD_ACK_TIME;
          outbuf[1] = msgid;
          outbuf[2] = mti_Now(); //least significant 32bits
          result = send(ip->sock, outbuf, 3*sizeof(uint32_t), 0);
          if(result<0)
            mti_PrintMessage("ERROR: failed to send CMD_ACK_TIME\n");
          //fli_debug("%d CMD_GET_TIME: %d\n", msgid, outbuf[2]);
          mti_Free(outbuf);
          break;

        case CMD_CMPL_TO_DEVICE:
          if(msgsize<=4)
            mti_PrintMessage("FLI Warning: Illegal message size "
                "for CMD_CMPL_TO_DEVICE\n");
          if (readDWfromSock(ip->sock, &requester_id))
            mti_PrintFormatted("CMD_CMPL_TO_DEVICE: failed to read "
                "requester_id\n");
          if(readDWfromSock(ip->sock, &param))
            mti_PrintMessage("CMD_CMPL_TO_DEVICE: failed to read param\n"); 
          /**
           * 	cmpl_status = param[31:29]
           * 	BCM = param[28] -- not implemented
           * 	remaining bytecount = param[27:16]
           * 	tag = param[15:8]
           * 	lower_addr = param[6:0]
           **/
          set_slv(ip->cmpl_status, ((param>>29)&0x7), 3);
          set_slv(ip->bytecount, ((param>>16)&0xfff), 12);
          set_slv(ip->tag, ((param>>8)&0xff), 8);
          set_slv(ip->lower_addr, (param&0x7f), 7);
          set_slv(ip->wr_len, msgsize-4, 10);
          set_slv(ip->requester_id, requester_id, 16);

          // TODO: add BAR

          msgsize -= 4;
          set_slv(ip->wr_len, msgsize, 10);
          for(i=0;i<msgsize;i++)
          {
            if(readDWfromSock(ip->sock, &tmpvar))
              mti_PrintFormatted("CMD_CMPL_TO_DEVICE: failed to read "
                  " DW[%d]\n", i);
            set_slv(ip->pWrData[i], tmpvar, 32);
          }

          mti_PrintFormatted("CMD_CMPL_TO_DEVICE, %d DWs\n", msgsize);
          // send completion request
          mti_ScheduleDriver(ip->cmpl_en, STD_LOGIC_1, 0, MTI_INERTIAL);


          break;

        default:
          break;

      } //switch(cmd)
    } //read
  } //rising edge
} //sockreader


void dump_sockcmd(uint32_t *buf, uint32_t len)
{
  int i;
  for(i=0; i<(len>>2); i++)
    fli_debug("%d: %08x\n", i, *(buf+i));
}


/**
 * Monitor all activities on trn_td
 * - capture DMA writes
 * - capture read completion
 * - capture write acknowledgements
 *
 * TODO: 
 * * wrap DMA data into new format
 *
 * */
static void trn_monitor( void *arg )
{
  inst_rec *ip = (inst_rec *)arg;
  uint64_t rx_trem;
  uint32_t tmp32;
  int result;
  uint32_t *lbuf;
  uint32_t lbufsize;

  if (mti_GetSignalValue(ip->clk)==STD_LOGIC_1) { //rising_edge(clk)
    if (mti_GetSignalValue(ip->tsrc_rdy_n)==STD_LOGIC_0 &&
        mti_GetSignalValue(ip->tdst_rdy_n)==STD_LOGIC_0) {

      // valid data
      if (rx_dvld) {

        rx_data = byte_reorder(slv2ulong_high(ip->td));
        rx_trem = slv2ulong(ip->trem_n);

        // data on [127:?], but not [127:0]
        if (rx_trem != 0  && mti_GetSignalValue(ip->teof_n)==STD_LOGIC_0) { 
          switch (rx_trem) {
            case 1:
              rx_ndw -= 3;
              tmp32 = (uint32_t)(rx_data & 0xffffffff);
              memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
              buffer_ptr += sizeof(uint32_t);
              //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
              //tmp32, buffer_ptr);	

              tmp32 = (uint32_t)(rx_data >> 32);
              memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
              buffer_ptr += sizeof(uint32_t);
              //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
              //tmp32, buffer_ptr);

              rx_data = byte_reorder(slv2ulong_low(ip->td));

              tmp32 = (uint32_t)(rx_data & 0xffffffff);
              memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
              buffer_ptr += sizeof(uint32_t);
              //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
              //tmp32, buffer_ptr);
              break;

            case 2:
              rx_ndw -= 2;
              tmp32 = (uint32_t)(rx_data & 0xffffffff);
              memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
              buffer_ptr += sizeof(uint32_t);
              //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
              //tmp32, buffer_ptr);	

              tmp32 = (uint32_t)(rx_data >> 32);
              memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
              buffer_ptr += sizeof(uint32_t);
              //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
              //tmp32, buffer_ptr);
              break;

            case 3:
              rx_ndw -= 1;
              tmp32 = (uint32_t)(rx_data & 0xffffffff);
              memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
              buffer_ptr += sizeof(uint32_t);
              //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
              //tmp32, buffer_ptr);
              break;

            default:
              break;
          } //switch(rx_trem)

        } else { //data on td[127:0]
          rx_ndw -= 4;

          tmp32 = (uint32_t)(rx_data & 0xffffffff);
          memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
          buffer_ptr += sizeof(uint32_t);
          //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
          //tmp32, buffer_ptr);

          tmp32 = (uint32_t)(rx_data >> 32);
          memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
          buffer_ptr += sizeof(uint32_t);
          //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
          //tmp32, buffer_ptr);

          rx_data = byte_reorder(slv2ulong_low(ip->td));

          tmp32 = (uint32_t)(rx_data & 0xffffffff);
          memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
          buffer_ptr += sizeof(uint32_t);
          //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
          //tmp32, buffer_ptr);

          tmp32 = (uint32_t)(rx_data >> 32);
          memcpy( buffer_ptr, &tmp32, sizeof(uint32_t) );
          buffer_ptr += sizeof(uint32_t);
          //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
          //tmp32, buffer_ptr);
        }

        // check for protocol error
        if ( (rx_ndw==0 && mti_GetSignalValue(ip->teof_n)!=STD_LOGIC_0 ) ||
            (rx_ndw!=0 && mti_GetSignalValue(ip->teof_n)==STD_LOGIC_0 ) ||
            rx_ndw < 0 ) {
          mti_PrintFormatted("%d Protocol Error: nDW in HDR does "
              "not comply with #DWs sent (%d)\n", mti_Now(), rx_ndw);
          rx_dvld = 0;
          mti_Free(buffer);
          fli_debug("free buffer 574\n");
          buffer = NULL;
          buffer_ptr = NULL;

        } else if (mti_GetSignalValue(ip->teof_n)==STD_LOGIC_0) {
          // send data
          dump_sockcmd(buffer, buffer_size);
          result = send(ip->sock, buffer, buffer_size, 0);
          if(result<0)
            mti_PrintMessage("ERROR: failed to send CMD_WRITE_TO_HOST\n");
          rx_dvld = 0;
          rx_ndw = 0;
          mti_Free(buffer);
          fli_debug("free buffer 586\n");
          buffer = NULL;
          buffer_ptr = NULL;
          buffer_size = 0;
        }
      }

      // wait for SOF
      if (mti_GetSignalValue(ip->tsof_n)==STD_LOGIC_0 && rx_dvld==0)
      {
        // number of DWs: td[41:32]
        rx_ndw = ( slv2ulong_high(ip->td)>>32 ) & 0x3ff;

        switch (slv2ulong_high(ip->td) & FMT_MASK) {
          case FMT_MWR64:

            // [msgsize,cmd][msgID][addr_high][addr_low][rx_ndw*DW]
            buffer_size = (rx_ndw+4)*sizeof(uint32_t);
            buffer = (uint32_t *)mti_Malloc( buffer_size );
            fli_debug("malloc buffer 605\n");
            buffer_ptr = (uint8_t *)buffer;
            if (!buffer) {
              perror("dma_monitor: malloc buffer");
              close_socket(&ip->sock);
              close_socket(&ip->sock_acc);
              mti_Break();
            }

            rx_addr = slv2ulong_low(ip->td);
            buffer[0] = ((rx_ndw+4)<<16) + CMD_WRITE_TO_HOST;
            buffer[1] = msgid;
            buffer[2] = (rx_addr>>32);
            buffer[3] = (rx_addr & 0xffffffff);

            buffer_ptr += 4*sizeof(uint32_t);
            fli_debug("** FLI DMA MON: MWR64 TSOF, %d DWs to %lx\n", 
                rx_ndw, rx_addr);
            rx_dvld = 1;
            break;

          case FMT_MWR32:

            // [msgsize,cmd][msgID][addr_high][addr_low][rx_ndw*DW]
            buffer_size = (rx_ndw+4)*sizeof(uint32_t);
            buffer = (uint32_t *)mti_Malloc( buffer_size );
            fli_debug("malloc buffer\n");
            buffer_ptr = (uint8_t *)buffer;
            if (!buffer) {
              perror("dma_monitor: malloc buffer 634");
              close_socket(&ip->sock);
              close_socket(&ip->sock_acc);
              mti_Break();
            }

            rx_addr = slv2ulong_low(ip->td);
            buffer[0] = ((rx_ndw+4)<<16) + CMD_WRITE_TO_HOST;
            buffer[1] = msgid;
            buffer[2] = 0x00000000; // addr_high

            // get lower 64bit of trn_td
            rx_data = slv2ulong_low(ip->td);
            buffer[3] = ((uint64_t)rx_data>>32); // addr_low
            // first DW
            buffer[4] = (uint32_t)(byte_reorder(rx_data) >> 32);

            buffer_ptr += 5*sizeof(uint32_t);
            //mti_PrintFormatted("***memcpy32 %08x to ptr %p\n", 
            //tmp32, buffer_ptr);
            rx_ndw -= 1;
            rx_dvld = 1;
            break;

          case FMT_MRD64:
            // [msgsize,cmd][msgID][addr_high][addr_low][ReqID][params]
            buffer_size = 6*sizeof(uint32_t);
            buffer = (uint32_t *)mti_Malloc( buffer_size );
            fli_debug("malloc buffer 662\n");
            buffer_ptr = (uint8_t *)buffer;
            if (!buffer) {
              perror("dma_monitor: malloc buffer");
              close_socket(&ip->sock);
              close_socket(&ip->sock_acc);
              mti_Break();
            }

            rx_addr = slv2ulong_low(ip->td);
            buffer[0] = (6<<16) + CMD_READ_FROM_HOST;
            buffer[1] = msgid;
            buffer[2] = (rx_addr>>32);
            buffer[3] = (rx_addr & 0xffffffff);
            // requesterID
            buffer[4] = ( (slv2ulong_high(ip->td)>>16) & 0x0000ffff );
            // params: 16b length, 8b tag, 8b BE
            buffer[5] = (rx_ndw<<16) + 
              ( slv2ulong_high(ip->td) & 0x0000ffff );

            fli_debug("CMD_READ_FROM_HOST: %08x\n", buffer[5]);

            // send request to host
            dump_sockcmd(buffer, buffer_size);
            result = send(ip->sock, buffer, buffer_size, 0);
            if(result<0)
              mti_PrintMessage("ERROR: failed to send "
                  "CMD_READ_FROM_HOST64\n");
            mti_Free(buffer);
            fli_debug("free buffer 690\n");
            break;

          case FMT_MRD32:
            // [msgsize,cmd][msgID][addr_high][addr_low][ReqID][params]
            buffer_size = 6*sizeof(uint32_t);
            buffer = (uint32_t *)mti_Malloc( buffer_size );
            fli_debug("malloc buffer 697\n");
            buffer_ptr = (uint8_t *)buffer;
            if (!buffer) {
              perror("dma_monitor: malloc buffer");
              close_socket(&ip->sock);
              close_socket(&ip->sock_acc);
              mti_Break();
            }

            rx_addr = slv2ulong_low(ip->td);
            buffer[0] = (6<<16) + CMD_READ_FROM_HOST;
            buffer[1] = msgid;
            buffer[2] = 0x00000000;
            buffer[3] = ((rx_addr>>32) & 0xffffffff);
            // requesterID
            buffer[4] = ( (slv2ulong_high(ip->td)>>16) & 0x0000ffff );
            // params: 16b length, 8b tag, 8b BE
            buffer[5] = (rx_ndw<<16) + 
              ( slv2ulong_high(ip->td) & 0x0000ffff );

            fli_debug("CMD_READ_FROM_HOST: %08x, addr=%08x\n", 
                buffer[5], buffer[3]);

            // send request to host
            dump_sockcmd(buffer, buffer_size);
            result = send(ip->sock, buffer, buffer_size, 0);
            if(result<0)
              mti_PrintMessage("ERROR: failed to send "
                  "CMD_READ_FROM_HOST32\n");

            mti_Free(buffer);
            fli_debug("free buffer 724\n");
            break;

          case FMT_CMPLD:
            // read completion to host
            // [msgsize,cmd][msgID][params][payload]
            if (rx_ndw!=1)
              mti_PrintFormatted("CMD_READ_FROM_HOST: unsupported "
                  "number of DWs: %d\n",
                  rx_ndw);
            lbufsize = 4*sizeof(uint32_t);
            lbuf = (uint32_t *)mti_Malloc( lbufsize );
            fli_debug("malloc lbuf 736\n");
            buffer_ptr = (uint8_t *)lbuf;
            if (!lbuf) {
              perror("dma_monitor: malloc lbuf");
              close_socket(&ip->sock);
              close_socket(&ip->sock_acc);
              mti_Break();
            }

            lbuf[0] = (4<<16) + CMD_CMPL_TO_HOST;
            lbuf[1] = msgid;
            lbuf[2] = ((slv2ulong_high(ip->td) & 0x0000ffff)<<16) + 
              ((slv2ulong_low(ip->td)>>32) & 0x0000ffff);
            rx_data = slv2ulong_low(ip->td);
            lbuf[3] = (uint32_t)(byte_reorder(rx_data) >> 32);

            mti_ScheduleDriver( ip->rd_en, STD_LOGIC_0, 0, MTI_INERTIAL );
            dump_sockcmd(lbuf, lbufsize);
            result = send(ip->sock, lbuf, lbufsize, 0);
            if(result<0)
              mti_PrintFormatted("failed to send CMD_CMPL_TO_HOST\n");
            fli_debug("%d FLI CMD %d served: %lx\n", 
                mti_Now(), msgid, lbuf[3]);
            mti_Free(lbuf);
            fli_debug("free lbuf 759\n");
            break;


          default:
            break;
        } //switch(FMT)

      } // SOF

    } // tsrc_rdy_n=='0'

    if (mti_GetSignalValue(ip->wr_done)==STD_LOGIC_1) { // write done?

      // [msgsize,cmd][msgID]
      lbufsize = 2*sizeof(uint32_t);
      lbuf = (uint32_t *)mti_Malloc( lbufsize );
      fli_debug("malloc lbuf 776\n");
      if (!lbuf) {
        perror("trn_monitor: malloc CMD_ACK_WRITE lbuf");
        close_socket(&ip->sock);
        close_socket(&ip->sock_acc);
        mti_Break();
      } else {

        lbuf[0] = (2<<16) + CMD_ACK_WRITE;
        lbuf[1] = msgid;

        //de-assert wr_en
        mti_ScheduleDriver( ip->wr_en, STD_LOGIC_0, 0, MTI_INERTIAL );

        result = send(ip->sock, lbuf, lbufsize, 0);
        if(result<0)
          mti_PrintFormatted("ERROR: failed to send CMD_ACK_WRITE\n");
        mti_Free(lbuf);
        fli_debug("free lbuf 794\n");
        fli_debug("%d FLI CMD %d served\n", mti_Now(), msgid);
      }
    } //wr_done


    if (mti_GetSignalValue(ip->cmpl_done)==STD_LOGIC_1) { // cmpl_done?

      // [msgsize,cmd][msgID]
      lbufsize = 2*sizeof(uint32_t);
      lbuf = (uint32_t *)mti_Malloc( lbufsize );
      fli_debug("malloc lbuf 805\n");
      if (!lbuf) {
        perror("trn_monitor: malloc CMD_ACK_CMPL lbuf");
        close_socket(&ip->sock);
        close_socket(&ip->sock_acc);
        mti_Break();
      } else {

        lbuf[0] = (2<<16) + CMD_ACK_CMPL;
        lbuf[1] = msgid;

        //de-assert cmpl_en
        mti_ScheduleDriver( ip->cmpl_en, STD_LOGIC_0, 0, MTI_INERTIAL );

        result = send(ip->sock, lbuf, lbufsize, 0);
        if(result<0)
          mti_PrintFormatted("ERROR: failed to send CMD_ACK_CMPL\n");
        mti_Free(lbuf);
        fli_debug("free lbuf 823\n");
        fli_debug("%d FLI CMD_ACK_CMPL %d served\n", mti_Now(), msgid);
      }
    } //cmpl_done





  } // rising_edge

} //trn_monitor



/**
 * initialize sockets, start server
 **/
int init_sockets(char *hostname, int port, int *sd_srv, int *sd_srv_acc)
{
  int     statusFlags;
  uint32_t addrlen;
  struct  sockaddr_in sin;
  struct  sockaddr_in pin;
  struct  hostent *hp;

  // Request Stream socket from OS using default protocol
  if ((*sd_srv = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
    mti_PrintMessage("*** Error opening socket ***\n" );
    return -1;
  }

  // get host machine info
  if ((hp = gethostbyname(hostname)) == 0) {
    mti_PrintFormatted( "%s: Unknown host.\n", hostname );
    close_socket(sd_srv);
    return -1;
  }

  // fill in the socket structure with host information 
  memset(&sin, 0, sizeof(sin));               // Clear Structure
  sin.sin_family = AF_INET;                   // Specify Address format
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons((uint16_t)port); // set port number

  // bind the socket to the port number 
  if (bind(*sd_srv, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
    mti_PrintMessage("*** Error: Bind\n");
    close_socket(sd_srv);
    return -1;
  }

  // Listen
  if (listen(*sd_srv, 5) == -1) {
    mti_PrintMessage("*** Error: Listen\n");
    close_socket(sd_srv);
    return -1;
  }

  // Accept connection from client, create new socket for communication
  addrlen = sizeof(struct sockaddr_in);
  if ((*sd_srv_acc = accept(*sd_srv, (struct sockaddr *)&pin, 
          &addrlen)) == -1) {
    mti_PrintMessage("*** Error: Accept\n");
    close_socket(sd_srv);
    return -1;
  }

  /*	statusFlags = fcntl(ip->sdsc, F_GETFL );
        if ( statusFlags == -1 ) {
        perror( "Getting socket status" );
        } else {
        int ctlValue;
        statusFlags |= O_NONBLOCK;
        ctlValue = fcntl( ip->sdsc, F_SETFL, statusFlags );
        if (ctlValue == -1 ) {
        perror("Setting listen socket status");
        }
        } */

  statusFlags = fcntl( *sd_srv_acc, F_GETFL );
  if ( statusFlags == -1 ) {
    perror("Getting listen socket status");
  } else {
    int ctlValue;
    statusFlags |= O_NONBLOCK;
    ctlValue = fcntl( *sd_srv_acc, F_SETFL, statusFlags );
    if (ctlValue == -1 ) {
      perror("Setting socket status");
    }
  }

  mti_PrintFormatted("Server: connect from host\n");
  return 0;
}


/**
 * Close Socket
 **/
void close_socket(int *sd)
{
  close(*sd);
  mti_PrintMessage("**** Socket removed ****\n");
}

#endif
