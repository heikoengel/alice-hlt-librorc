/**
 * @file ddr3_data_replay.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-02-03
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

#include <librorc.h>
#include <vector>

using namespace std;


#define HELP_TEXT "ddr3_data_replay usage: \n\
        ddr3_data_replay [parameters] \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -f [path]     Load DDL file for replay, \n\
        -s [addr]     override default DDR3 start address \n\
        -l            Current event is the last to be replayed \n\
        -e [0/1]      Enable/disable data replay channel \n\
        -o [0/1]      Set One-Shot replay mode \n\
        -C [0/1]      Enable/disable continuous replay mode \n\
        -r [0/1]      Enable/disable channel reset\n\
Note: channel enable (-e) and global enable (-g) are applied AFTER \n\
loading a file with -f. \n\
"

#define HEX32(x) setw(8) << setfill('0') << hex << x << setfill(' ')

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
        cout << "ERROR: Failed to read from DDR3 SPD on SO-DIMM "
             << module_number << endl;
        abort();
    }
    return total_cap;
}


int main
(
    int argc,
    char *argv[]
)
{

   /** parse command line arguments **/
    int32_t DeviceId  = -1;
    uint32_t ControllerId;
    int32_t ChannelId = -1;
    uint32_t channel_enable_val = 0;
    //char *filename = NULL;
    vector<string> list_of_filenames;
    vector<string>::iterator iter;
    uint64_t module_size = 0;
    uint64_t max_ctrl_size = 0;
    uint64_t default_start_addr = 0;
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
    int arg;

    while( (arg = getopt(argc, argv, "hn:c:f:e:s:o:C:r:")) != -1 )
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
                /*filename = (char *)malloc(strlen(optarg)+2);
                sprintf(filename, "%s", optarg);*/
                do_load_file = true;
            }
            break;

            case 's':
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
    #ifdef SIM
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

    if ( ChannelId < 0 || ChannelId >= (int)sm->numberOfChannels() )
    {
        cout << "ERROR: no or invalid channel selected: " << ChannelId
             << endl;
        abort();
    }

    /**
     * get ControllerId from selected channel:
     * DDLs 0 to 5 are fed from SO-DIMM 0
     * DDLs 6 to 11 are fed from DO-DIMM 1
     **/
    ControllerId = (ChannelId<6) ? 0 : 1;

    /** get size of RAM module */
    module_size = getDdr3ModuleCapacity(sm, ControllerId);
    max_ctrl_size = sm->ddr3ControllerMaxModuleSize(ControllerId);

    /**
     * get start address:
     * divide module size into 8 partitions. We can have up to 6
     * channels per module, so dividing by 6 is also fine here,
     * but /8 gives the nicer boundaries.
     * The additional factor 8 comes from the data width of the
     * DDR3 interface: 64bit = 8 byte for each address.
     * 1/(8*8) = 2^(-6) => shift right by 6 bit
     **/
    if(max_ctrl_size>=module_size)
    { default_start_addr = ChannelId*(module_size>>6); }
    else
    { default_start_addr = ChannelId*(max_ctrl_size>>6); }

    /** override default start address with provided value */
    if( start_addr_ovrd_set )
    { start_addr = start_addr_ovrd; }
    else
    { start_addr = default_start_addr; }

    /** create link instance */
    librorc::link *link = new librorc::link(bar, ChannelId);

    if( link->isGtxDomainReady() &&
            link->ddr3DataReplayAvailable() )
    {
        cout << "ERROR: Data Replay not available on current channel!"
             << endl;
        abort();
    }

    librorc::datareplaychannel *dr = new librorc::datareplaychannel( link );

#ifdef SIM
    /** wait for phy_init_done */
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

        for(
                iter = list_of_filenames.begin();
                iter != list_of_filenames.end();
                iter++
           )
        {
            if( iter == (list_of_filenames.end()-1) )
            { is_last_event = true; }

            const char *filename = (*iter).c_str();

            int fd_in = open(filename, O_RDONLY);
            if ( fd_in==-1 )
            {
                cout << "ERROR: Failed to open input file" << endl;
                abort();
            }
            struct stat fd_in_stat;
            fstat(fd_in, &fd_in_stat);

            uint32_t *event = (uint32_t *)mmap(NULL, fd_in_stat.st_size,
                    PROT_READ, MAP_SHARED, fd_in, 0);
            if ( event == MAP_FAILED )
            {
                cout << "ERROR: failed to mmap input file" << endl;
                abort();
            }

            cout << "Writing " << (*iter) << " ("
                << dec << fd_in_stat.st_size
                << " B) to DDR3 SO-DIMM"
                << ControllerId << ", starting at address 0x"
                << HEX32(next_addr) << endl;
            try {
                next_addr = sm->ddr3DataReplayEventToRam(
                        event,
                        (fd_in_stat.st_size>>2), // num_dws
                        next_addr, // ddr3 start address
                        ChannelId, // channel
                        is_last_event); // last event
            }
            catch( int e )
            {
                cout << "Exception while writing event: " << e << endl;
                abort();
            }

            munmap(event, fd_in_stat.st_size);
            close(fd_in);
        }

        cout << "Done." << endl;
        cout << "Closed channel; "
             << "Start address for next channel could be 0x"
             << HEX32(next_addr) << endl;
    }

    if( start_addr_ovrd_set )
    {
        dr->setStartAddress(start_addr);
    }

    if( set_oneshot )
    {
        dr->setModeOneshot(oneshot);
    }

    if( set_channel_reset )
    {
        dr->setReset(channel_reset);
    }

    if( set_continuous )
    {
        dr->setModeContinuous(continuous);
    }

    if ( do_channel_enable )
    {
        dr->setEnable(channel_enable_val);
    }

    /**
     * print status
     **/
    cout << "SO-DIMM " << ControllerId << endl
         << "\tModule Capacity: " << dec
         << (module_size>>20) << " MB" << endl
         << "\tMax. Controller Capacity: " << dec
         << (max_ctrl_size>>20) << " MB" << endl
         << "\tDefault Start Address: 0x"
         << HEX32(default_start_addr) << endl;

    cout << "Channel " << ChannelId << " Config:" << endl
         << "\tStart Address: " << hex
         << dr->startAddress() << dec << endl
         << "\tReset: " << dr->isInReset() << endl
         << "\tContinuous: " << dr->isContinuousEnabled() << endl
         << "\tOneshot: " << dr->isOneshotEnabled() << endl
         << "\tEnabled: " << dr->isEnabled() << endl;

    cout << "Channel " << ChannelId << " Status:" << endl
         << "\tNext Address: " << hex
         << dr->nextAddress() << dec << endl
         << "\tWaiting: " << dr->isWaiting() << endl
         << "\tDone: " << dr->isDone() << endl;

    delete dr;
    delete link;
    delete sm;
    delete bar;
    delete dev;
    return 0;
}
