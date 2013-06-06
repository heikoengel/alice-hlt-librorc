/**
 * @file flisock.h
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-01-28
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
 * This file is based on the sample implementation of a
 * FLI TX/RX UART Interface from Hans Tiggeler, 03 Aug 2003
 *
 *
 *
 *
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
 * 	[short msgsize>4, short cmd=CMD_CMPL_TO_DEVICE]
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
 * 	[ReqesterID]
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
* * CMD_ACK_WRITE
* 	[short msgsize=2, short cmd=CMD_ACK_WRITE]
* 	[msgID]
*
* * CMD_ACK_CMPL
* 	[short msgsize=2, short cmd=CMD_ACK_TIME]
* 	[msgID]
* */


#ifndef _FLISOCK_H
#define _FLISOCK_H

/** Mask to get format/type fields of SOF TRN word **/
#define FMT_MASK  0x7F00000000000000

/** format/type for 64b memory write **/
#define FMT_MWR64 0x6000000000000000

/** format/type for 32b memory write **/
#define FMT_MWR32 0x4000000000000000

/** format/type for 64b memory read **/
#define FMT_MRD64 0x2000000000000000

/** format/type for 32b memory read **/
#define FMT_MRD32 0x0000000000000000

/** format/type for completion with data **/
#define FMT_CMPLD 0x4a00000000000000

/** arbitrary address offset added to each TRN address **/
#define ADDR_OFFSET 0xfa000000

/** default max payload value **/
#define MAX_PAYLOAD_BYTES 512

/** timeout of socket read in seconds **/
#define SOCKET_TIMEOUT 5.0

/** debug output **/
#ifdef FLI_DEBUG
#define fli_debug(fmt, args...) mti_PrintFormatted(fmt, ## args)
#else
#define fli_debug(fmt, args...)
#endif

/** VHDL signal types **/
typedef enum {
  STD_LOGIC_U,
  STD_LOGIC_X,
  STD_LOGIC_0,
  STD_LOGIC_1,
  STD_LOGIC_Z,
  STD_LOGIC_W,
  STD_LOGIC_L,
  STD_LOGIC_H,
  STD_LOGIC_D
} StdLogicType;


/** signals to be monitored/controlled in pcie_io.vhd **/
typedef struct {
  mtiSignalIdT clk ;
  mtiDriverIdT wr_en;
  mtiSignalIdT wr_done;
  mtiDriverIdT *addr;
  mtiDriverIdT *wr_len;
  mtiDriverIdT *pWrData[MAX_PAYLOAD_BYTES>>2];
  mtiSignalIdT pWrDataId;
  mtiDriverIdT *bar;
  mtiDriverIdT *byte_enable;
  mtiDriverIdT *tag;
  mtiSignalIdT rd_done;
  mtiDriverIdT rd_en;

  mtiDriverIdT cmpl_en;
  mtiSignalIdT cmpl_done;
  mtiDriverIdT *cmpl_status;
  mtiDriverIdT *bytecount;
  mtiDriverIdT *lower_addr;
  mtiDriverIdT *requester_id;

  mtiSignalIdT td;
  mtiSignalIdT teof_n;
  mtiSignalIdT tsof_n;
  mtiSignalIdT trem_n;
  mtiSignalIdT tsrc_rdy_n;
  mtiSignalIdT tdst_rdy_n;

  mtiDriverIdT *id;

  int       sock;
  int       sock_acc;
} inst_rec;

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



/**
 * change byte ordering of 64bit trn_td QWord
 * {[7:0],[15:8],[23:16],[31:24],[39:32],[47:40],[55:48],[63:56]}
 **/
unsigned long byte_reorder( unsigned long qw )
{
  unsigned long ret=0;
  int i;
  for(i=0; i<8; i++)
    ret += (((unsigned long)(qw>>(8*i)) & 0x00000000000000ff) << (56-8*i));
  return ret;
}

/**
 * change byte ordering of 32bit trn_td DWord
 * {[7:0],[15:8],[23:16],[31:24]}
 **/
unsigned int byte_reorder32( unsigned int dw )
{
  unsigned int ret=0;
  int i;
  for(i=0; i<4; i++)
    ret += (((unsigned int)(dw>>(4*i)) & 0x000000ff) << (28-4*i));
  return ret;
}



/**
 * Convert std_logic_vector into an integer
 **/
mtiUInt32T conv_std_logic_vector(mtiSignalIdT stdvec)
{
  mtiSignalIdT *  elem_list;
  mtiTypeIdT      sigtype;
  mtiInt32T       i,num_elems;
  mtiUInt32T      retvalue,shift;

  sigtype = mti_GetSignalType(stdvec);  // signal type
  num_elems = mti_TickLength(sigtype);  // Get number of elements

  elem_list = mti_GetSignalSubelements(stdvec, 0);

  shift=(mtiUInt32T) pow(2.0,(double)num_elems-1);// start position

  retvalue=0;
  for (i=0; i < num_elems; i++ ) {
    if (mti_GetSignalValue(elem_list[i])==3) {
      retvalue=retvalue+shift;
    }
    shift=shift>>1;
  }

  mti_VsimFree(elem_list);

  return(retvalue);
}


/**
 * Convert std_logic_vector into an unsigned long
 **/
unsigned long slv2ulong(mtiSignalIdT stdvec)
{
  mtiSignalIdT *  elem_list;
  mtiTypeIdT      sigtype;
  unsigned long   i,num_elems;
  unsigned long   retvalue,shift;

  sigtype = mti_GetSignalType(stdvec);  // signal type
  num_elems = mti_TickLength(sigtype);  // Get number of elements

  elem_list = mti_GetSignalSubelements(stdvec, 0);

  shift=(unsigned long) pow(2.0,(double)num_elems-1);// start position

  retvalue=0;
  for (i=0; i < num_elems; i++ ) {
    if (mti_GetSignalValue(elem_list[i])==3) {
      retvalue=retvalue+shift;
    }
    shift=shift>>1;
  }

  mti_VsimFree(elem_list);

  //mti_PrintFormatted("slv2ulong: num_elems=%d, sizeof(ret)=%d, ret=%llx\n",
  //		num_elems, sizeof(retvalue), retvalue);

  return(retvalue);
}
/**
 * Convert bits 127:63 of std_logic_vector into
 * an unsigned long
 **/
unsigned long slv2ulong_high(mtiSignalIdT stdvec)
{
  mtiSignalIdT *  elem_list;
  unsigned long   i;
  unsigned long   retvalue = 0;

  elem_list = mti_GetSignalSubelements(stdvec, 0);

  for ( i=0; i<64; i++ ) {
    if (mti_GetSignalValue(elem_list[i])==3)
      retvalue += ((unsigned long)(1)<<(63-i));
  }

  mti_VsimFree(elem_list);
  return(retvalue);
}


/**
 * Convert bits 63:0 of std_logic_vector into
 * an unsigned long
 **/
unsigned long slv2ulong_low(mtiSignalIdT stdvec)
{
  mtiSignalIdT *  elem_list;
  unsigned long   i;
  unsigned long   retvalue = 0;

  elem_list = mti_GetSignalSubelements(stdvec, 0);

  for ( i=64; i<128; i++ ) {
    if (mti_GetSignalValue(elem_list[i])==3)
      retvalue += ((unsigned long)(1)<<(63-i));
  }

  mti_VsimFree(elem_list);
  return(retvalue);
}


/*
 * set_bar:
 * e.g. value=1 -> bar=1111101, value=0 -> bar=1111110, ...
 **/
void set_bar( mtiDriverIdT *drivers, unsigned int value )
{
  int i;
  for (i=0;i<7;i++) {
    if ( ( (~(1<<value)) >>i) & 0x01 )
      mti_ScheduleDriver( drivers[6-i], STD_LOGIC_1, 0, MTI_INERTIAL);
    else
      mti_ScheduleDriver( drivers[6-i], STD_LOGIC_0, 0, MTI_INERTIAL);
  }
}


/**
 * set_slv: drive std_logic_vector of any bit bitdth <=32 with binary
 * representation of the input data
 * */
void set_slv( mtiDriverIdT *drivers, unsigned int data, int bit_width)
{
  int i;
  for ( i=0; i<bit_width; i++ ) {
    if(data>>i & 0x01 )
      mti_ScheduleDriver( drivers[bit_width-1-i],
          STD_LOGIC_1, 0, MTI_INERTIAL);
    else
      mti_ScheduleDriver( drivers[bit_width-1-i],
          STD_LOGIC_0, 0, MTI_INERTIAL);
  }
}



/**
 * get time difference of two struct timeval as (double)seconds
 * @param time1 earlier time
 * @param time2 later time
 * @return time difference as (double)seconds
 * */
double gettimeofday_diff(struct timeval time1, struct timeval time2) {
  struct timeval diff;
  diff.tv_sec = time2.tv_sec - time1.tv_sec;
  diff.tv_usec = time2.tv_usec - time1.tv_usec;
  while(diff.tv_usec < 0) {
    diff.tv_usec += 1000000;
    diff.tv_sec -= 1;
  }

  return (double)((double)diff.tv_sec +
      (double)((double)diff.tv_usec / 1000000));
}




#endif
