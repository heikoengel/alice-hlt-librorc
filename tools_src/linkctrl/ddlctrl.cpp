/**
 * @file ddlctrl.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-10-01
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

#define HELP_TEXT "ddlctrl usage: \n\
        ddlctrl [parameters] \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -e [0,1]      DDLIF Enable \n\
        -r [0,1]      Set DDL Reset \n\
        -p [0,1]      PG Enable \n\
        -m [0..3]     PG Mode \n\
        -P [0,1]      PRBS EventSize mode enable \n\
        -C [cmd]      Send Command (DIU only) \n\
        -s [size]     PG EventSize or PRBS masks (see below) \n\
        -f [0,1]      FlowControl Enable \n\
        -N [num]      Number of events or 0 for continuous mode\n\
        -M [0,1]      EventStream MUX: 0->DDL, 1->PG \n\
        -W [pattern]  Set initial PG pattern word \n\
        -x            Clear counters \n\
\n\
In PRBS EventSize mode: \n\
[size] parameter consists of {prbs_mask[31:16], prbs_min_size[15:0]}, where\n\
* {prbs_mask, 0xffff} is applied with logical-AND, and \n\
* {0x0000, prbs_min_size} with logical-OR \n\
to a 32bit PRBS Event Size value.\n\
"

void
dump_channel_status
(
    uint32_t chID,
    uint32_t ddlctrl,
    uint32_t fwtype
)
{
    cout << "CH" << chID
         << " EN:" << (ddlctrl&1)
         << " FC:" << ((ddlctrl>>1)&1)
         << " LF_N:" << ((ddlctrl>>4)&1);
    if ( fwtype ==  RORC_CFG_PROJECT_hlt_out )
    {
        cout << " OPEN:";
    }
    else
    {
        cout << " LD_N:";
    }
    cout << ((ddlctrl>>5)&1)
         << " XOFF:" << ((ddlctrl>>6)&1)
         << " PG_EN:" << ((ddlctrl>>8)&1)
         << " PG_ADAP:" << ((ddlctrl>>9)&1)
         << " PG_CONT:" << ((ddlctrl>>10)&1)
         << " PG_PRBS:" << ((ddlctrl>>13)&1)
         << " roBSY_N:" << ((ddlctrl>>30)&1)
         << endl;
}




int main
(
    int argc,
    char *argv[]
)
{
    int do_clear = 0;
    int set_enable = 0;
    int set_pgenable = 0;
    int set_fc = 0;
    int set_pgmode = 0;
    int set_pgsize = 0;
    int set_ddlcmd = 0;
    int set_pgnevents = 0;
    int set_mux = 0;
    int set_prbssize = 0;
    int set_pattern = 0;
    int set_reset = 0;

    uint32_t enable = 0;
    uint32_t reset = 0;
    uint32_t pgenable = 0;
    uint32_t pgsize = 0;
    uint32_t fc = 0;
    uint32_t pgmode = 0;
    uint32_t ddlcmd = 0;
    uint32_t pgnevents = 0;
    uint32_t mux = 0;
    uint32_t prbssize = 0;
    uint32_t pattern = 0;


   /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    int arg;
    while( (arg = getopt(argc, argv, "hn:c:e:p:m:C:s:f:xN:M:P:W:r:")) != -1 )
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

            case 'x':
            {
                do_clear = 1;
            }
            break;

            case 'e':
            {
                enable = strtol(optarg, NULL, 0);
                set_enable = 1;
            }
            break;

            case 'r':
            {
                reset = strtol(optarg, NULL, 0);
                set_reset = 1;
            }
            break;

            case 'p':
            {
                pgenable = strtol(optarg, NULL, 0);
                set_pgenable = 1;
            }
            break;

            case 'f':
            {
                fc = strtol(optarg, NULL, 0);
                set_fc = 1;
            }
            break;

            case 'm':
            {
                pgmode = strtol(optarg, NULL, 0);
                set_pgmode = 1;
            }
            break;

            case 'P':
            {
                prbssize = strtol(optarg, NULL, 0);
                set_prbssize = 1;
            }
            break;

            case 's':
            {
                pgsize = strtol(optarg, NULL, 0);
                set_pgsize = 1;
            }
            break;

            case 'C':
            {
                ddlcmd = strtol(optarg, NULL, 0);
                set_ddlcmd = 1;
            }
            break;

            case 'N':
            {
                pgnevents = strtol(optarg, NULL, 0);
                set_pgnevents = 1;
            }
            break;

            case 'M':
            {
                mux = strtol(optarg, NULL, 0);
                set_mux = 1;
            }
            break;

            case 'W':
            {
                pattern = strtol(optarg, NULL, 0);
                set_pattern = 1;
            }
            break;

            case 'h':
            {
                cout << HELP_TEXT;
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
    else if( dev->DMAChannelIsImplemented(ChannelId) )
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

        /** check if GTX domain is up */
        if ( !current_link->isGtxDomainReady() )
        {
            cout << "GTX Domain not ready for channel "
                 << chID << " - skipping..." << endl;
            continue;
        }

        if ( do_clear )
        {
            /** clear event count */
            current_link->setGTX(RORC_REG_DDL_EC, 0);
            /** clear deadtime counter */
            current_link->setGTX(RORC_REG_DDL_DEADTIME, 0);
            /** clear DDL status words */
            current_link->setGTX(RORC_REG_DDL_CTSTW, 0);
            current_link->setGTX(RORC_REG_DDL_FESTW, 0);
            current_link->setGTX(RORC_REG_DDL_DTSTW, 0);
            current_link->setGTX(RORC_REG_DDL_IFSTW, 0);
        }

        /** get current config */
        uint32_t ddlctrl = current_link->GTX(RORC_REG_DDL_CTRL);

        /** enable/disable DDL Interface */
        if ( set_enable )
        {
            ddlctrl &= ~(1<<0);
            ddlctrl |= (enable&1);
        }

        /** enable/disable DDL Component */
        if ( set_reset)
        {
            ddlctrl &= ~(1<<31);
            ddlctrl |= ((reset&1)<<31);
        }

        /** enable/disable PatternGenerator */
        if ( set_pgenable )
        {
            /** bit 8: PG Enable */
            ddlctrl &= ~(1<<8);
            ddlctrl |= ((pgenable&1)<<8);
        }

        /** enable/disable PRBS Event Size */
        if ( set_prbssize )
        {
            ddlctrl &= ~(1<<13);
            ddlctrl |= ((prbssize&1)<<13);
        }

        /** set number of events - if non-zero set continuous-bit */
        if ( set_pgnevents )
        {
            if ( pgnevents )
            {
                ddlctrl &= ~(1<<10); //clear continuous-bit
            } 
            else
            {
                ddlctrl |= (1<<10); //set continuous-bit
            }
            current_link->setGTX(RORC_REG_DDL_PG_NUM_EVENTS, pgnevents);
        }

        /** set PatternGenerator Event Size or PRBS Masks */
        if ( set_pgsize )
        {
            current_link->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, pgsize);
        }

        /** set PatternGenerator Mode:
         * 0: increment
         * 1: shift
         * 2: decrement
         * 3: toggle
         * */
        if ( set_pgmode )
        {
            /** clear bits 12:11 and set new value */
            ddlctrl &= ~(3<<11);
            ddlctrl |= ((pgmode&3)<<11);
        }


        /** send DIU command */
        if ( set_ddlcmd )
        {
            if ((type_channels>>16) == RORC_CFG_PROJECT_hlt_in )
            {
                current_link->setGTX(RORC_REG_DDL_CMD, ddlcmd);
            }
            else
            {
                cout << "Current FW is not HLT_IN, cannot send DIU command."
                     << endl;
            }
        }

        /** set initial PG pattern */
        if ( set_pattern )
        {
            current_link->setGTX(RORC_REG_DDL_PG_PATTERN, pattern);
        }


        if ( set_mux )
        {
            /** set EventStream Multiplexer:
             * Setting | HLT_IN   | HLT_OUT
             * 0       | from DIU | from host
             * 1       | from PG  | from PG
             * */
            ddlctrl &= ~(1<<3);
            ddlctrl |= ((mux&1)<<3);
        }

        /** set flow control */
        if ( set_fc )
        {
            if(fc)
            { current_link->enableFlowControl(); }
            else
            { current_link->disableFlowControl(); }
        }

        if ( set_fc || set_enable || set_pgmode || set_pgenable ||
                set_pgnevents || set_prbssize || set_mux || set_reset )
        {
            current_link->setGTX(RORC_REG_DDL_CTRL, ddlctrl);
        }

        dump_channel_status(chID, ddlctrl, type_channels>>16);

        delete current_link;
    }

    delete bar;
    delete dev;

    return 0;
}

