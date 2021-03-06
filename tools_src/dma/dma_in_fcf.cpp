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

#define MAX_CHANNELS 12
#define MAPPING_FILE "/build/ddl_data/FCF_Mapping/FCF_Mapping_Patch0.data"
//#define INPUT_FILE "/build/ddl_data/raw28/TPC_962.ddl"
#define INPUT_FILE "/build/ddl_data/raw28/TPC_768.ddl"
#define INPUT_FILE2 "/build/ddl_data/raw28/TPC_959.ddl"

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
    cout << "Writing " << filename << " to DDR3..." << endl;
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

    if
    (
        (opts.datasource == ES_SRC_HWPG) &&
        !checkEventSize(opts.eventSize, argv[0])
    )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    librorc_event_callback event_callback = eventCallBack;


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

    librorc::link *link = new librorc::link(bar, opts.channelId);



    /** wait until GTX domain is up */
    cout << "Waiting for GTX domain..." << endl;
    link->waitForGTXDomain();

    /** wait until DDL is up */
    /*cout << "Waiting for DIU riLD_N..." << endl;
    while( (link->GTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20)
    {
        usleep(100);
    }*/


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

    /**
     * configure datastream: from DDR3 through FCF
     **/
    if( opts.datasource==ES_SRC_DDR3 &&
            !link->ddr3DataReplayAvailable() )
    {
        cout << "ERROR: DDR3 Data Replay is not available." << endl;
        abort();
    }

    librorc::datareplaychannel *dr = new librorc::datareplaychannel(link);

    // reset Data Replay
    dr->setReset(1);

    cout << "Waiting for phy_init_done..." << endl;
    while ( !sm->ddr3ModuleInitReady(0) )
    { usleep(100); }


    uint32_t ch_start_addr = 0;

    //uint32_t next_addr =
        fileToRam(sm, INPUT_FILE, opts.channelId, ch_start_addr, false);
    //next_addr =
    //    fileToRam(sm, INPUT_FILE2, opts.channelId, next_addr, true);

    //uint32_t next_addr =
        fileToRam(sm, INPUT_FILE, opts.channelId, ch_start_addr, false);
    //next_addr =
    //    fileToRam(sm, INPUT_FILE2, opts.channelId, next_addr, true);

    //-----------------------------------------------------//


    uint64_t last_bytes_received;
    uint64_t last_events_received;

    librorc::event_stream *hlEventStream;
    cout << "Prepare ES" << endl;

    hlEventStream = prepareEventStream(dev, bar, opts);
    if( !hlEventStream )
    { exit(-1); }

    hlEventStream->setEventCallback(event_callback);

    last_bytes_received = 0;
    last_events_received = 0;

    hlEventStream->printDeviceStatus();


    librorc::fastclusterfinder *fcf =
        hlEventStream->getFastClusterFinder();
    if( !fcf )
    {
        cout << "Clusterfinder not available for this FW/Channel!"
             << endl;
        abort();
    }

    fcf->setState(1, 0); // reset, not enabled
#ifndef MODELSIM
    if( opts.loadFcfMappingRam )
    {
        fcf->loadMappingRam(opts.fcfmappingfile);
    }
#endif
    fcf->setSinglePadSuppression(0);
    fcf->setBypassMerger(0);
    fcf->setDeconvPad(1);
    fcf->setSingleSeqLimit(0);
    fcf->setClusterLowerLimit(10);
    fcf->setMergerDistance(4);
    fcf->setMergerAlgorithm(1);
    fcf->setChargeTolerance(0);
    fcf->setState(0, 1);// not reset, enabled
    delete fcf;

    link->setDataSourceDdr3DataReplay();
    link->setFlowControlEnable(1);

    dr->setReset(0);
    dr->setModeContinuous(0);
    dr->setModeOneshot(0);
    dr->setStartAddress(ch_start_addr);
    dr->setEnable(1);

    /** make clear what will be checked*/
    //int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE;
    int32_t sanity_check_mask = CHK_SIZES|CHK_EOE;
    if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else if ( opts.datasource == ES_SRC_HWPG )
    { sanity_check_mask |= (CHK_PATTERN|CHK_ID|CHK_SOE); }
    else if ( opts.datasource == ES_SRC_DIU)
    { sanity_check_mask |= (CHK_DIU_ERR); }


    librorc::event_sanity_checker checker =
        (opts.useRefFile)
        ?   librorc::event_sanity_checker
            (
                hlEventStream->m_eventBuffer,
                opts.channelId,
                sanity_check_mask,
                logdirectory,
                opts.refname
            )
        :   librorc::event_sanity_checker
            (
                hlEventStream->m_eventBuffer,
                opts.channelId,
                sanity_check_mask,
                logdirectory
            ) ;

    /** Create event stream */
    uint64_t result = 0;

    /** Capture starting time */
    timeval start_time;
    hlEventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;


    /** Event loop */
    while( !done )
    {
        result = hlEventStream->handleChannelData( (void*)&(checker) );

        if( result )
        {
            if ( dr->isDone() )
                dr->setEnable(1);
        }

        hlEventStream->m_bar1->gettime(&current_time, 0);

        if(gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
        {
            //here we need a callback
            printStatusLine
                (
                 last_time,
                 current_time,
                 hlEventStream->m_channel_status,
                 last_events_received,
                 last_bytes_received
                );

            last_bytes_received  = hlEventStream->m_channel_status->bytes_received;
            last_events_received = hlEventStream->m_channel_status->n_events;
            last_time = current_time;
        }
    }

    timeval end_time;
    hlEventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine
    (
        hlEventStream->m_channel_status,
        opts,
        start_time,
        end_time
    );

    /** Cleanup */
    delete dr;
    delete sm;
    delete link;
    delete hlEventStream;
    delete bar;
    delete dev;
    return result;
}
