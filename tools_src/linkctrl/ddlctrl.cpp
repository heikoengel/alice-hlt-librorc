/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#include <iostream>
#include <cstdio>
#include <getopt.h>
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
        -S            Read remote serial \n\
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
    int set_readRemoteSerial = 0;

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
    while( (arg = getopt(argc, argv, "hn:c:e:p:m:C:s:f:xN:M:P:W:r:S")) != -1 )
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

            case 'S':
            {
                set_readRemoteSerial = 1;
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
        bar = new librorc::bar(dev, 1);
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR1.\n");
        abort();
    }

    librorc::sysmon *sm;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        printf("ERROR: failed to initialize Sysmon.\n");
        abort();
    }

    uint32_t startChannel, endChannel;
    uint32_t nChannels = sm->numberOfChannels();
    if ( ChannelId==-1 )
    {
        /** no specific channel selected, iterate over all channels */
        startChannel = 0;
        endChannel = nChannels - 1;
    }
    else if( ChannelId < (int)nChannels )
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

        uint32_t linkType = current_link->linkType();
        bool pgAvail = current_link->patternGeneratorAvailable();

        /** check if GTX domain is up */
#ifdef MODELSIM
        current_link->waitForGTXDomain();
#endif
        if ( !current_link->isDdlDomainReady() )
        {
            cout << "DDL Domain not ready for channel "
                 << chID << " - skipping..." << endl;
            continue;
        }

        if( !pgAvail && (set_pattern || set_pgenable || set_pgmode ||
                    set_pgsize || set_prbssize || set_pgnevents) )
        {
            cout << "WARNING: PatternGenerator not available for channel "
                 << chID << " - skipping all PG related commands." << endl;
        }

        if ( do_clear )
        {
            switch( linkType )
            {
                case RORC_CFG_LINK_TYPE_DIU:
                    {
                        librorc::diu *diu = new librorc::diu(current_link);
                        diu->clearAllLastStatusWords();
                        diu->clearDdlDeadtime();
                        diu->clearDmaDeadtime();
                        diu->clearEventcount();
                        delete diu;
                    }
                    break;

                case RORC_CFG_LINK_TYPE_SIU:
                    {
                        librorc::siu *siu = new librorc::siu(current_link);
                        siu->clearLastFrontEndCommandWord();
                        siu->clearDdlDeadtime();
                        siu->clearDmaDeadtime();
                        siu->clearEventcount();
                        delete siu;
                    }
                    break;

                case RORC_CFG_LINK_TYPE_VIRTUAL:
                    {
                        librorc::ddl *ddl = new librorc::ddl(current_link);
                        ddl->clearDmaDeadtime();
                        delete ddl;
                    }
                    break;

                default: // RORC_CFG_LINK_TYPE_IBERT
                    break;
            }
        }

        /** get current config */
        uint32_t ddlctrl = current_link->ddlReg(RORC_REG_DDL_CTRL);

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
        if ( set_pgenable && pgAvail )
        {
            /** bit 8: PG Enable */
            ddlctrl &= ~(1<<8);
            ddlctrl |= ((pgenable&1)<<8);
        }

        /** enable/disable PRBS Event Size */
        if ( set_prbssize && pgAvail )
        {
            ddlctrl &= ~(1<<13);
            ddlctrl |= ((prbssize&1)<<13);
        }

        /** set number of events - if non-zero set continuous-bit */
        if ( set_pgnevents && pgAvail )
        {
            if ( pgnevents )
            {
                ddlctrl &= ~(1<<10); //clear continuous-bit
            }
            else
            {
                ddlctrl |= (1<<10); //set continuous-bit
            }
            current_link->setDdlReg(RORC_REG_DDL_PG_NUM_EVENTS, pgnevents);
        }

        /** set PatternGenerator Event Size or PRBS Masks */
        if ( set_pgsize && pgAvail )
        {
            current_link->setDdlReg(RORC_REG_DDL_PG_EVENT_LENGTH, pgsize);
        }

        /** set PatternGenerator Mode:
         * 0: increment
         * 1: shift
         * 2: decrement
         * 3: toggle
         * */
        if ( set_pgmode && pgAvail )
        {
            /** clear bits 12:11 and set new value */
            ddlctrl &= ~(3<<11);
            ddlctrl |= ((pgmode&3)<<11);
        }


        /** send DIU command */
        if ( set_ddlcmd || set_readRemoteSerial )
        {
            if( linkType==RORC_CFG_LINK_TYPE_DIU )
            {
                librorc::diu *diu = new librorc::diu(current_link);
#ifdef MODELSIM
                diu->waitForLinkUp();
                // waiting for SIU ready the dirty way...
                for( int i=0; i<20; i++ )
                { diu->readAndClearDiuInterfaceStatus(); }
#endif
                if( diu->linkUp() )
                {
                    if( set_ddlcmd )
                    { diu->sendCommand(ddlcmd); }
                    else
                    { cout << diu->readRemoteSerial() << endl; }
                }
                else
                {
                    cout << "WARNING: Channel " << chID << " is down - "
                         << " not sending command." << endl;
                }
                delete diu;
            }
            else
            {
                cout << "WARNING: Channel " << chID
                     << "does not have a DIU - cannot send DIU command."
                     << endl;
            }
        }

        /** set initial PG pattern */
        if ( set_pattern && pgAvail )
        {
            current_link->setDdlReg(RORC_REG_DDL_PG_PATTERN, pattern);
        }


        if ( set_mux )
        {
            if( (mux==1 && !current_link->ddr3DataReplayAvailable()) ||
                    (mux==2 && !pgAvail) )
            {
                cout << "WARNING: multiplexer setting is not valid for this channel."
                     << endl;
            }
            /** set EventStream Multiplexer:
             * Setting | HLT_IN   | HLT_IN_FCF | HLT_OUT
             * 0       | from DIU | from DIU   | from host
             * 1       | from DDR | from DDR   | -
             * 2       | from PG  | -          | from PG
             * */
            ddlctrl &= ~(3<<16);
            ddlctrl |= ((mux&3)<<16);
        }

        /** set flow control */
        if ( set_fc )
        {
            ddlctrl &= ~(1<<1); // clear flow control bit
            ddlctrl |= ((fc&1)<<1);
        }

        if ( set_fc || set_enable || set_pgmode || set_pgenable ||
                set_pgnevents || set_prbssize || set_mux || set_reset )
        {
            current_link->setDdlReg(RORC_REG_DDL_CTRL, ddlctrl);
        }

        dump_channel_status(chID, ddlctrl, sm->firmwareType());

        delete current_link;
    }

    delete sm;
    delete bar;
    delete dev;

    return 0;
}

