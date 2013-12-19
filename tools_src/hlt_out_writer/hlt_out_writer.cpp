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
#include "dma_handling.hh"
#include "event_handling.h"
#include "event_generation.h"

using namespace std;


DMA_ABORT_HANDLER



int main( int argc, char *argv[])
{
    int result = 0;
    librorc::device      *dev  = NULL;
    librorc::bar         *bar1 = NULL;
    librorc::buffer      *ebuf = NULL;
    librorc::buffer      *rbuf = NULL;
    librorc::dma_channel *ch   = NULL;

    timeval start_time, end_time;
    timeval last_time, cur_time;
    unsigned long last_bytes_received;
    unsigned long last_events_received;
    uint64_t nevents;
    uint64_t EventID;

    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    struct stat ddlstat;
    if ( opts.useRefFile )
    {
        if ( stat(opts.refname, &ddlstat) == -1 )
        {
            perror("stat ddlfile");
            exit(-1);
        }
    }
    else
    {
        if ( !checkEventSize(opts.eventSize, argv[0]) )
        { exit(-1); }
    }

    DMA_ABORT_HANDLER_REGISTER

    librorcChannelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    // create new device instance
    try{ dev = new librorc::device(opts.deviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }

    /** Print some stats */
    printf("Bus %x, Slot %x, Func %x\n",
            dev->getBus(),
            dev->getSlot(),
            dev->getFunc());

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

    try
    {
        librorc::sysmon *sm = new librorc::sysmon(bar1);
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setw(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setw(8) << sm->FwBuildDate() << dec << endl;
        delete sm;
    }
    catch(...)
    { cout << "Firmware Rev. and Date not available!" << endl; }

    /** Check if requested channel is implemented in firmware */
    if( !dev->DMAChannelIsImplemented(opts.channelId) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
               "firmware - exiting\n", opts.channelId);
        abort();
    }

    // check if firmware is HLT_OUT
    if ( (bar1->get32(RORC_REG_TYPE_CHANNELS)>>16) != RORC_CFG_PROJECT_hlt_out )
    {
        cout << "Firmware is not HLT_OUT - exiting." << endl;
        abort();
    }

    /** create new DMA event buffer */
    try
    { ebuf = new librorc::buffer(dev, EBUFSIZE, 2*opts.channelId, 1, LIBRORC_DMA_TO_DEVICE); }
    catch(...)
    {
        perror("ERROR: ebuf->allocate");
        abort();
    }
    printf("EventBuffer size: 0x%lx bytes\n", EBUFSIZE);

    /** create new DMA report buffer */
    try
    { rbuf = new librorc::buffer(dev, RBUFSIZE, 2*opts.channelId+1, 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: rbuf->allocate");
        abort();
    }
    printf("ReportBuffer size: 0x%lx bytes\n", RBUFSIZE);



    /** Create DMA channel */
    try
    {
        ch = new librorc::dma_channel(opts.channelId, 128, dev, bar1, ebuf, rbuf);
        ch->enable();
    }
    catch(...)
    {
        cout << "DMA channel failed!" << endl;
        abort();
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

    last_bytes_received = 0;
    last_events_received = 0;

    int32_t sanity_checks = CHK_SIZES|CHK_SOE;
    if(opts.useRefFile)
    {
        sanity_checks |= CHK_FILE;
    }
    else
    {
        sanity_checks |= CHK_PATTERN | CHK_ID;
    }


    event_generator eventGen(rbuf, ebuf, ch);
    // wait for RB entry
    while(!done)
    {
        nevents = eventGen.fillEventBuffer(opts.eventSize);

        if( nevents > 0 )
        { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", nevents); }

        result = handle_channel_data(
                rbuf,
                ebuf,
                ch, // channe struct
                chstats, // stats struct
                sanity_checks, // do sanity check
                NULL, // no DDL reference file
                0); //DDL reference size

        if (result<0)
        {
            printf("handle_channel_data failed for channel %d\n",
                    opts.channelId);
        }
        else if(result==0)
        { usleep(100); }

        bar1->gettime(&cur_time, 0);

        // print status line each second
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL) {
            printf("Events OUT: %10ld, Size: %8.3f GB",
                    chstats->n_events,
                    (double)chstats->bytes_received/(double)(1<<30));

            if ( chstats->bytes_received-last_bytes_received)
            {
                printf(" Rate: %9.3f MB/s",
                        (double)(chstats->bytes_received-last_bytes_received)/
                        gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
            } else {
                printf(" Rate: -");
            }

            if ( chstats->n_events - last_events_received)
            {
                printf(" (%.3f kHz)",
                        (double)(chstats->n_events-last_events_received)/
                        gettimeofday_diff(last_time, cur_time)/1000.0);
            } else {
                printf(" ( - )");
            }
            printf(" Errors: %ld\n", chstats->error_count);
            last_time = cur_time;
            last_bytes_received = chstats->bytes_received;
            last_events_received = chstats->n_events;
        }

    }

    // EOR
    bar1->gettime(&end_time, 0);

    // print summary
    printf("%ld Byte / %ld events in %.2f sec"
            "-> %.1f MB/s.\n",
            (chstats->bytes_received), chstats->n_events,
            gettimeofday_diff(start_time, end_time),
            ((float)chstats->bytes_received/
             gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

    if(!chstats->set_offset_count) //avoid DivByZero Exception
        printf("CH%d: No Events\n", opts.channelId);
    else
        printf("CH%d: Events %ld, max_epi=%ld, min_epi=%ld, "
                "avg_epi=%ld, set_offset_count=%ld\n", opts.channelId,
                chstats->n_events, chstats->max_epi,
                chstats->min_epi,
                chstats->n_events/chstats->set_offset_count,
                chstats->set_offset_count);

    // wait until EL_FIFO runs empty
    // TODO: add timeout
    while( ch->getLink()->packetizer(RORC_REG_DMA_ELFIFO) & 0xffff )
        usleep(100);

    // wait for pending transfers to complete (dma_busy->0)
    // TODO: add timeout
    while( ch->getDMABusy() )
        usleep(100);

    // disable EBDM Engine
    ch->disableEventBuffer();

    // disable RBDM
    ch->disableReportBuffer();

    // reset DFIFO, disable DMA PKT
    ch->setDMAConfig(0X00000002);

    // clear reportbuffer
    memset(rbuf->getMem(), 0, rbuf->getMappingSize());


    shmdt(chstats);

    if (ch)
        delete ch;
    if (ebuf)
        delete ebuf;
    if (rbuf)
        delete rbuf;

    if (bar1)
        delete bar1;
    if (dev)
        delete dev;

    return result;
}
