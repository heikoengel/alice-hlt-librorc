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
 *
 * @brief
 * Open a DMA Channel and read out data
 *
 **/

#define LIBRORC_INTERNAL

#include <librorc.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>

#include "dma_handling.hh"

using namespace std;

#define MAX_CHANNELS 6

bool done = false;
void abort_handler( int s )
{
    printf("Caught signal %d\n", s);
    if( done==true )
    { exit(-1); }
    else
    { done = true; }
}



uint32_t fileToRam
(
 librorc::sysmon *sm,
 const char* filename,
 uint32_t chId,
 uint32_t addr,
 bool last_event
)
{
    int fd_in = open(filename, O_RDONLY);
    uint32_t next_addr = 0;
    if ( fd_in==-1 )
    {
        cout << "ERROR: Failed to open input file" << endl;
        abort();
    }
    struct stat fd_in_stat;
    fstat(fd_in, &fd_in_stat);

    uint32_t *event = (uint32_t *)mmap(NULL, fd_in_stat.st_size,
            PROT_READ, MAP_SHARED, fd_in, 0);
    if ( event == MAP_FAILED )
    {
        cout << "ERROR: failed to mmap input file" << endl;
        abort();
    }
    cout << "Ch " << chId << ": Writing " << filename << " to DDR3..." << endl;
    try {
        next_addr = sm->ddr3DataReplayEventToRam(
                event,
                (fd_in_stat.st_size>>2), // num_dws
                addr, // ddr3 start address
                chId, // channel
                last_event); // last event
    }
    catch( int e )
    {
        cout << "Exception while writing event: " << e << endl;
        abort();
    }

    munmap(event, fd_in_stat.st_size);
    close(fd_in);

    return next_addr;
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

    if( !opts.useRefFile ) {
        exit(-1);
    }


    DMA_ABORT_HANDLER_REGISTER

    librorc::event_callback event_callback = eventCallBack;
    librorc::device *dev;
    librorc::bar *bar;

    try
    {
        dev = new librorc::device(opts.deviceId);
        bar = new librorc::bar(dev, 1);
    }
    catch(...)
    {
        cout << "Failed to initialize dev and bar" << endl;
        abort();
    }

    librorc::link *link[MAX_CHANNELS];
    librorc::datareplaychannel *dr[MAX_CHANNELS];

    for(int i=0; i < MAX_CHANNELS; i++) {
        link[i] = new librorc::link(bar, i);
        dr[i] = new librorc::datareplaychannel(link[i]);
    }

    librorc::sysmon *sm = NULL;
    try
    {
        sm = new librorc::sysmon(bar);
    }
    catch(...)
    {
        cout << "ERROR: failed to initialize sysmon" << endl;
        abort();
    }

    sm->ddr3SetReset(0, 0);
    sm->ddr3SetReset(1, 0);

    cout << "Waiting for phy_init_done..." << endl;
    while ( !(bar->get32(RORC_REG_DDR3_CTRL) & (1<<1)) )
    { usleep(100); }

    uint64_t last_bytes_received[MAX_CHANNELS];
    uint64_t last_events_received[MAX_CHANNELS];

    librorc::high_level_event_stream *hlEventStream[MAX_CHANNELS];

    for(int i=0; i < MAX_CHANNELS; i++) {
        uint32_t ch_start_addr = (i % 6)*0x02000000;
        fileToRam(sm, opts.refname, i, ch_start_addr, true);
    }

    /** wait until GTX domain is up */
    cout << "Waiting for GTX domain..." << endl;
    link[0]->waitForGTXDomain();

    for(int i=0; i < MAX_CHANNELS; i++) {
        // reset Data Replay
        dr[i]->setReset(1);
        opts.channelId = i;
        hlEventStream[i] = prepareEventStream(dev, bar, opts);
        if( !hlEventStream[i] )
        { exit(-1); }

        hlEventStream[i]->setEventCallback(event_callback);

        last_bytes_received[i] = 0;
        last_events_received[i] = 0;

        hlEventStream[i]->m_channel->setRateLimit(40000, 2);

        link[i]->setDataSourceDdr3DataReplay();
        link[i]->setFlowControlEnable(1);
        link[i]->setChannelActive(1);

        uint32_t ch_start_addr = (i % 6)*0x02000000;
        dr[i]->setStartAddress(ch_start_addr);

        dr[i]->setModeContinuous(1);
        dr[i]->setModeOneshot(0);
        dr[i]->setReset(0);
        dr[i]->setEnable(1);
    }

    /** make clear what will be checked*/
    //int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE;
    int32_t sanity_check_mask = CHK_SIZES|CHK_EOE|CHK_PATTERN;
    /*if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else if ( opts.datasource == ES_SRC_HWPG )
    { sanity_check_mask |= (CHK_PATTERN|CHK_ID|CHK_SOE); }
    else if ( opts.datasource == ES_SRC_DIU)
    { sanity_check_mask |= (CHK_DIU_ERR); }*/


    librorc::event_sanity_checker checker[MAX_CHANNELS];
    for(int i=0; i < MAX_CHANNELS; i++) {
        checker[i] =librorc::event_sanity_checker
            (
             hlEventStream[i]->m_eventBuffer,
             i,
             sanity_check_mask,
             logdirectory
            );
    }

    /** Create event stream */
    uint64_t result = 0;

    /** Capture starting time */
    timeval start_time;
    hlEventStream[0]->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;
#if 0
    timeval disable_time = start_time;
    bool trigger_en = false;
    uint64_t eventcount = 0;
#endif

    /** Event loop */
    while( !done )
    {
        for(int i=0; i < MAX_CHANNELS; i++) {
            result = hlEventStream[i]->handleChannelData( (void*)&(checker[i]) );
        }
        hlEventStream[0]->m_bar1->gettime(&current_time, 0);

        if(librorc::gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
        {
            for(int i=0; i < MAX_CHANNELS; i++) {
                //here we need a callback
                printStatusLine(last_time, current_time,
                        hlEventStream[i]->m_channel_status,
                        last_events_received[i], last_bytes_received[i]);

                last_bytes_received[i]  = hlEventStream[i]->m_channel_status->bytes_received;
                last_events_received[i] = hlEventStream[i]->m_channel_status->n_events;
            }
            cout << "----------------" << endl;
            last_time = current_time;
        }

#if 0
        if(!trigger_en) {
            for(int i=0; i < MAX_CHANNELS; i++) {
                dr[i]->setModeOneshot(0);
                dr[i]->setEnable(1);
            }
            trigger_en = true;
        } else {
            for(int i=0; i < MAX_CHANNELS; i++) {
                dr[i]->setModeOneshot(1);
            }
            trigger_en = false;
        }
#endif


#if 0
        if(trigger_en && librorc::gettimeofdayDiff(disable_time, current_time)>2*STAT_INTERVAL)
        {
            cout << "Re-Enabling Replay" << endl;
            for(int i=0; i < MAX_CHANNELS; i++) {
                dr[i]->setModeOneshot(0);
                dr[i]->setEnable(1);
            }
            trigger_en = false;
        }

        uint64_t events_diff = hlEventStream[0]->m_channel_status->n_events - eventcount;
        if( !trigger_en && events_diff>10 )
        {
            cout << "Disabling Replay" << endl;
            for(int i=0; i < MAX_CHANNELS; i++) {
                dr[i]->setModeOneshot(1);
            }
            disable_time = current_time;
            eventcount = hlEventStream[0]->m_channel_status->n_events;
            trigger_en = true;
        }
#endif
    }

    timeval end_time;
    hlEventStream[0]->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine
    (
        hlEventStream[0]->m_channel_status,
        opts,
        start_time,
        end_time
    );

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
        dr[i]->setReset(1);
        delete dr[i];
        delete link[i];
        delete hlEventStream[i];
    }
    delete sm;
    delete bar;
    delete dev;
    return result;
}
