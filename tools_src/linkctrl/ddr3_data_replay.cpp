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

#include <librorc.h>
#include <vector>

using namespace std;


#define HELP_TEXT "ddr3_data_replay usage: \n\
        ddr3_data_replay [parameters] \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -f [path]     Load DDL file for replay, \n\
        -S [addr]     override default DDR3 start address \n\
        -e [0/1]      Enable/disable data replay channel \n\
        -o [0/1]      Set One-Shot replay mode \n\
        -C [0/1]      Enable/disable continuous replay mode \n\
        -r [0/1]      Enable/disable channel reset \n\
        -s            Show Status \n\
        -D            Disable replay gracefully \n\
Note: channel enable (-e) is applied AFTER \n\
loading a file or setting the mode. \n\
"

#define HEX32(x) setw(8) << setfill('0') << hex << x << setfill(' ')


/********** Prototypes **********/
uint64_t
getDdr3ModuleCapacity
(
    librorc::sysmon *sm,
    uint8_t module_number
);

void
print_channel_status
(
    uint32_t ChannelId,
    librorc::datareplaychannel *dr,
    uint32_t default_start_addr
);

void
print_module_status
(
    uint32_t ControllerId,
    uint32_t module_size,
    uint32_t max_ctrl_size
);

uint32_t
fileToRam
(
    librorc::sysmon *sm,
    uint32_t ChannelId,
    const char *filename,
    uint32_t start_addr,
    bool is_last_event
);

int
waitForReplayDone
(
    librorc::datareplaychannel *dr
);

/********** Main **********/
int main
(
    int argc,
    char *argv[]
)
{

   /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelIdSelect = -1;
    uint32_t channel_enable_val = 0;
    //char *filename = NULL;
    vector<string> list_of_filenames;
    uint64_t module_size[2] = { 0, 0 };
    uint64_t max_ctrl_size[2] = { 0, 0 };
    uint32_t start_addr = 0;
    uint32_t start_addr_ovrd = 0;

    int do_channel_enable = 0;
    bool do_load_file = false;
    int start_addr_ovrd_set = 0;
    bool set_oneshot = false;
    uint32_t oneshot = 0;
    bool set_continuous = false;
    uint32_t continuous = 0;
    bool set_channel_reset = false;
    uint32_t channel_reset = 0;
    bool do_show_status = false;
    bool do_disable = false;
    int arg;

    while( (arg = getopt(argc, argv, "hn:c:f:e:S:o:C:r:sD")) != -1 )
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
                ChannelIdSelect = strtol(optarg, NULL, 0);
            }
            break;

            case 'e':
            {
                channel_enable_val = strtol(optarg, NULL, 0);
                do_channel_enable = 1;
            }
            break;

            case 'f':
            {
                /** add filename to list */
                list_of_filenames.push_back(optarg);
                do_load_file = true;
            }
            break;

            case 'S':
            {
                start_addr_ovrd = strtol(optarg, NULL, 0);
                start_addr_ovrd_set = 1;
            }
            break;

            case 'o':
            {
                oneshot = strtol(optarg, NULL, 0);
                set_oneshot = true;
            }
            break;

            case 'r':
            {
                channel_reset = strtol(optarg, NULL, 0);
                set_channel_reset = true;
            }
            break;

            case 'C':
            {
                continuous = strtol(optarg, NULL, 0);
                set_continuous = true;
            }
            break;

            case 'D':
            {
                do_disable = true;
            }
            break;

            case 's':
            {
                do_show_status = true;
            }
            break;

            default:
            {
                cout << "Unknown parameter (" << arg << ")!" << endl;
                cout << HELP_TEXT;
                return -1;
            }
            break;
        } //switch
    } //while

    if ( DeviceId < 0 || DeviceId > 255 )
    {
        cout << "No or invalid device selected: " << DeviceId << endl;
        cout << HELP_TEXT;
        abort();
    }

    /** check channel start address alignment */
    if( start_addr_ovrd_set )
    {
        if( start_addr_ovrd & 0x00000007 )
        {
            cout << "ERROR: start address must be a multiple of 8." << endl;
            abort();
        }
    }

    /** Instantiate device **/
    librorc::device *dev = NULL;
    try{
        dev = new librorc::device(DeviceId);
    }
    catch(...)
    {
        cout << "Failed to intialize device " << DeviceId
            << endl;
        return -1;
    }

    /** Instantiate a new bar */
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
        cout << "ERROR: failed to initialize BAR." << endl;
        delete dev;
        abort();
    }

    librorc::sysmon *sm = new librorc::sysmon(bar);
    int nchannels = sm->numberOfChannels();
    uint32_t startChannel, endChannel;

    if( ChannelIdSelect >= nchannels )
    {
        cout << "ERROR: invalid channel selected: "
             << ChannelIdSelect << endl;
        abort();
    }
    else if( ChannelIdSelect==-1 )
    {
        /** no specific channel selected, apply options to all channels */
        if( start_addr_ovrd_set )
        {
            cout << "ERROR: Start address override is not allowed "
                 << "without selecting a specific channel." << endl;
            abort();
        }

        startChannel = 0;
        endChannel = nchannels-1;
    }
    else
    {
        startChannel = ChannelIdSelect;
        endChannel = ChannelIdSelect;
    }


    /** get size of RAM modules */
    if( startChannel<6)
    {
        module_size[0] = getDdr3ModuleCapacity(sm, 0);
        max_ctrl_size[0] = sm->ddr3ControllerMaxModuleSize(0);
        if( do_show_status )
        { print_module_status(0, module_size[0], max_ctrl_size[0]); }
    }

    if( endChannel>5 )
    {
        module_size[1] = getDdr3ModuleCapacity(sm, 1);
        max_ctrl_size[1] = sm->ddr3ControllerMaxModuleSize(1);
        if( do_show_status )
        { print_module_status(1, module_size[1], max_ctrl_size[1]); }
    }


    /**
     * iterate over all selected DMA channels
     **/
    for( uint32_t ChannelId=startChannel;
            ChannelId<=endChannel;
            ChannelId++ )
    {
        uint64_t default_start_addr = 0;

        /**
         * get ControllerId from selected channel:
         * DDLs 0 to 5 are fed from SO-DIMM 0
         * DDLs 6 to 11 are fed from DO-DIMM 1
         **/
        uint32_t ControllerId = (ChannelId<6) ? 0 : 1;

        /** skip channel if no DDR3 module was detected */
        if( !module_size[ControllerId] )
        {
            continue;
        }

        /**
         * set default channel start address:
         * divide module size into 8 partitions. We can have up to 6
         * channels per module, so dividing by 6 is also fine here,
         * but /8 gives the nicer boundaries.
         * The additional factor 8 comes from the data width of the
         * DDR3 interface: 64bit = 8 byte for each address.
         * 1/(8*8) = 2^(-6) => shift right by 6 bit
         **/
        if(max_ctrl_size[ControllerId]>=module_size[ControllerId])
        { default_start_addr = ChannelId*(module_size[ControllerId]>>6); }
        else
        { default_start_addr = ChannelId*(max_ctrl_size[ControllerId]>>6); }

        /** override default start address with provided value */
        if( start_addr_ovrd_set )
        { start_addr = start_addr_ovrd; }
        else
        { start_addr = default_start_addr; }

        /** create link instance */
        librorc::link *link = new librorc::link(bar, ChannelId);

        if( !link->isGtxDomainReady() ||
                !link->ddr3DataReplayAvailable() )
        {
            cout << "INFO: Data Replay not available on channel "
                 << ChannelId << " - skipping..." << endl;
            continue;
        }

        /** create data replay channel instance */
        librorc::datareplaychannel *dr =
            new librorc::datareplaychannel( link );

#ifdef MODELSIM
        /** wait for phy_init_done in simulation */
        while( !(bar->get32(RORC_REG_DDR3_CTRL) & (1<<1)) )
        { usleep(100); }
#endif

        if( !sm->ddr3ModuleInitReady(ControllerId) )
        {
            cout << "ERROR: DDR3 Controller " << ControllerId
                << " is not ready - doing nothing." << endl;
            abort();
        }

        if ( do_load_file )
        {
            uint32_t next_addr = start_addr;
            bool is_last_event = false;
            vector<string>::iterator iter;

            /** iterate over list of files */
            for( iter = list_of_filenames.begin();
                    iter != list_of_filenames.end();
                    iter++ )
            {
                if( iter == (list_of_filenames.end()-1) )
                { is_last_event = true; }

                const char *filename = (*iter).c_str();
                next_addr = fileToRam(sm, ChannelId, filename,
                        next_addr, is_last_event);
            }
            cout << "Done." << endl;
        }

        if( start_addr_ovrd_set || do_load_file )
        { dr->setStartAddress(start_addr); }

        if( set_oneshot )
        { dr->setModeOneshot(oneshot); }

        if( set_channel_reset )
        { dr->setReset(channel_reset); }

        if( set_continuous )
        { dr->setModeContinuous(continuous); }

        if ( do_channel_enable )
        { dr->setEnable(channel_enable_val); }

        if( do_disable )
        {
            if( dr->isInReset() )
            {
                // already in reset: disable and release reset
                dr->setEnable(0);
                dr->setReset(0);
            }
            else if( !dr->isEnabled() )
            {
                // not in reset, not enabled, this is where we want to end
                cout << "Channel " << ChannelId
                     << " is not enabled, skipping" << endl;
            }
            else
            {
                // enable OneShot if not already enabled
                if( !dr->isOneshotEnabled() )
                { dr->setModeOneshot(1); }

                // wait for OneShot replay to complete
                if( waitForReplayDone(dr)<0 )
                {
                    cout << "Timeout waiting for Replay-Done, skipping..."
                         << endl;
                }
                else
                {
                    // we are now at the end of an event, so it's safe
                    // to disable the channel
                    dr->setEnable(0);
                    // disable OneShot again
                    dr->setModeOneshot(0);
                    // toggle reset
                    dr->setReset(1);
                    dr->setReset(0);
                }
            }
        } // do_disable

        if( do_show_status )
        { print_channel_status(ChannelId, dr, default_start_addr); }

        delete dr;
        delete link;
    } // for loop: ChannelId

    delete sm;
    delete bar;
    delete dev;
    return 0;
}


uint64_t
getDdr3ModuleCapacity
(
    librorc::sysmon *sm,
    uint8_t module_number
)
{
    uint64_t total_cap = 0;
    try
    {
        uint8_t density = sm->ddr3SpdRead(module_number, 0x04);
        /** lower 4 bit: 0000->256 Mbit, ..., 0110->16 Gbit */
        uint64_t sd_cap = ((uint64_t)256<<(20+(density&0xf)));
        uint8_t mod_org = sm->ddr3SpdRead(module_number, 0x07);
        uint8_t n_ranks = ((mod_org>>3)&0x7) + 1;
        uint8_t dev_width = 4*(1<<(mod_org&0x07));
        uint8_t mod_width = sm->ddr3SpdRead(module_number, 0x08);
        uint8_t pb_width = 8*(1<<(mod_width&0x7));
        total_cap = sd_cap / 8 * pb_width / dev_width * n_ranks;
    } catch(...)
    {
        cout << "WARNING: Failed to read from DDR3 SPD on SO-DIMM "
             << (uint32_t)module_number << endl
             << "Is a module installed?" << endl;
        total_cap = 0;
    }
    return total_cap;
}

void
print_channel_status
(
    uint32_t ChannelId,
    librorc::datareplaychannel *dr,
    uint32_t default_start_addr
)
{
    cout << "Channel " << ChannelId << " Config:" << endl
        << "\tStart Address: " << hex
        << dr->startAddress() << dec << endl
        << "\tDefault Start Address: 0x"
        << HEX32(default_start_addr) << endl
        << "\tReset: " << dr->isInReset() << endl
        << "\tContinuous: " << dr->isContinuousEnabled() << endl
        << "\tOneshot: " << dr->isOneshotEnabled() << endl
        << "\tEnabled: " << dr->isEnabled() << endl;

    cout << "Channel " << ChannelId << " Status:" << endl
        << "\tNext Address: " << hex
        << dr->nextAddress() << dec << endl
        << "\tWaiting: " << dr->isWaiting() << endl
        << "\tDone: " << dr->isDone() << endl;
}


void
print_module_status
(
    uint32_t ControllerId,
    uint32_t module_size,
    uint32_t max_ctrl_size
)
{
    if(module_size)
    {
        cout << "SO-DIMM " << ControllerId << endl
             << "\tModule Capacity: " << dec
             << (module_size>>20) << " MB" << endl
             << "\tMax. Controller Capacity: " << dec
             << (max_ctrl_size>>20) << " MB" << endl;
    }
    else
    {
        cout << "SO-DIMM " << ControllerId
             << " no module detected" << endl;
    }
}


uint32_t
fileToRam
(
    librorc::sysmon *sm,
    uint32_t ChannelId,
    const char *filename,
    uint32_t start_addr,
    bool is_last_event
)
{
    uint32_t next_addr;
    int fd_in = open(filename, O_RDONLY);
    if ( fd_in==-1 )
    {
        cout << "ERROR: Failed to open input file"
             << filename << endl;
        abort();
    }
    struct stat fd_in_stat;
    fstat(fd_in, &fd_in_stat);

    uint32_t *event = (uint32_t *)mmap(NULL, fd_in_stat.st_size,
            PROT_READ, MAP_SHARED, fd_in, 0);
    if ( event == MAP_FAILED )
    {
        cout << "ERROR: failed to mmap input file"
             << filename << endl;
        abort();
    }

    try {
        next_addr = sm->ddr3DataReplayEventToRam(
                event,
                (fd_in_stat.st_size>>2), // num_dws
                start_addr, // ddr3 start address
                ChannelId, // channel
                is_last_event); // last event
    }
    catch( int e )
    {
        cout << "Exception " << e << " while writing event to RAM:" << endl
             << "File " << filename
             << " Channel " << ChannelId << " Addr " << hex
             << start_addr << dec << " LastEvent " << is_last_event << endl;
        abort();
    }

    munmap(event, fd_in_stat.st_size);
    close(fd_in);
    return next_addr;
}

int
waitForReplayDone
(
    librorc::datareplaychannel *dr
)
{
    uint32_t timeout = 10000;
    while( !dr->isDone() && (timeout>0) )
    {
        timeout--;
        usleep(100);
    }
    return (timeout==0) ? (-1) : 0;
}

