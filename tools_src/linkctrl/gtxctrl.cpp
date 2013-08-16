/**
 * @file gtxctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-07-19
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
 * */


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <getopt.h>
#include <stdint.h>

#include "librorc.h"

using namespace std;

#define HELP_TEXT "gtxctrl usage: \n\
        gtxctrl [parameters] \n\
parameters: \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -x            Clear error counters \n\
        -r [0...7]    Set GTX reset values, see below\n\
        -l [0...7]    Set GTX loopback value \n\
        -h            Show this text \n\
\n\
GTX reset consists of 3 bits, MSB to LSB: {TXreset, RXreset, GTXreset}. \n\
In order to set GTXreset=1, RXreset=0, TXreset=0, do \n\
        gtxctrl -n [...] -c [...] -r 0x01 \n\
In order to set GTXreset=0, RXreset=1, TXreset=1, do \n\
        gtxctrl -n [...] -c [...] -r 0x06 \n\
To release all resets, do \n\
        gtxctrl -n [...] -r 0 \n\
"

struct
gtxpll_settings
{
    uint8_t clk25_div;
    uint8_t n1;
    uint8_t n2;
    uint8_t d;
    uint8_t m;
};


const struct gtxpll_settings available_configs[] =
{
    //div,n1,n2,d, m
    {  9, 5, 2, 2, 1}, // 2.125 Gbps with RefClk=212.5 Mhz
    {  9, 5, 2, 1, 1}, // 4.250 Gbps with RefClk=212.5 MHz
    { 10, 5, 2, 1, 1}, // 5.000 Gbps with RefClk=250.0 MHz
};

/** Conversions between PLL values and their register representations */
uint8_t divselout_reg2val ( uint8_t reg )
{
    if (reg==0) return 1;
    else if (reg==1) return 2;
    else return 4;
}

uint8_t divselout_val2reg ( uint8_t val )
{
    if (val==1) return 0;
    else if (val==2) return 1;
    else return 2;
}

uint8_t divselfb_reg2val ( uint8_t reg )
{
    if (reg==0) return 2;
    else if (reg==2) return 4;
    else return 5;
}

uint8_t divselfb_val2reg ( uint8_t val )
{
    if (val==2) return 0;
    else if (val==4) return 2;
    else return 3;
}

uint8_t divselfb45_reg2val ( uint8_t reg )
{
    if (reg==1) return 5;
    else return 4;
}

uint8_t divselfb45_val2reg ( uint8_t val )
{
    if (val==5) return 1;
    else return 0;
}

uint8_t clk25div_reg2val ( uint8_t reg )
{
    if (reg==5) { return 6; }
    else if (reg<=4) { return reg-1; }
    else { return reg+1; }
}

uint8_t clk25div_val2reg ( uint8_t val )
{
    if (val==6) { return 5; }
    else if (val<=5) { return val+1; }
    else { return val-1; }
}

uint8_t divselref_reg2val ( uint8_t reg )
{
    if (reg==16) return 1;
    else return 2;
}

uint8_t divselref_val2reg ( uint8_t val )
{
    if (val==1) return 16;
    else return 0;
}



/**
 * Read from GTX DRP port
 * @param ch pointer to dma_channel instance
 * @param drp_addr DRP address to read from
 * @return DRP value
 * */
uint16_t
drp_read
(
    librorc::dma_channel *ch,
    uint8_t drp_addr
)
{
  uint32_t drp_status;
  uint32_t drp_cmd = (0<<24) | //read
    (drp_addr<<16) | //DRP addr
    (0x00); // data

  ch->setPKT(RORC_REG_GTX_DRP_CTRL, drp_cmd);

  /** wait for drp_den to deassert */
  do {
    usleep(100);
    drp_status = ch->getPKT(RORC_REG_GTX_DRP_CTRL);
  } while (drp_status & (1<<31));

  printf("drp_read(%x)=%04x\n", drp_addr, (drp_status & 0xffff));

  return (drp_status & 0xffff);
}



/**
 * Write to GTX DRP port
 * @param ch pointer to dma_channel instane
 * @param drp_addr DRP address to write to
 * @param drp_data data to be written
 * */
void
drp_write
(
    librorc::dma_channel *ch,
    uint8_t drp_addr,
    uint16_t drp_data
)
{
  uint32_t drp_status;
  uint32_t drp_cmd = (1<<24) | //write
    (drp_addr<<16) | //DRP addr
    (drp_data); // data

  ch->setPKT(RORC_REG_GTX_DRP_CTRL, drp_cmd);

  /** wait for drp_den to deassert */
  do {
    usleep(100);
    drp_status = ch->getPKT(RORC_REG_GTX_DRP_CTRL);
  } while (drp_status & (1<<31));
}



/**
 * get current PLL configuration
 * @param ch pointer to dma_channel instance
 * @return struct gtxpll_settings
 * */
struct gtxpll_settings
drp_get_pll_config
(
    librorc::dma_channel *ch
)
{
    uint16_t drpdata;
    struct gtxpll_settings pll;

    drpdata = drp_read(ch, 0x1f);
    pll.n1 = divselfb45_reg2val((drpdata>>6)&0x1);
    pll.n2 = divselfb_reg2val((drpdata>>1)&0x1f);
    pll.d = divselout_reg2val((drpdata>>14)&0x3);

    drpdata = drp_read(ch, 0x20);
    pll.m = divselref_reg2val((drpdata>>1)&0x1f);

    drpdata = drp_read(ch, 0x23);
    pll.clk25_div = clk25div_reg2val((drpdata>>10)&0x1f);

    //Frequency = refclk_freq*gtx_n1*gtx_n2/gtx_m*2/gtx_d;
    return pll;
}




/**
 * set new PLL configuration
 * @param ch pointer to dma_channel instance
 * @param pll struct gtxpll_settings with new values
 * */
void
drp_set_pll_config
(
    librorc::dma_channel *ch,
    struct gtxpll_settings pll
)
{
    uint8_t n1_reg = divselfb45_val2reg(pll.n1);
    uint8_t n2_reg = divselfb_val2reg(pll.n2);
    uint8_t d_reg = divselout_val2reg(pll.d);
    uint8_t m_reg = divselref_val2reg(pll.m);
    uint8_t clkdiv = clk25div_val2reg(pll.clk25_div);

    uint16_t drp_data = drp_read(ch, 0x1f);
    drp_data &= 0x3f81; //clear all fields
    drp_data |= ((n1_reg&0x01)<<6 | (n2_reg&0x1f)<<1 | (d_reg&0x03)<<14);
    drp_write(ch, 0x1f, drp_data);

    /** dummy read */
    drp_read(ch, 0x0);

    drp_data = drp_read(ch, 0x20);
    drp_data &= 0xffc1; //clear TXPLL_DIVSEL_REF
    drp_data |= (m_reg&0x1f)<<1;
    drp_write(ch, 0x20, drp_data);

    /** dummy read */
    drp_read(ch, 0x0);

    drp_data = drp_read(ch, 0x23);
    drp_data &= 0x7c00; // clear TX_CLK25_DIVIDER
    drp_data |= (clkdiv&0x1f)<<10;
    drp_write(ch, 0x23, drp_data);

    /** dummy read */
    drp_read(ch, 0x0);
}



int main
(
    int argc,
    char *argv[]
)
{
    int do_clear = 0;
    int do_status = 0;
    int do_reset = 0;
    int do_loopback = 0;

    /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    int32_t gtxreset = 0;
    int32_t loopback = 0;

    int arg;
    while( (arg = getopt(argc, argv, "hn:c:r:l:xs")) != -1 )
    {
        switch(arg)
        {
            case 'n':
            {
                DeviceId = strtol(optarg, NULL, 0);
            }
            break;

            case 'c':
            {
                ChannelId = strtol(optarg, NULL, 0);
            }
            break;

            case 'r':
            {
                gtxreset = strtol(optarg, NULL, 0);
                do_reset = 1;
            }
            break;

            case 'l':
            {
                loopback = strtol(optarg, NULL, 0);
                do_loopback = 1;
            }
            break;

            case 's':
            {
                do_status = 1;
            }
            break;

            case 'x':
            {
                do_clear = 1;
            }
            break;

            case 'h':
            {
                cout << HELP_TEXT;
                exit(0);
            }
            break;

            default:
            {
                cout << "Unknown parameter (" << arg << ")!" << endl;
                cout << HELP_TEXT;
                return -1;
            }
        }
    }

    /** sanity checks on command line arguments **/
    if( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "DeviceId invalid or not set: " << DeviceId << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( gtxreset > 7 )
    {
        cout << "gtxreset value invalid, allowed values are 0...7" << endl;
        cout << HELP_TEXT;
        abort();
    }

    if ( loopback > 7 )
    {
        cout << "loopback value invalid, allowed values are 0...7." << endl;
        cout << HELP_TEXT;
        abort();
    }

    /** Create new device instance */
    librorc::device *dev = NULL;
    try{ dev = new librorc::device(DeviceId); }

    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }

    /** bind to BAR1 */
    librorc::bar *bar = NULL;
    try
    {
    #ifdef SIM
        bar = new librorc::sim_bar(dev, 1);
    #else
        bar = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR1.\n");
        abort();
    }

    /** get number channels implemented in firmware */
    uint32_t type_channels = bar->get32(RORC_REG_TYPE_CHANNELS);

    uint32_t startChannel, endChannel;
    if ( ChannelId==-1 )
    {
        /** no specific channel selected, iterate over all channels */
        startChannel = 0;
        endChannel = (type_channels & 0xffff) - 1;
    }
    else if ( ChannelId < (int32_t)(type_channels & 0xffff) )
    {
        /** use only selected channel */
        startChannel = ChannelId;
        endChannel = ChannelId;
    }
    else
    {
        cout << "ERROR: Selected Channel " << ChannelId
             << " is not implemented in Firmware." << endl;
        abort();
    }

    /** iterate over selected channels */
    for ( uint32_t chID=startChannel; chID<=endChannel; chID++ )
    {
        /** Create DMA channel and bind channel to BAR1 */
        librorc::dma_channel *ch = new librorc::dma_channel();
        ch->init(bar, chID);

        /** get current GTX configuration */
        uint32_t gtxasynccfg = ch->getPKT(RORC_REG_GTX_ASYNC_CFG);

        if ( do_status )
        {
            cout << "CH " << setw(2) << chID << ": 0x"
                 << hex << setw(8) << setfill('0') << gtxasynccfg
                 << dec << setfill(' ') << endl;
            struct gtxpll_settings pll = drp_get_pll_config(ch);
            cout << "PLL: N1=" << (int)pll.n1 << " N2=" << (int)pll.n2
                 << " D=" << (int)pll.d << " M=" << (int)pll.m
                 << " CLK25DIV=" << (int)pll.clk25_div << endl;

            /** TODO: also provide error counter values here */
        }

        if ( do_clear )
        {
            /** make sure GTX clock is running */
            if ( (gtxasynccfg & (1<<8)) != 0 )
            {
                cout << "WARNING: CH " << chID
                     << " : GTX clock is not running - skipping." << endl;
                continue;
            }

            /** clear disparity error count */
            ch->setGTX(RORC_REG_GTX_DISPERR_CNT, 0);

            /** clear RX-not-in-table count */
            ch->setGTX(RORC_REG_GTX_RXNIT_CNT, 0);

            /** clear RX-Loss-Of-Signal count */
            ch->setGTX(RORC_REG_GTX_RXLOS_CNT, 0);

            /** clear RX-Byte-Realign count */
            ch->setGTX(RORC_REG_GTX_RXBYTEREALIGN_CNT, 0);

            /** also clear GTX error counter for HWTest firmwares */
            if ( type_channels>>16 == RORC_CFG_PROJECT_hwtest )
            {
                ch->setGTX(RORC_REG_GTX_ERROR_CNT, 0);
            }

        }

        /** set {tx/rx/gtx}reset */
        if ( do_reset )
        {
            /** clear previous reset bits (0,1,3) */
            gtxasynccfg &= ~(0x0000000b);

            /** set new reset values */
            gtxasynccfg |= (gtxreset&1); //GTXreset
            gtxasynccfg |= (((gtxreset>>1)&1)<<1); //RXreset
            gtxasynccfg |= (((gtxreset>>2)&1)<<3); //TXreset
        }

        /** set loopback values */
        if ( do_loopback )
        {
            /** clear previous loopback bits */
            gtxasynccfg &= ~(0x00000007<<9);

            /** set new loopback value */
            gtxasynccfg |= ((loopback&7)<<9);
        }

        if ( do_reset || do_loopback )
        {
            /** write new values to RORC */
            ch->setPKT(RORC_REG_GTX_ASYNC_CFG, gtxasynccfg);
        }

        delete ch;
    }

    delete bar;
    delete dev;

    return 0;
}
