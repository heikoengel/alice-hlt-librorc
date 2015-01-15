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

#define LIBRORC_INTERNAL

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <librorc.h>

#include "dma_handling.hh"

using namespace std;

#define MAX_CHANNELS 4

bool done = false;
void abort_handler( int s )
{
    printf("Caught signal %d\n", s);
    if( done==true )
    { exit(-1); }
    else
    { done = true; }
}



int main(int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])    &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }


    DMA_ABORT_HANDLER_REGISTER

    librorc::event_callback event_callback = eventCallBack;
    librorc::device *dev;
    librorc::bar *bar;
    librorc::sysmon *sm;

    try
    {
        dev = new librorc::device(opts.deviceId);
        bar = new librorc::bar(dev, 1);
        sm = new librorc::sysmon(bar);

    }
    catch(...)
    {
        cout << "Failed to initialize dev, bar or sm" << endl;
        abort();
    }

    bar->simSetPacketSize(128);

    // check if firmware is HLT_OUT
    if( !sm->firmwareIsHltOut() )
    {
        cout << "Firmware is not HLT_OUT - exiting." << endl;
        abort();
    }


    uint64_t last_bytes_received[MAX_CHANNELS];
    uint64_t last_events_received[MAX_CHANNELS];

    librorc::high_level_event_stream *hlEventStream[MAX_CHANNELS];
    for(int i=0; i < MAX_CHANNELS; i++) {
        opts.channelId = i;
        hlEventStream[i] = prepareEventStream(dev, bar, opts);
        if( !hlEventStream[i] )
        { exit(-1); }

        hlEventStream[i]->setEventCallback(event_callback);

        last_bytes_received[i] = 0;
        last_events_received[i] = 0;

        configureDataSource(hlEventStream[i], opts);
    }

    /** make clear what will be checked*/
    int32_t sanity_check_mask = CHK_SIZES|CHK_CMPL;
    librorc::event_sanity_checker checker[MAX_CHANNELS];
    uint32_t eventSize;

    for(int i=0; i < MAX_CHANNELS; i++)
    {
        checker[i] =librorc::event_sanity_checker
            (
             hlEventStream[i]->m_eventBuffer,
             i,
             sanity_check_mask,
             logdirectory
            );

        uint32_t *eventBuffer = hlEventStream[i]->m_eventBuffer->getMem();

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

            if( (uint64_t)fd_stat.st_size > hlEventStream[i]->m_eventBuffer->getPhysicalSize() )
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
            const uint64_t eventBufferSize = hlEventStream[i]->m_eventBuffer->getPhysicalSize();//0x404;
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

    }

    /** Create event stream */
    uint64_t result = 0;

    /** Capture starting time */
    timeval start_time;
    hlEventStream[0]->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    bool waitForDone[MAX_CHANNELS];
    for(int i=0; i < MAX_CHANNELS; i++) {
        waitForDone[i] = false;
    }

    librorc::EventDescriptor *report               = NULL;
    const uint32_t           *event                = 0;
    uint64_t                  reference            = 0;

    /** Event loop */
    while( !done )
    {

        for(int i=0; i < MAX_CHANNELS; i++) {

            if( opts.datasource==ES_SRC_DMA)
            {
                if( !waitForDone[i] )
                    //if( pending < 256 )
                {
                    std::vector<librorc::ScatterGatherEntry> list;
                    if( !hlEventStream[i]->m_eventBuffer->composeSglistFromBufferSegment(0, eventSize, &list) )
                    { cout << "Failed to get event sglist" << endl; }
                    hlEventStream[i]->m_channel->announceEvent(list);
                    waitForDone[i] = true;
                }
                else
                {
                    if( hlEventStream[i]->getNextEvent(&report, &event, &reference) )
                    {
                        uint32_t timeout_flag = (report->reported_event_size>>30)&1;
                        uint32_t cmpl_flag = (report->calc_event_size>>30);
                        if( timeout_flag || cmpl_flag )
                        { printf("ERROR: T:%d, S:%d\n", timeout_flag, cmpl_flag ); }

                        checker[i].check(*report, hlEventStream[i]->m_channel_status, hlEventStream[i]->m_channel_status->n_events);

                        waitForDone[i]=false;
                        hlEventStream[i]->updateChannelStatus(report);
                        hlEventStream[i]->m_channel_status->set_offset_count++;
                        hlEventStream[i]->releaseEvent(reference);
                    }
                    //else
                    //{ usleep(100); }
                }
            }
        }

        hlEventStream[0]->m_bar1->gettime(&cur_time, 0);

        // print status line each second
        if(librorc::gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
        {
            for(int i=0; i < MAX_CHANNELS; i++) {
                /*printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                  opts.channelId, hlEventStream->m_channel_status->n_events,
                  (double)hlEventStream->m_channel_status->bytes_received/(double)(1<<30));*/
                double throughput, rate;
                uint64_t bytes_received = hlEventStream[i]->m_channel_status->bytes_received - last_bytes_received[i];
                uint64_t events_received = hlEventStream[i]->m_channel_status->n_events - last_events_received[i];

                if(bytes_received)
                { throughput = (double)(bytes_received)/librorc::gettimeofdayDiff(last_time, cur_time); }
                else
                { throughput = 0; }

                if(events_received)
                { rate = (double)(events_received)/librorc::gettimeofdayDiff(last_time, cur_time); }
                else
                { rate = 0; }

                printf("CH%d: Events OUT: %10ld, Size: %8.3f GB",
                        i, hlEventStream[i]->m_channel_status->n_events,
                        hlEventStream[i]->m_channel_status->bytes_received/(double)(1<<30));
                printf(" Rate: %9.3f MB/s", throughput/(double)(1<<20));
                printf(" (%.3f kHz)", rate/1000.0);
                printf(" Errors: %ld\n", hlEventStream[i]->m_channel_status->error_count);

                last_bytes_received[i]  = hlEventStream[i]->m_channel_status->bytes_received;
                last_events_received[i] = hlEventStream[i]->m_channel_status->n_events;
            } // channel loop
            last_time = cur_time;
        } // gettimeofdayDiff
    } // !done

    timeval end_time;
    hlEventStream[0]->m_bar1->gettime(&end_time, 0);

    /** Cleanup */
    for(int i=0; i < MAX_CHANNELS; i++) {
        opts.channelId = i;
        printFinalStatusLine
            (
             hlEventStream[i]->m_channel_status,
             opts,
             start_time,
             end_time
            );
        unconfigureDataSource(hlEventStream[i], opts);
        delete hlEventStream[i];
    }

    delete sm;
    delete bar;
    delete dev;
    return result;
}
