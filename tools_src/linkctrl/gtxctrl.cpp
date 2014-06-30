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

#include <getopt.h>
#include <pda.h>
#include <librorc.h>

using namespace std;

#define HELP_TEXT "gtxctrl usage: \n\
        gtxctrl [parameters] \n\
parameters: \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -x            Clear error counters \n\
        -r [0...7]    Set GTX reset values, see below\n\
        -l [0...7]    Set GTX loopback value \n\
        -d            Dump DRP address space to stdout \n\
        -s            Show link status \n\
        -p [number]   Set PLL config from list below \n\
        -P            List available PLL configs \n\
        -h            Show this text \n\
        -E [0..7]     Set RXEQMIX \n\
        -D [0..15]    Set TXDIFFCTRL \n\
        -R [0..15]    Set TXPREEMPH \n\
        -O [0..15]    Set TXPOSTEMPH \n\
\n\
GTX reset consists of 3 bits, MSB to LSB: {TXreset, RXreset, GTXreset}. \n\
In order to set GTXreset=1, RXreset=0, TXreset=0, do \n\
        gtxctrl -n [...] -c [...] -r 0x01 \n\
In order to set GTXreset=0, RXreset=1, TXreset=1, do \n\
        gtxctrl -n [...] -c [...] -r 0x06 \n\
To release all resets, do \n\
        gtxctrl -n [...] -r 0 \n\n\
"


/**
 * fPLL = fREF * N1 * N2 / M
 * fLineRate = fPLL * 2 / D
 * */
const gtxpll_settings available_configs[] =
{
    //div,n1,n2,d, m, tdcc, cp, refclk
    {  9, 5, 2, 2, 1, 0, 0x0d, 212.50}, // 2.125 Gbps with RefClk=212.50 MHz
    {  9, 5, 2, 1, 1, 3, 0x0d, 212.50}, // 4.250 Gbps with RefClk=212.50 MHz
    { 10, 5, 2, 1, 1, 3, 0x0d, 250.00}, // 5.000 Gbps with RefClk=250.00 MHz
    {  4, 5, 4, 2, 1, 0, 0x0d, 100.00}, // 2.000 Gbps with RefClk=100.00 MHz
    { 10, 4, 2, 2, 1, 0, 0x0d, 250.00}, // 2.000 Gbps with RefClk=250.00 MHz
    {  7, 5, 2, 1, 1, 3, 0x0d, 156.25}, // 3.125 Gbps with RefClk=156.25 MHz
    {  5, 5, 5, 1, 2, 3, 0x07, 125.00}, // 3.125 Gbps with RefClk=125.00 MHz
    {  9, 5, 5, 1, 2, 3, 0x07, 212.50}, // 5.3125 Gbps with RefClk=212.50 MHz
};



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
    int do_pllcfg = 0;
    int do_dump = 0;
    int show_pllcfgs = 0;
    int do_rxeqmix = 0;
    int do_txdiffctrl = 0;
    int do_txpreemph = 0;
    int do_txpostemph = 0;

    /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    int32_t gtxreset = 0;
    int32_t loopback = 0;
    int32_t pllcfgnum = 0;
    int32_t rxeqmix = 0;
    int32_t txdiffctrl = 0;
    int32_t txpreemph = 0;
    int32_t txpostemph = 0;

    int32_t nconfigs = sizeof(available_configs) /
                       sizeof(gtxpll_settings);

    int arg;
    while( (arg = getopt(argc, argv, "hn:c:r:l:xsp:dPE:D:R:O:")) != -1 )
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

            case 'd':
            {
                do_dump = 1;
            }
            break;

            case 'p':
            {
                pllcfgnum = strtol(optarg, NULL, 0);
                do_pllcfg = 1;
            }
            break;

            case 'P':
            {
                show_pllcfgs = 1;
            }
            break;

            case 'E':
            {
                do_rxeqmix = 1;
                rxeqmix = strtol(optarg, NULL, 0);
            }
            break;

            case 'D':
            {
                do_txdiffctrl= 1;
                txdiffctrl = strtol(optarg, NULL, 0);
            }
            break;

            case 'R':
            {
                do_txpreemph = 1;
                txpreemph = strtol(optarg, NULL, 0);
            }
            break;

            case 'O':
            {
                do_txpostemph = 1;
                txpostemph = strtol(optarg, NULL, 0);
            }
            break;

            case 'h':
            {
                cout << HELP_TEXT;
                show_pllcfgs = 1;
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

    if ( show_pllcfgs )
    {
        cout << "Available PLL Configurations:" << endl;
        for ( int i=0; i<nconfigs; i++ )
        {
            gtxpll_settings pll = available_configs[i];
            float fPLL = pll.refclk * pll.n1 * pll.n2 / pll.m;
            float link_rate = fPLL * 2 / pll.d / 1000.0;
            cout << "[" << i << "] RefClk="
                << fixed << setprecision(3)
                << available_configs[i].refclk
                << " MHz, LinkRate=" << link_rate
                << " Gbps, PLL=" << fPLL << " MHz" << endl;
        }
        exit(0);
    }

    /** sanity checks on command line arguments **/
    if( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "DeviceId invalid or not set: " << DeviceId << endl;
        cout << HELP_TEXT;
        abort();
    }

    if( gtxreset > 7 )
    {
        cout << "gtxreset value invalid, allowed values are 0...7" << endl;
        cout << HELP_TEXT;
        abort();
    }

    if( loopback > 7 )
    {
        cout << "loopback value invalid, allowed values are 0...7." << endl;
        cout << HELP_TEXT;
        abort();
    }

    if( pllcfgnum > nconfigs )
    {
        cout << "Invalid PLL config number!" << endl;
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
    #ifdef MODELSIM
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
    else if( ChannelId < (int)(type_channels & 0xffff) )
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
        librorc::link *current_link
            = new librorc::link(bar, chID);

        /** get current GTX configuration */
        uint32_t gtxasynccfg = current_link->pciReg(RORC_REG_GTX_ASYNC_CFG);

        if( do_status )
        {
            cout << "CH " << setw(2) << chID << ": 0x"
                 << hex << setw(8) << setfill('0') << gtxasynccfg
                 << dec << setfill(' ') << endl;

            gtxpll_settings pll = current_link->drpGetPllConfig();
            cout << "\tPLL: N1=" << (int)pll.n1 << " N2=" << (int)pll.n2
                 << " D=" << (int)pll.d << " M=" << (int)pll.m
                 << " CLK25DIV=" << (int)pll.clk25_div
                 << " TX_TDCC_CFG=" << (int)pll.tx_tdcc_cfg << endl;
            cout << "\tTxDiffCtrl=" << ((gtxasynccfg>>16)&0xf)
                 << " TxPreEmph=" << ((gtxasynccfg>>20)&0xf)
                 << " TxPostEmph=" << ((gtxasynccfg>>24)&0x1f)
                 << " RxEqMix=" << ((gtxasynccfg>>29)&0x7)
                 << endl;

            /** TODO: also provide error counter values here */
        }

        if( do_clear ) //TODO: clear all error counter
        {
            /** make sure GTX clock is running */
            if( !current_link->isGtxDomainReady() )
            {
                cout << "WARNING: CH " << chID
                     << " : GTX clock is not running - skipping."
                     << endl;
            }
            else
            {
                current_link->clearAllGtxErrorCounters();

                /** also clear GTX error counter for HWTest firmwares */
                if ( type_channels>>16 == RORC_CFG_PROJECT_hwtest )
                { current_link->setGtxReg(RORC_REG_GTX_ERROR_CNT, 0); }
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

        if( do_rxeqmix )
        {
            gtxasynccfg &= ~(7<29);
            gtxasynccfg |= ((rxeqmix&7)<<29);
        }

        if( do_txdiffctrl )
        {
            gtxasynccfg &= ~(0xf<16);
            gtxasynccfg |= ((txdiffctrl&0xf)<<16);
        }

        if( do_txpreemph )
        {
            gtxasynccfg &= ~(0xf<20);
            gtxasynccfg |= ((txpreemph&0xf)<<20);
        }

        if( do_txpostemph )
        {
            gtxasynccfg &= ~(0xf<24);
            gtxasynccfg |= ((txpostemph&0xf)<<24);
        }

        if ( do_reset || do_loopback || do_rxeqmix ||
                do_txdiffctrl || do_txpreemph || do_txpostemph )
        {
            /** write new values to RORC */
            current_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, gtxasynccfg);
        }

        if ( do_pllcfg )
        {
            /** set GTXRESET */
            gtxasynccfg |= 0x00000001;
            current_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, gtxasynccfg);

            /** Write new PLL config */
            current_link->drpSetPllConfig(available_configs[pllcfgnum]);

            /** release GTXRESET */
            gtxasynccfg &= ~(0x00000001);
            current_link->setPciReg(RORC_REG_GTX_ASYNC_CFG, gtxasynccfg);
        }

        if ( do_dump )
        {
            /** dump all DRP Registers */
            for ( int i=0x00; i<=0x4f; i++ )
            {
                cout << hex << setw(2) << i << ": 0x"
                     << setw(4) << setfill('0')
                     << current_link->drpRead(i) << endl;
            }
        }

        delete current_link;
    }

    delete bar;
    delete dev;

    return 0;
}
