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

using namespace std;


#define HELP_TEXT "ddr3_data_replay usage: \n\
        ddr3_data_replay [parameters] \n\
        -n [0...255]  Target device ID \n\
        -c [0...11]   Channel ID \n\
        -f [path]     Load DDL file for replay, \n\
        -s [addr]     DDR3 start address \n\
        -l            Current event is the last to be replayed \n\
        -e [0/1]      Enable/disable data replay channel \n\
        -o [0/1]      Set One-Shot replay mode \n\
        -C [0/1]      Enable/disable continuous replay mode \n\
        -r [0/1]      Enable/disable channel reset\n\
        -g [0/1]      Global enable/disable of data replay engine \n\
Note: channel enable (-e) and global enable (-g) are applied AFTER \n\
loading a file with -f. \n\
"

#define HEX32(x) setw(8) << setfill('0') << hex << x << setfill(' ')

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
    uint32_t global_enable_val = 0;
    char *filename = NULL;
    uint32_t start_addr = 0;

    int do_channel_enable = 0;
    int do_global_enable = 0;
    int do_load_file = 0;
    int start_addr_set = 0;
    bool is_last_event = false;
    bool set_oneshot = false;
    uint32_t oneshot = 0;
    bool set_continuous = false;
    uint32_t continuous = 0;
    bool set_channel_reset = false;
    uint32_t channel_reset = 0;
    int arg;
     
    while( (arg = getopt(argc, argv, "hn:c:f:e:g:s:lo:C:r:")) != -1 )
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

            case 'g':
            {
                global_enable_val = strtol(optarg, NULL, 0);
                do_global_enable = 1;
            }
            break;

            case 'f':
            {
                filename = (char *)malloc(strlen(optarg)+2);
                sprintf(filename, "%s", optarg);
                do_load_file = 1;
            }
            break;

            case 's':
            {
                start_addr = strtol(optarg, NULL, 0);
                start_addr_set = 1;
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

            case 'l':
            {
                is_last_event = true;
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

    /**
     * make sure channel start address is provided when loading a
     * DDL file or enabling a channel
     **/
    if( do_load_file || (do_channel_enable && channel_enable_val) )
    {
        if( !start_addr_set )
        {
            cout << "ERROR: Please provide a channel start address for this operation!" << endl;
            abort();
        }
    }

    /** check channel start address alignment */
    if( start_addr_set )
    {
        if( start_addr & 0x00000003 )
        {
            cout << "ERROR: start address must be a 32bit aligned." << endl;
            abort();
        }
        else if( start_addr & 0x000000ff )
        {
            cout << "WARNING: start address is not a multiple of 256 byte "
                 << "- are you sure you know what you're doing?!" << endl;
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

    if ( !sm->firmwareIsHltInFcf() && !sm->firmwareIsHltIn() )
    {
        cout << "ERROR: no HLT_IN_FCF firmware with DataReplay capabilities detected!" << endl;
        abort();
    }

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

    /** create link instance */
    librorc::link *link = new librorc::link(bar, ChannelId);

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

    /**
     * TODO: do we want automatic partitioning of available DDR3 space?
     * -> get module size, divide by number of DDLs, set start_addr, ...
     **/
    if ( do_load_file )
    {
        uint32_t next_addr;
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

        cout << "Writing event to DDR3 SO-DIMM" << ControllerId
            << ", starting at address 0x"
            << HEX32(start_addr) << endl;
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
            cout << "Exception while writing event: " << e << endl;
            abort();
        }

        cout << "Done." << endl;
        if( is_last_event )
        {
            cout << "Closed channel; "
                 << "Start address for next channel could be 0x"
                 << HEX32(next_addr) << endl;
        }
        else
        {
             cout << "Start address for next event would be 0x"
             << HEX32(next_addr) << endl;
        }

        munmap(event, fd_in_stat.st_size);
        close(fd_in);
    }

    if( start_addr_set )
    {
        link->configureDdr3DataReplayChannel(start_addr);
    }

    if( set_oneshot )
    {
        link->setDdr3DataReplayChannelOneshot(oneshot);
    }

    if( set_channel_reset )
    {
        link->setDdr3DataReplayChannelReset(channel_reset);
    }

    if( set_continuous )
    {
        link->setDdr3DataReplayChannelContinuous(continuous);
    }

    if ( do_channel_enable )
    {
        if ( channel_enable_val )
        {
            link->enableDdr3DataReplayChannel();
        }
        else
        {
            link->disableDdr3DataReplayChannel();
        }
    }

    if ( do_global_enable )
    {
        // enable/disable data replay globally
        if ( global_enable_val )
        { sm->enableDdr3DataReplay(); }
        else
        { sm->disableDdr3DataReplay(); }
    }

    delete link;
    delete sm;
    delete bar;
    delete dev;
    return 0;
}
