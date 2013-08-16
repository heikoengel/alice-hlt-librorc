/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2012-11-14
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
 * @brief
 * Open DMA Channel sourced by PatternGenerator
 *
 **/

#include <librorc.h>
#include <event_handling.h>

#include "dma_handling.hh"

using namespace std;

DMA_ABORT_HANDLER


int main(int argc, char *argv[])
{

    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0]) &&
        checkEventSize(opts.eventSize, argv[0])
    ) )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    channelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    //ready

    /** Create new device instance */
    librorc::device *dev;
    try{ dev = new librorc::device(opts.deviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }

    printf("Bus %x, Slot %x, Func %x\n", dev->getBus(),
           dev->getSlot(),dev->getFunc());

    /** bind to BAR1 */
    librorc::bar *bar1 = NULL;
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

    cout << "FirmwareDate: " << setw(8) << hex
         << bar1->get32(RORC_REG_FIRMWARE_DATE);
    cout << "FirmwareRevision: " << setw(8) << hex
         << bar1->get32(RORC_REG_FIRMWARE_REVISION);

    /** Check if requested channel is implemented in firmware */
    if( opts.channelId >= (int32_t)(bar1->get32(RORC_REG_TYPE_CHANNELS) & 0xffff) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
               "firmware - exiting\n", opts.channelId);
        abort();
    }


    /** Create new DMA event buffer */
    librorc::buffer *ebuf;
    try
    { ebuf = new librorc::buffer(dev, EBUFSIZE, (2*opts.channelId), 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: ebuf->allocate");
        abort();
    }

    /** Create new DMA report buffer */
    librorc::buffer *rbuf;
    try
    { rbuf = new librorc::buffer(dev, RBUFSIZE, 2*opts.channelId+1, 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: rbuf->allocate");
        abort();
    }

    /** clear report buffer */
    struct rorcfs_event_descriptor *reportbuffer
        = (struct rorcfs_event_descriptor *)rbuf->getMem();
    memset(reportbuffer, 0, rbuf->getMappingSize());


    /** Create DMA channel */
    librorc::dma_channel *ch;
    try
    { ch = new librorc::dma_channel(opts.channelId, bar1, ebuf, rbuf); }
    catch(...)
    {
        cout << "DMA channel failed!" << endl;
        abort();
    }


//    /** Prepare EventBufferDescriptorManager with scatter-gather list */
//    if(ch->prepareEB(ebuf) < 0)
//    {
//        perror("prepareEB()");
//        abort();
//    }
//
//    /** Prepare ReportBufferDescriptorManager with scatter-gather list */
//    if(ch->prepareRB(rbuf) < 0)
//    {
//        perror("prepareRB()");
//        abort();
//    }

    /** Aet MAX_PAYLOAD, buffer sizes, #sgEntries, ... */
    if(ch->configureChannel(ebuf, rbuf, MAX_PAYLOAD) < 0)
    {
        perror("configureChannel()");
        abort();
    }

    /** Enable Buffer Description Managers (BDMs) */
    ch->setEnableEB(1);
    ch->setEnableRB(1);

    /** Enable DMA channel */
    ch->setDMAConfig( ch->getDMAConfig() | 0x01 );


    /**
     * wait for GTX domain to be ready
     * read asynchronous GTX status
     * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
     * & !gtx_in_rst
     **/
    printf("Waiting for GTX to be ready...\n");
    while( (ch->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
        { usleep(100); }

//PG SPECIFIC
    /** Configure Pattern Generator */
    ch->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, opts.eventSize);
    ch->setGTX(RORC_REG_DDL_CTRL, ch->getGTX(RORC_REG_DDL_CTRL) | 0x600);
    ch->setGTX(RORC_REG_DDL_CTRL, ch->getGTX(RORC_REG_DDL_CTRL) | 0x100);
//PG SPECIFIC

    /** capture starting time */
    timeval start_time;
    bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;

    int result = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    while( !done )
    {
    /** ... triggered from CTRL+C                  */

        result = handle_channel_data
        (
            rbuf,
            ebuf,
            ch,      /** channel struct     */
            chstats, /** stats struct       */
            0xff,    /** do sanity check    */
            NULL,    /** no reference DDL   */
            0        /** reference DDL size */
        );

        if( result < 0 )
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            abort();
        }
        else if( result==0 )
        {
            /** no events available */
            usleep(100);
        }

        bar1->gettime(&cur_time, 0);

        /** print status line each second */
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL)
        {
            printf
            (
                "Events: %10ld, DataSize: %8.3f GB",
                chstats->n_events,
                (double)chstats->bytes_received/(double)(1<<30)
            );

            if( chstats->bytes_received-last_bytes_received )
            {
                printf
                (
                    " DataRate: %9.3f MB/s",
                    (double)(chstats->bytes_received-last_bytes_received)/
                    gettimeofday_diff(last_time, cur_time)/(double)(1<<20)
                );
            }
            else
            {
                printf(" DataRate: -");
            }

            if( chstats->n_events - last_events_received)
            {
                printf
                (
                    " EventRate: %9.3f kHz/s",
                    (double)(chstats->n_events-last_events_received)/
                    gettimeofday_diff(last_time, cur_time)/1000.0
                );
            }
            else
            {
                printf(" EventRate: -");
            }

            printf(" Errors: %ld\n", chstats->error_count);
            last_time = cur_time;
            last_bytes_received = chstats->bytes_received;
            last_events_received = chstats->n_events;
        }
    }

    timeval end_time;
    bar1->gettime(&end_time, 0);
    printf
    (
        "%ld Byte / %ld events in %.2f sec -> %.1f MB/s.\n",
        chstats->bytes_received,
        chstats->n_events,
        gettimeofday_diff(start_time, end_time),
        ((float)chstats->bytes_received/gettimeofday_diff(start_time, end_time))/(float)(1<<20)
    );

    if(!chstats->set_offset_count)
    {
        printf("CH%d: No Events\n", opts.channelId);
    }
    else
    {
        printf
        (
            "CH%d: Events %ld, max_epi=%ld, min_epi=%ld, avg_epi=%ld, set_offset_count=%ld\n",
            opts.channelId,
            chstats->n_events,
            chstats->max_epi,
            chstats->min_epi,
            chstats->n_events/chstats->set_offset_count,
            chstats->set_offset_count
        );
    }

    /** Disable PG */
    ch->setGTX(RORC_REG_DDL_CTRL, 0x0);

    /** Disable DMA Engine */
    ch->setEnableEB(0);

    /** Wait for pending transfers to complete (dma_busy->0) */
    while( ch->getDMABusy() )
    {
        usleep(100);
    }

    /** Disable RBDM */
    ch->setEnableRB(0);

    /** Reset DFIFO, disable DMA PKT */
    ch->setDMAConfig(0X00000002);

    /** Clear reportbuffer */
    memset(reportbuffer, 0, rbuf->getMappingSize());

    /** cleanup */
    if(chstats)
    {
        shmdt(chstats);
        chstats = NULL;
    }

//    if(ddlref)
//    {
//        if( munmap(ddlref, ddlref_size)==-1 )
//        { perror("ERROR: failed to unmap file"); }
//    }
//
//    if(ddlref_fd>=0)
//    { close(ddlref_fd); }

    if(ch)
    { delete ch; }

    if(ebuf)
    { delete ebuf; }

    if(rbuf)
    { delete rbuf; }

    if(bar1)
    { delete bar1; }

    if(dev)
    { delete dev; }

    return result;
}
