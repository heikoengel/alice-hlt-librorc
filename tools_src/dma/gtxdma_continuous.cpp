/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2012-12-17
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
 * Open DMA Channel sourced by optical DDL
 *
 * */

#include <librorc.h>
#include <event_handling.h>

#include "dma_handling.hh"

using namespace std;

DMA_ABORT_HANDLER


int main( int argc, char *argv[])
{
    int result = 0;

    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    channelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    DDLRefFile ddlref = getDDLReferenceFile(opts);

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
                eventStream->m_eventBuffer,
                eventStream->m_reportBuffer
            );
        ch->enable();
    }
    catch(...)
    {
        cout << "DMA channel failed!" << endl;
        return(-1);
    }

//ready

    /**
     * wait for GTX domain to be ready
     * read asynchronous GTX status
     * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
     * & !gtx_in_rst
     */
    printf("Waiting for GTX to be ready...\n");
    while( (ch->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
        { usleep(100); }

//GTX SPECIFIC
    /** set ENABLE, activate flow control (DIU_IF:busy) */
    ch->setGTX(RORC_REG_DDL_CTRL, 0x00000003);

    /** wait for riLD_N='1' */
    printf("Waiting for LD_N to deassert...\n");
    while( (ch->getGTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20 )
        {usleep(100);}

    /** clear DIU_IF IFSTW, CTSTW */
    ch->setGTX(RORC_REG_DDL_IFSTW, 0);
    ch->setGTX(RORC_REG_DDL_CTSTW, 0);

    /** send EOBTR to close any open transaction */
    ch->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

    /** wait for command transmission status word (CTSTW) from DIU */
    uint32_t ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    while( ctstw == 0xffffffff )
    {
        usleep(100);
        ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    }

    uint8_t ddl_trn_id = 2 & 0x0f;

    printf("DIU CTSTW: %08x\n", ctstw);
    printf("DIU IFSTW: %08x\n", ch->getGTX(RORC_REG_DDL_IFSTW));

    /** clear DIU_IF IFSTW */
    ch->setGTX(RORC_REG_DDL_IFSTW, 0);
    ch->setGTX(RORC_REG_DDL_CTSTW, 0);

    /** send RdyRx to SIU */
    ch->setGTX(RORC_REG_DDL_CMD, 0x00000014);

    /** wait for command transmission status word (CTSTW) from DIU */
    ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    while( ctstw == 0xffffffff )
    {
        usleep(100);
        ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    }
    ddl_trn_id = (ddl_trn_id+2) & 0x0f;

    /** clear DIU_IF IFSTW */
    ch->setGTX(RORC_REG_DDL_IFSTW, 0);
    ch->setGTX(RORC_REG_DDL_CTSTW, 0);
//GTX SPECIFIC


    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received = 0;
    uint64_t last_events_received = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    int32_t sanity_checks;
    while( !done )
    {

        /** this can be aborted by abort_handler(),
         * triggered from CTRL+C
         */

        if(ddlref.map && ddlref.size)
            {sanity_checks = CHK_FILE;}
        else
            {sanity_checks = CHK_SIZES;}

        result = handle_channel_data
        (
            eventStream->m_reportBuffer,
            eventStream->m_eventBuffer,
            ch, // channel struct
            chstats, // stats struct
            sanity_checks, // do sanity check
            ddlref.map,
            ddlref.size
        );

        if(result < 0)
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            abort();
        }
        else if(result==0)
        {
            /** no events available */
            usleep(100);
        }

        eventStream->m_bar1->gettime(&cur_time, 0);

        /** print status line each second */
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL)
        {
            printf("Events: %10ld, DataSize: %8.3f GB ",
                    chstats->n_events,
                    (double)chstats->bytes_received/(double)(1<<30));

            if(chstats->bytes_received-last_bytes_received)
            {
                printf(" DataRate: %9.3f MB/s",
                        (double)(chstats->bytes_received-last_bytes_received)/
                        gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
            }
            else
            {
                printf(" DataRate: -");
                /** dump_dma_state(ch); */
                /** dump_diu_state(ch); */
            }

            if(chstats->n_events - last_events_received)
            {
                printf(" EventRate: %9.3f kHz/s",
                        (double)(chstats->n_events-last_events_received)/
                        gettimeofday_diff(last_time, cur_time)/1000.0);
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

    /** EOR */
    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    /** print summary */
    printf("%ld Byte / %ld events in %.2f sec"
            "-> %.1f MB/s.\n",
            (chstats->bytes_received), chstats->n_events,
            gettimeofday_diff(start_time, end_time),
            ((float)chstats->bytes_received/
             gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

    if(!chstats->set_offset_count) //avoid DivByZero Exception
    {
        printf("CH%d: No Events\n", opts.channelId);
    }
    else
        printf("CH%d: Events %ld, max_epi=%ld, min_epi=%ld, "
                "avg_epi=%ld, set_offset_count=%ld\n", opts.channelId,
                chstats->n_events, chstats->max_epi,
                chstats->min_epi,
                chstats->n_events/chstats->set_offset_count,
                chstats->set_offset_count);

    /** check if link is still up: LD_N == 1 */
    if ( ch->getGTX(RORC_REG_DDL_CTRL) & (1<<5) )
    {
        /** disable BUSY -> drop current data in chain */
        ch->setGTX(RORC_REG_DDL_CTRL, 0x00000001);

        /** wait for LF_N to go high */
        while(!(ch->getGTX(RORC_REG_DDL_CTRL) & (1<<4)))
            {usleep(100);}

        /** clear DIU_IF IFSTW */
        ch->setGTX(RORC_REG_DDL_IFSTW, 0);
        ch->setGTX(RORC_REG_DDL_CTSTW, 0);

        /** Send EOBTR command */
        ch->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

        /** wait for command transmission status word (CTST)
         * in response to the EOBTR:
         * STS[7:4]="0000"
         */
        while(ch->getGTX(RORC_REG_DDL_CTSTW) & 0xf0)
            {usleep(100);}

        /** disable DIU_IF */
        ch->setGTX(RORC_REG_DDL_CTRL, 0x00000000);
    }
    else
    { /**link is down -> unable to send EOBTR */
        printf("Link is down - unable to send EOBTR\n");
    }

    /** disable EBDM -> no further sg-entries to PKT */
    ch->setEnableEB(0);

    /** wait for pending transfers to complete (dma_busy->0) */
    while( ch->getDMABusy() )
        {usleep(100);}

    /** disable RBDM */
    ch->setEnableRB(0);

    /** reset DFIFO, disable DMA PKT */
    ch->setDMAConfig(0X00000002);

    /** cleanup */
    deleteDDLReferenceFile(ddlref);
    if(chstats)
    {
        shmdt(chstats);
        chstats = NULL;
    }

    if(ch)
    { delete ch; }

    return result;
}
