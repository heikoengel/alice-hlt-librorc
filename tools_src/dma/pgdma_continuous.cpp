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

    librorc::event_stream *eventStream = NULL;
    try
    { eventStream = new librorc::event_stream(opts.deviceId, opts.channelId); }
    catch( int error )
    {
        switch(error)
        {
            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED:
            { cout << "ERROR: failed to initialize device." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BAR_FAILED:
            { cout << "ERROR: failed to initialize BAR1." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BUFFER_FAILED:
            { cout << "ERROR: failed to allocate buffer." << endl; }
            break;
        }
        exit(-1);
    }

    /** Print some stats */
    printf("Bus %x, Slot %x, Func %x\n",
            eventStream->m_dev->getBus(),
            eventStream->m_dev->getSlot(),
            eventStream->m_dev->getFunc());

    try
    {
        librorc::sysmon *sm = new librorc::sysmon(eventStream->m_bar1);
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setw(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setw(8) << sm->FwBuildDate() << dec << endl;
        delete sm;
    }
    catch(...)
    { cout << "Firmware Rev. and Date not available!" << endl; }

    /** Create DMA channel */
    librorc::dma_channel *ch;
    try
    {
        ch =
            new librorc::dma_channel
            (
                opts.channelId,
                MAX_PAYLOAD,
                eventStream->m_dev,
                eventStream->m_bar1,
                eventStream->m_eventBuffer,
                eventStream->m_reportBuffer
            );
        ch->enable();
    }
    catch( int error )
    {
        cout << "DMA channel failed (ERROR :" << error << ")" << endl;
        return(-1);
    }

    cout << "Waiting for GTX to be ready..." << endl;
    ch->waitForGTXDomain();


//ready

//PG SPECIFIC
    /** Configure Pattern Generator */
    ch->setGTX(RORC_REG_DDL_PG_EVENT_LENGTH, opts.eventSize);
    ch->setGTX(RORC_REG_DDL_CTRL, ch->getGTX(RORC_REG_DDL_CTRL) | 0x600);
    ch->setGTX(RORC_REG_DDL_CTRL, ch->getGTX(RORC_REG_DDL_CTRL) | 0x100);
//PG SPECIFIC

    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
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
            eventStream->m_reportBuffer,
            eventStream->m_eventBuffer,
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

        eventStream->m_bar1->gettime(&cur_time, 0);

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
    eventStream->m_bar1->gettime(&end_time, 0);
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

    /** cleanup */
    if(chstats)
    {
        shmdt(chstats);
        chstats = NULL;
    }

    if(ch)
    { delete ch; }

    return result;
}
