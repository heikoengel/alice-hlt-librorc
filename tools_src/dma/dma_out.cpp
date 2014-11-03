/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/


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

    hlEventStream = prepareEventStream(opts);
    if( !hlEventStream )
    { exit(-1); }

    configureDataSource(hlEventStream, opts);

    hlEventStream->printDeviceStatus();

    //hlEventStream->setEventCallback(eventCallBack);
    hlEventStream->setEventCallback(NULL);

    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE;
    if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else
    { sanity_check_mask |= CHK_PATTERN | CHK_ID; }


    //librorc::event_generator eventGen(hlEventStream);

    librorc::event_sanity_checker checker =
        (opts.useRefFile)
        ?
            librorc::event_sanity_checker
            (
                hlEventStream->m_eventBuffer,
                opts.channelId,
                sanity_check_mask,
                logdirectory,
                opts.refname
            )
        :
            librorc::event_sanity_checker
            (
                hlEventStream->m_eventBuffer,
                opts.channelId,
                sanity_check_mask,
                logdirectory
            )
        ;

//    uint64_t number_of_events = eventGen.fillEventBuffer(opts.eventSize);
//    uint64_t result = hlEventStream->eventLoop((void*)&checker);

    // fill EB
    const uint32_t EVENT_SIZE = hlEventStream->m_eventBuffer->size();//0x404;
    const uint64_t event_id = 0;
    uint32_t *eb = hlEventStream->m_eventBuffer->getMem();
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
    hlEventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;
    uint64_t number_of_samples = 0;
    bool waitForDone = false;
    uint32_t eventSize = 0x1000;
    uint64_t pending = 0;
    librorc::EventDescriptor *report               = NULL;
    const uint32_t           *event                = 0;
    uint64_t                  reference            = 0;

    /** wait for RB entry */
    while(!hlEventStream->m_done)
    {
        if( opts.datasource==ES_SRC_DMA)
        {
            /** Data is fed via DMA */
            /*number_of_events = eventGen.fillEventBuffer(opts.eventSize);

            if( number_of_events > 0 )
            { DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Pushed %ld events into EB\n", number_of_events); }
            */

            if( !waitForDone )
            //if( pending < 256 )
            {
                std::vector<librorc::ScatterGatherEntry> list;
                if( !hlEventStream->m_eventBuffer->composeSglistFromBufferSegment(0, eventSize, &list) )
                { cout << "Failed to get event sglist" << endl; }
                hlEventStream->m_channel->announceEvent(list);
                waitForDone = true;
                pending++;
            }
            else
            {
                if( hlEventStream->getNextEvent(&report, &event, &reference) )
                {
                    if( (report->reported_event_size>>30)&1 || (report->calc_event_size>>30) )
                    {
                        printf("ERROR: T:%d, S:%d\n", (report->reported_event_size>>30)&1, (report->calc_event_size>>30) );
                    }
                    waitForDone=false;
                    hlEventStream->m_channel_status->bytes_received += (report->calc_event_size << 2);
                    hlEventStream->m_channel_status->n_events++;
                    hlEventStream->releaseEvent(reference);
                    pending--;
                }
                //else
                //{ usleep(100); }
            }

            hlEventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                /*printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                        opts.channelId, hlEventStream->m_channel_status->n_events,
                        (double)hlEventStream->m_channel_status->bytes_received/(double)(1<<30));*/
                double throughput, rate;

                if(hlEventStream->m_channel_status->bytes_received-last_bytes_received)
                {
                    throughput = (double)(hlEventStream->m_channel_status->bytes_received-last_bytes_received)/
                            gettimeofdayDiff(last_time, cur_time);
                    //printf(" Rate: %9.3f MB/s", rate);
                }
                else
                {
                    throughput = 0;
                    //printf(" Rate: -");
                }

                if(hlEventStream->m_channel_status->n_events - last_events_received)
                {
                    rate = (double)(hlEventStream->m_channel_status->n_events-last_events_received)/
                            gettimeofdayDiff(last_time, cur_time);
                    /*printf(" (%.3f kHz)",
                            (double)(hlEventStream->m_channel_status->n_events-last_events_received)/
                            gettimeofdayDiff(last_time, cur_time)/1000.0);*/
                }
                else
                {
                    //printf(" ( - )");
                    rate = 0;
                }

                //printf(" Errors: %ld\n", hlEventStream->m_channel_status->error_count);
                last_time = cur_time;
                last_bytes_received  = hlEventStream->m_channel_status->bytes_received;
                last_events_received = hlEventStream->m_channel_status->n_events;
                number_of_samples++;
                number_of_samples &= 0x3;

                printf("%d, %f, %f\n", eventSize, throughput, rate);

                /*if(number_of_samples==0x3)
                {
                    eventSize = nextEventSize(eventSize);
                    if( eventSize >= hlEventStream->m_eventBuffer->size() )
                    { eventSize = 0x100; }
                }*/

            }
        }
        else
        {
            /** Data is generated from PG */
            hlEventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                hlEventStream->m_channel_status->n_events = hlEventStream->m_link->ddlReg(RORC_REG_DDL_EC);
                printf("CH%d: Events OUT via PG: %10ld",
                        opts.channelId, hlEventStream->m_channel_status->n_events);

                if(hlEventStream->m_channel_status->n_events - last_events_received)
                {
                    printf(" EventRate: %.3f kHz",
                            (double)(hlEventStream->m_channel_status->n_events-last_events_received)/
                            gettimeofdayDiff(last_time, cur_time)/1000.0);
                }
                else
                { printf(" ( - )"); }

                printf("\n");
                last_time = cur_time;
                last_events_received = hlEventStream->m_channel_status->n_events;
            }
        }
    }

    // EOR
    timeval end_time;
    hlEventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(hlEventStream->m_channel_status, opts, start_time, end_time);

    // disable SIU
    // TODO: this should happen after stopping the data source,
    // which is currently in hlEventStream::~high_level_event_stream()
    unconfigureDataSource(hlEventStream, opts);

    // clear reportbuffer
    //memset(hlEventStream->m_reportBuffer->getMem(), 0, hlEventStream->m_reportBuffer->getMappingSize());

    delete hlEventStream;

    return 1;
}
