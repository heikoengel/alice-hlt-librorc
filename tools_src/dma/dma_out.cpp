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

#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

    int32_t sanity_check_mask = CHK_SIZES|CHK_CMPL;

    librorc::event_sanity_checker checker =
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

    uint32_t eventSize;
    uint32_t *eventBuffer = hlEventStream->m_eventBuffer->getMem();

    if( opts.useRefFile )
    {
        // load reffile into EB
        int fd = open(opts.refname, O_RDONLY);
        if( fd == -1 )
        {
            cout << "ERROR: Failed to open input file " << opts.refname
                 << endl;
            abort();
        }
        struct stat fd_stat;
        fstat(fd, &fd_stat);

        if( (uint64_t)fd_stat.st_size > hlEventStream->m_eventBuffer->getPhysicalSize() )
        {
            cout << "ERROR: Input file does not fit into EventBuffer" << endl;
            abort();
        }

        uint32_t *event = (uint32_t *)mmap(NULL, fd_stat.st_size,
                PROT_READ, MAP_SHARED, fd, 0);
        if ( event == MAP_FAILED )
        {
            cout << "ERROR: failed to mmap input file" << endl;
            abort();
        }

        memcpy(eventBuffer, event, fd_stat.st_size);
        eventSize = fd_stat.st_size;

        munmap(event, fd_stat.st_size);
        close(fd);
    }
    else
    {
        // fill EB with pattern data
        const uint64_t eventBufferSize = hlEventStream->m_eventBuffer->getPhysicalSize();//0x404;
        const uint64_t event_id = 0;
        eventBuffer[0] = 0xffffffff;
        eventBuffer[1] = event_id & 0xfff;
        eventBuffer[2] = ((event_id>>12) & 0x00ffffff);
        eventBuffer[3] = 0x00000000; // PGMode / participating subdetectors
        eventBuffer[4] = 0x00000000; // mini event id, error flags, MBZ
        eventBuffer[5] = 0xaffeaffe; // trigger classes low
        eventBuffer[6] = 0x00000000; // trigger classes high, MBZ, ROI
        eventBuffer[7] = 0xdeadbeaf; // ROI high
        for( uint64_t i=8; i<(eventBufferSize/4); i++ )
        { eventBuffer[i] = i-8; }

        // set initial event size
        eventSize = opts.eventSize;
    }

    /** capture starting time */
    timeval start_time;
    hlEventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time  = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;
    uint64_t number_of_samples = 0;
    bool waitForDone = false;
    uint64_t pending = 0;
    librorc::EventDescriptor *report               = NULL;
    const uint32_t           *event                = 0;
    uint64_t                  reference            = 0;

    /** wait for RB entry */
    while(!hlEventStream->m_done)
    {
        if( opts.datasource==ES_SRC_DMA)
        {
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
                    uint32_t timeout_flag = (report->reported_event_size>>30)&1;
                    uint32_t cmpl_flag = (report->calc_event_size>>30);
                    if( timeout_flag || cmpl_flag )
                    { printf("ERROR: T:%d, S:%d\n", timeout_flag, cmpl_flag ); }

                    checker.check(*report, hlEventStream->m_channel_status, hlEventStream->m_channel_status->n_events);

                    waitForDone=false;
                    hlEventStream->updateChannelStatus(report);
                    hlEventStream->releaseEvent(reference);
                    pending--;
                }
                //else
                //{ usleep(100); }
            }

            hlEventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(librorc::gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                /*printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                        opts.channelId, hlEventStream->m_channel_status->n_events,
                        (double)hlEventStream->m_channel_status->bytes_received/(double)(1<<30));*/
                double throughput, rate;
                uint64_t bytes_received = hlEventStream->m_channel_status->bytes_received - last_bytes_received;
                uint64_t events_received = hlEventStream->m_channel_status->n_events - last_events_received;

                if(bytes_received)
                { throughput = (double)(bytes_received)/librorc::gettimeofdayDiff(last_time, cur_time); }
                else
                { throughput = 0; }

                if(events_received)
                { rate = (double)(events_received)/librorc::gettimeofdayDiff(last_time, cur_time); }
                else
                { rate = 0; }

                // create csv output
                // printf("%d, %f, %f\n", eventSize, throughput, rate);

                // create nice output
                printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                        opts.channelId, hlEventStream->m_channel_status->n_events,
                        hlEventStream->m_channel_status->bytes_received/(double)(1<<30));
                printf(" Rate: %9.3f MB/s", throughput/(double)(1<<20));
                printf(" (%.3f kHz)", rate/1000.0);
                printf(" Errors: %ld\n", hlEventStream->m_channel_status->error_count);

                last_time = cur_time;
                last_bytes_received  = hlEventStream->m_channel_status->bytes_received;
                last_events_received = hlEventStream->m_channel_status->n_events;
                number_of_samples++;
                number_of_samples &= 0x3;

                /*if(number_of_samples==0x3)
                {
                    eventSize = nextEventSize(eventSize);
                    if( eventSize >= hlEventStream->m_eventBuffer->getPhysicalSize() )
                    { eventSize = opts.eventSize; }
                }*/

            }
        }
        else
        {
            /** Data is generated from PG */
            hlEventStream->m_bar1->gettime(&cur_time, 0);

            // print status line each second
            if(librorc::gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
            {
                hlEventStream->m_channel_status->n_events = hlEventStream->m_link->ddlReg(RORC_REG_DDL_EC);
                printf("CH%d: Events OUT via PG: %10ld",
                        opts.channelId, hlEventStream->m_channel_status->n_events);

                if(hlEventStream->m_channel_status->n_events - last_events_received)
                {
                    printf(" EventRate: %.3f kHz",
                            (double)(hlEventStream->m_channel_status->n_events-last_events_received)/
                            librorc::gettimeofdayDiff(last_time, cur_time)/1000.0);
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
