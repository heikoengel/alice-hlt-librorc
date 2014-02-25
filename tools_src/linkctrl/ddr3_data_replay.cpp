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
        -f [path]     Load DDL file(s) for replay, \n\
                      separate multiple files with \",\" \n\
        -e [0/1]      Enable/disable data replay channel \n\
        -g [0/1]      Global enable/disable of data replay engine \n\
"


int main
(
    int argc,
    char *argv[]
)
{
    
   /** parse command line arguments **/
    int32_t DeviceId  = -1;
    int32_t ChannelId = -1;
    uint32_t channel_enable_val = 0;
    uint32_t global_enable_val = 0;
    char *filename = NULL;

    int do_channel_enable = 0;
    int do_global_enable = 0;
    int do_load_file = 0;
    int arg;
     
    while( (arg = getopt(argc, argv, "hn:c:f:e:g:")) != -1 )
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

    librorc::link *link = new librorc::link(bar, ChannelId);

#ifdef SIM
    /** wait for phy_init_done */
    while( !(bar->get32(RORC_REG_DDR3_CTRL) & (1<<1)) )
    { usleep(100); }
#endif

    /**
     * TODO: get module size, divide by number of DDLs, set start_addr, ...
     * TODO: confirm phy_init_done==1
     **/
    uint32_t ch_start_addr = 0x00000000;
    

    if ( do_load_file )
    {
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

        cout << "Writing event to DDR3..." << endl;
        try {
            sm->ddr3DataReplayEventToRam(
                    event,
                    (fd_in_stat.st_size>>2), // num_dws
                    ch_start_addr, // ddr3 start address
                    0, // channel
                    true); // last event
        }
        catch( int e )
        {
            cout << "Exception while writing event: " << e << endl;
            abort();
        }
    }

    if ( do_channel_enable )
    {
        if ( channel_enable_val )
        {
            link->setDataSourceDdr3DataReplay();
            link->configureDdr3DataReplayChannel(ch_start_addr);
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
