/**
 * @file hlt_out_writer.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-06-20
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/shm.h>
#include <getopt.h>

#include <librorc.h>
#include "../dma/dma_handling.hh"
#include "event_handling.h"
#include "event_generation.h"

using namespace std;

#define MAX_CHANNELS 12

DMA_ABORT_HANDLER



int main( int argc, char *argv[])
{
    int result = 0;
    librorc::device      *dev  = NULL;
    librorc::bar         *bar1 = NULL;
    librorc::buffer      *ebuf[MAX_CHANNELS];
    librorc::buffer      *rbuf[MAX_CHANNELS];
    librorc::dma_channel *ch[MAX_CHANNELS];

    timeval start_time, end_time;
    timeval last_time, cur_time;
    unsigned long last_bytes_received[MAX_CHANNELS];
    unsigned long last_events_received[MAX_CHANNELS];
    uint64_t ebuf_fill_state[MAX_CHANNELS];
    uint64_t nevents;
    uint64_t EventID[MAX_CHANNELS];

    DMAOptions opts[MAX_CHANNELS];
    opts[0] = evaluateArguments(argc, argv);
    int32_t nChannels = opts[0].channelId;
    opts[0].channelId = 0;
    opts[0].esType = LIBRORC_ES_OUT_SWPG;

    int i;

    for (i=1; i<nChannels; i++)
    {
        opts[i] = opts[0];
        opts[i].channelId = i;
    }
   
    DMA_ABORT_HANDLER_REGISTER

    librorcChannelStatus *chstats[MAX_CHANNELS];
    for ( i=0; i<nChannels; i++ )
    {
        chstats[i] = prepareSharedMemory(opts[i]);
        if(chstats[i] == NULL)
        { exit(-1); }
    }

   
    // create new device instance
    try{ dev = new librorc::device(opts[0].deviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }


    // bind to BAR1
    try
    {
    #ifdef SIM
        bar1 = new librorc::sim_bar(dev, 1);
    #else
        bar1 = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR1.\n");
        abort();
    }

    bar1->simSetPacketSize(32);

    // check if firmware is HLT_OUT
    if ( (bar1->get32(RORC_REG_TYPE_CHANNELS)>>16) != RORC_CFG_PROJECT_hlt_out )
    {
        cout << "Firmware is not HLT_OUT - exiting." << endl;
        abort();
    }


    for ( i=0; i<nChannels; i++ )
    {
        /** create new DMA event buffer */
        try
        { ebuf[i] = new librorc::buffer(dev, EBUFSIZE, 
                2*opts[i].channelId, 1, LIBRORC_DMA_TO_DEVICE); }
        catch(...)
        {
            perror("ERROR: ebuf->allocate");
            abort();
        }

        /** create new DMA report buffer */
        try
        { rbuf[i] = new librorc::buffer(dev, RBUFSIZE, 
                2*opts[i].channelId+1, 1, LIBRORC_DMA_FROM_DEVICE); }
        catch(...)
        {
            perror("ERROR: rbuf0->allocate");
            abort();
        }

        /** Create DMA channel */
        try
        {
            ch[i] = new librorc::dma_channel(opts[i].channelId, 
                    128, dev, bar1, ebuf[i], rbuf[i]);
            ch[i]->enable();
        }
        catch(...)
        {
            cout << "DMA channel failed!" << endl;
            abort();
        }
        
        // no event in EB now
        ebuf_fill_state[i] = 0;
        EventID[i] = 0;
        last_bytes_received[i] = 0;
        last_events_received[i] = 0;
    }
    //TODO: all SIU interface handling

    /** wait for GTX domain to be ready */
    //ch->waitForGTXDomain();

    /** set ENABLE, activate flow control (DIU_IF:busy), MUX=0 */
    //ch->setGTX(RORC_REG_DDL_CTRL, 0x00000003);

    // capture starting time
    bar1->gettime(&start_time, 0);
    last_time = start_time;
    cur_time = start_time;


    int32_t sanity_checks = CHK_SIZES|CHK_SOE;
    if(opts[0].useRefFile)
    {
        sanity_checks |= CHK_FILE;
    }
    else
    {
        sanity_checks |= CHK_PATTERN | CHK_ID;
    }

    // wait for RB entry
    while(!done)
    {
        for ( i=0; i<nChannels; i++ )
        {
            nevents = fill_eventbuffer(
                    rbuf[i], //report buffer instance
                    ebuf[i], //event buffer instance
                    ch[i], //channel instance
                    &ebuf_fill_state[i], // event buffer fill state
                    &EventID[i],
                    opts[i].eventSize // event size to be used for event generation
                    );
            if ( nevents > 0 )
            {
                DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                        "Pushed %ld events into EB\n", nevents);
            }

            result = handle_channel_data(
                    rbuf[i],
                    ebuf[i],
                    ch[i], // channe struct
                    chstats[i], // stats struct
                    sanity_checks, // do sanity check
                    NULL, // no DDL reference file
                    0); //DDL reference size

            if (result<0)
            {
                printf("handle_channel_data failed for channel %d\n",
                        opts[i].channelId);
            } else if (result==0)
            {
                usleep(100);
            }
        }

        
        bar1->gettime(&cur_time, 0);

        // print status line each second
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL) {
            for ( i=0; i<nChannels; i++ )
            {
                printf("CH%d Events OUT: %10ld, Size: %8.3f GB",
                        i, chstats[i]->n_events,
                        (double)chstats[i]->bytes_received/(double)(1<<30));

                if ( chstats[i]->bytes_received-last_bytes_received[i])
                {
                    printf(" Rate: %9.3f MB/s",
                            (double)(chstats[i]->bytes_received-last_bytes_received[i])/
                            gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
                } else {
                    printf(" Rate: -");
                }

                if ( chstats[i]->n_events - last_events_received[i])
                {
                    printf(" (%.3f kHz)",
                            (double)(chstats[i]->n_events-last_events_received[i])/
                            gettimeofday_diff(last_time, cur_time)/1000.0);
                } else {
                    printf(" ( - )");
                }
                printf(" Errors: %ld\n", chstats[i]->error_count);
                last_bytes_received[i] = chstats[i]->bytes_received;
                last_events_received[i] = chstats[i]->n_events;
            }
            last_time = cur_time;
        }

    }

    // EOR
    bar1->gettime(&end_time, 0);

    for ( i=0; i<nChannels; i++ )
    {
        // print summary
        printf("%ld Byte / %ld events in %.2f sec"
                "-> %.1f MB/s.\n",
                (chstats[i]->bytes_received), chstats[i]->n_events,
                gettimeofday_diff(start_time, end_time),
                ((float)chstats[i]->bytes_received/
                 gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

        if(!chstats[i]->set_offset_count) //avoid DivByZero Exception
            printf("CH%d: No Events\n", opts[i].channelId);
        else
            printf("CH%d: Events %ld, max_epi=%ld, min_epi=%ld, "
                    "avg_epi=%ld, set_offset_count=%ld\n", opts[i].channelId,
                    chstats[i]->n_events, chstats[i]->max_epi,
                    chstats[i]->min_epi,
                    chstats[i]->n_events/chstats[i]->set_offset_count,
                    chstats[i]->set_offset_count);

        // wait until EL_FIFO runs empty
        // wait for pending transfers to complete (dma_busy->0)

        // disable EBDM Engine
        ch[i]->disableEventBuffer();

        // disable RBDM
        ch[i]->disableReportBuffer();

        // reset DFIFO, disable DMA PKT
        ch[i]->setDMAConfig(0X00000002);

        // clear reportbuffer
        memset(rbuf[i]->getMem(), 0, rbuf[i]->getMappingSize());

        shmdt(chstats[i]);

        delete ch[i];
        delete ebuf[i];
        delete rbuf[i];
    }

    if (bar1)
        delete bar1;
    if (dev)
        delete dev;

    return result;
}
