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



uint32_t
nextEventSize
(
    uint32_t EventSize
)
{
    for ( uint32_t i=0; i<32; i++ )
    {
        uint32_t es = (EventSize>>i);
        switch (es)
        {
            case 2:
            case 3:
                return EventSize + (1<<(i-1));
                return EventSize + (1<<(i-1));
            case 5:
                return EventSize + (1<<i);
            case 7:
                return ( 1<<(i+3) );
            default:
                break;
        }
    }
    //TODO
    return 0;
}

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


    //librorc::event_generator eventGen(eventStream);

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
    const uint32_t EVENT_SIZE = eventStream->m_eventBuffer->size();//0x404;
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

    /** capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;
    uint64_t number_of_samples = 0;
    bool waitForDone = false;
    uint32_t eventSize = 0x100;
    uint64_t pending = 0;
    librorc_event_descriptor *report               = NULL;
    const uint32_t           *event                = 0;
    uint64_t                  reference            = 0;

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

            //if( !waitForDone )
            if( pending < 256 )
            {
                std::vector<librorc_sg_entry> list;
                if( !eventStream->m_eventBuffer->composeSglistFromBufferSegment(0, eventSize, &list) )
                { cout << "Failed to get event sglist" << endl; }
                eventStream->m_channel->announceEvent(list);
                waitForDone = true;
                pending++;
            }
            //else
            //{
                if( eventStream->getNextEvent(&report, &event, &reference) )
                {
                    if( (report->reported_event_size>>30)&1 || (report->calc_event_size>>30) )
                    {
                        printf("ERROR: T:%d, S:%d\n", (report->reported_event_size>>30)&1, (report->calc_event_size>>30) );
                    }
                    waitForDone=false;
                    eventStream->m_channel_status->bytes_received += (report->calc_event_size << 2);
                    eventStream->m_channel_status->n_events++;
                    eventStream->releaseEvent(reference);
                    pending--;
                }
                //else
                //{ usleep(100); }
            //}

            eventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                /*printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                        opts.channelId, eventStream->m_channel_status->n_events,
                        (double)eventStream->m_channel_status->bytes_received/(double)(1<<30));*/
                double throughput, rate;

                if(eventStream->m_channel_status->bytes_received-last_bytes_received)
                {
                    throughput = (double)(eventStream->m_channel_status->bytes_received-last_bytes_received)/
                            gettimeofdayDiff(last_time, cur_time);
                    //printf(" Rate: %9.3f MB/s", rate);
                }
                else
                {
                    throughput = 0;
                    //printf(" Rate: -");
                }

                if(eventStream->m_channel_status->n_events - last_events_received)
                {
                    rate = (double)(eventStream->m_channel_status->n_events-last_events_received)/
                            gettimeofdayDiff(last_time, cur_time);
                    /*printf(" (%.3f kHz)",
                            (double)(eventStream->m_channel_status->n_events-last_events_received)/
                            gettimeofdayDiff(last_time, cur_time)/1000.0);*/
                }
                else
                {
                    //printf(" ( - )");
                    rate = 0;
                }

                //printf(" Errors: %ld\n", eventStream->m_channel_status->error_count);
                last_time = cur_time;
                last_bytes_received  = eventStream->m_channel_status->bytes_received;
                last_events_received = eventStream->m_channel_status->n_events;
                number_of_samples++;
                number_of_samples &= 0x3;

                printf("%d, %f, %f\n", eventSize, throughput, rate);

                if(number_of_samples==0x3)
                {
                    eventSize = nextEventSize(eventSize);
                    if( eventSize >= eventStream->m_eventBuffer->size() )
                    { eventSize = 0x100; }
                }

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
