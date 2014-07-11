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

    configureDataSource(eventStream, opts);

    eventStream->printDeviceStatus();

    //eventStream->setEventCallback(eventCallBack);
    eventStream->setEventCallback(NULL);

    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE;
    if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else
    { sanity_check_mask |= CHK_PATTERN | CHK_ID; }


    librorc::event_generator eventGen(eventStream);

    librorc::event_sanity_checker checker =
        (opts.useRefFile)
        ?
            librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                sanity_check_mask,
                logdirectory,
                opts.refname
            )
        :
            librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                sanity_check_mask,
                logdirectory
            )
        ;

//    uint64_t number_of_events = eventGen.fillEventBuffer(opts.eventSize);
//    uint64_t result = eventStream->eventLoop((void*)&checker);

    // fill EB
    const uint32_t EVENT_SIZE = 0x404;
    const uint64_t event_id = 0;
    uint32_t *eb = eventStream->m_eventBuffer->getMem();
    eb[0] = 0xffffffff;
    eb[1] = event_id & 0xfff;
    eb[2] = ((event_id>>12) & 0x00ffffff);
    eb[3] = 0x00000000; // PGMode / participating subdetectors
    eb[4] = 0x00000000; // mini event id, error flags, MBZ
    eb[5] = 0xaffeaffe; // trigger classes low
    eb[6] = 0x00000000; // trigger classes high, MBZ, ROI
    eb[7] = 0xdeadbeaf; // ROI high
    for( uint32_t i=8; i<(EVENT_SIZE/4); i++ )
    { eb[i] = i-8; }

    typedef struct
        __attribute__((__packed__))
        {
            uint32_t sg_addr_low;  /** lower part of sg address **/
            uint32_t sg_addr_high; /** higher part of sg address **/
            uint32_t sg_len;       /** total length of sg entry in bytes **/
            uint32_t ctrl;         /** BDM control register: [31]:we, [30]:sel, [29:0]BRAM addr **/
        } librorc_sg_entry_config;

    librorc_sg_entry_config sg;
    uint64_t phys_addr;
    if( !eventStream->m_eventBuffer->offsetToPhysAddr(0, &phys_addr) )
    { cout << "Failed to resolv physical address for offset" << endl; }
    sg.sg_addr_low = (phys_addr & 0xffffffff);
    sg.sg_addr_high = (phys_addr>>32);
    sg.sg_len = EVENT_SIZE/4;
    sg.ctrl = (1<<31);
    eventStream->m_bar1->memcopy(eventStream->m_link->base()+RORC_REG_SGENTRY_ADDR_LOW, &sg, sizeof(sg));

    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;
    uint64_t number_of_events     = 0;
    /** wait for RB entry */
    while(!eventStream->m_done)
    {
        if( opts.datasource==ES_SRC_DMA)
        {
            /** Data is fed via DMA */
            /*number_of_events = eventGen.fillEventBuffer(opts.eventSize);

            if( number_of_events > 0 )
            { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", number_of_events); }
            */

            if(eventStream->handleChannelData(&checker) == 0)
            { usleep(100); }

            eventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                        opts.channelId, eventStream->m_channel_status->n_events,
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
        else
        {
            /** Data is generated from PG */
            eventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                eventStream->m_channel_status->n_events = eventStream->m_link->ddlReg(RORC_REG_DDL_EC);
                printf("CH%d: Events OUT via PG: %10ld",
                        opts.channelId, eventStream->m_channel_status->n_events);

                if(eventStream->m_channel_status->n_events - last_events_received)
                {
                    printf(" EventRate: %.3f kHz",
                            (double)(eventStream->m_channel_status->n_events-last_events_received)/
                            gettimeofdayDiff(last_time, cur_time)/1000.0);
                }
                else
                { printf(" ( - )"); }

                printf("\n");
                last_time = cur_time;
                last_events_received = eventStream->m_channel_status->n_events;
            }
        }
    }

    // EOR
    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(eventStream->m_channel_status, opts, start_time, end_time);

    // disable SIU
    // TODO: this should happen after stopping the data source,
    // which is currently in eventStream::~event_stream()
    unconfigureDataSource(eventStream, opts);

    // clear reportbuffer
    //memset(eventStream->m_reportBuffer->getMem(), 0, eventStream->m_reportBuffer->getMappingSize());

    delete eventStream;

    return 1;
}
