/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
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


#include <librorc.h>
#include <event_generation.h>
#include "dma_handling.hh"


using namespace std;



DMA_ABORT_HANDLER



int main( int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    struct stat ddlstat;
    if( opts.useRefFile )
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

    eventStream = prepareEventStream(opts);
    if( !eventStream )
    { exit(-1); }

    eventStream->printDeviceStatus();

    eventStream->setEventCallback(eventCallBack);

    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE;
    if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else
    { sanity_check_mask |= CHK_PATTERN | CHK_ID; }


    event_generator
    eventGen
    (
        eventStream->m_reportBuffer,
        eventStream->m_eventBuffer,
        eventStream->m_channel
    );

    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    librorc::event_sanity_checker checker =
        (opts.useRefFile)
        ?
            librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC,
                sanity_check_mask,
                logdirectory,
                opts.refname
            )
        :
            librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC,
                sanity_check_mask,
                logdirectory
            )
        ;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;
    uint64_t number_of_events     = 0;
    /** wait for RB entry */
    while(!eventStream->m_done)
    {
        number_of_events = eventGen.fillEventBuffer(opts.eventSize);

        if( number_of_events > 0 )
        { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", number_of_events); }

        if(eventStream->handleChannelData(&checker) == 0)
        { usleep(100); }

        eventStream->m_bar1->gettime(&cur_time, 0);

        // print status line each second
        if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
        {
            printf("Events OUT: %10ld, Size: %8.3f GB",
                    eventStream->m_channel_status->n_events,
                    (double)eventStream->m_channel_status->bytes_received/(double)(1<<30));

            if(eventStream->m_channel_status->bytes_received-last_bytes_received)
            {
                printf(" Rate: %9.3f MB/s",
                        (double)(eventStream->m_channel_status->bytes_received-last_bytes_received)/
                        gettimeofdayDiff(last_time, cur_time)/(double)(1<<20));
            }
            else
            { printf(" Rate: -"); }

            if(eventStream->m_channel_status->n_events - last_events_received)
            {
                printf(" (%.3f kHz)",
                        (double)(eventStream->m_channel_status->n_events-last_events_received)/
                        gettimeofdayDiff(last_time, cur_time)/1000.0);
            }
            else
            { printf(" ( - )"); }

            printf(" Errors: %ld\n", eventStream->m_channel_status->error_count);
            last_time = cur_time;
            last_bytes_received  = eventStream->m_channel_status->bytes_received;
            last_events_received = eventStream->m_channel_status->n_events;
        }

    }

    // EOR
    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(eventStream->m_channel_status, opts, start_time, end_time);

    // wait until EL_FIFO runs empty
    // TODO: add timeout
    while( eventStream->m_channel->getLink()->packetizer(RORC_REG_DMA_ELFIFO) & 0xffff )
    { usleep(100); }

    // wait for pending transfers to complete (dma_busy->0)
    // TODO: add timeout
    while( eventStream->m_channel->getDMABusy() )
    { usleep(100); }

    // disable EBDM Engine
    eventStream->m_channel->disableEventBuffer();

    // disable RBDM
    eventStream->m_channel->disableReportBuffer();

    // reset DFIFO, disable DMA PKT
    eventStream->m_channel->setDMAConfig(0X00000002);

    // clear reportbuffer
    memset(eventStream->m_reportBuffer->getMem(), 0, eventStream->m_reportBuffer->getMappingSize());

    delete eventStream;

    return 1;
}
