/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-11-05
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
#define MAPPING_FILE "/mnt/data/svn/CRORC/software/FCF/common/FCF_Mapping/FCF_Mapping_Patch0.data"

bool done = false;
void abort_handler( int s )                                          \
{                                        \
    printf("Caught signal %d\n", s);     \
    if( done==true )      \
    { exit(-1); }                        \
    else                                 \
    { done = true; }                     \
}

uint64_t
eventCallBack
(
    void                     *userdata,
    uint64_t                  event_id,
    librorc_event_descriptor  report,
    const uint32_t           *event,
    librorcChannelStatus     *channel_status
)
{
    librorc::event_sanity_checker *checker = (librorc::event_sanity_checker*)userdata;

    try{ checker->check(report, channel_status, event_id); }
    catch(...){ abort(); }
    return 0;
}



void
loadFcfRam
(
    librorc::link *link,
    const char *fname
)
{
    ifstream memfile(fname);
    if ( !memfile.is_open() )
    {
        cout << "Failed to open mapping file" << endl;
        abort();
    }

    string line;
    uint32_t i = 0;

    while ( getline(memfile, line) )
    {
        uint32_t hexval;
        stringstream ss;
        ss << hex << line;
        ss >> hexval;
        cout << dec << i << ": " << hex << hexval << endl;

        if ( i>4095 )
        {
            cout << "Mapping file has more than 4096 entries - skipping remaining lines" << endl;
            break;
        }

        link->setGTX(RORC_REG_FCF_RAM_DATA, hexval);
        link->setGTX(RORC_REG_FCF_RAM_CTRL, i);

        i++;
    }

    if ( i<4096 )
    {
        cout << "WARNING: only wrote " << dec << i 
             << " entries from mapping file." << endl;
    }

}




int main(int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
    DMAOptions opts;
    opts = evaluateArguments(argc, argv);

    DMA_ABORT_HANDLER_REGISTER

    librorc_event_callback event_callback = eventCallBack;


    librorc::device *dev;
    librorc::bar *bar;

    try
    {
        dev = new librorc::device(opts.deviceId, LIBRORC_ES_IN_GENERIC);
#ifdef SIM
        bar = new librorc::sim_bar(dev, 1);
#else
        bar = new librorc::rorc_bar(dev, 1);
#endif

    }
    catch(...)
    {
        cout << "Failed to initialize dev and bar" << endl;
        abort();
    }

    librorc::link *link = new librorc::link(bar, opts.channelId);

    /** wait until GTX domain is up */
    cout << "Waiting for GTX domain..." << endl;
    while ( !link->isGtxDomainReady() )
    {
        usleep(100);
    }

    loadFcfRam(link, MAPPING_FILE);

    /** wait until DDL is up */
    cout << "Waiting for DIU riLD_N..." << endl;
    while( (link->GTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20)
    {
        usleep(100);
    }

    /** set ENABLE */
    uint32_t ddlctrl = (1<<0) | //enable DDLIF
        (1<<1) | // enable flow control
        (0<<3) | // set MUX to use DDL as data source
        (1<<9) | // enable PG adaptive
        (1<<10) | // enable continuous mode
        (1<<8) | // enable PG
        //(1<<13) | // enable PRBS EventLength
        (0<<11); // set mode to INCREMENT
    link->setGTX(RORC_REG_DDL_CTRL, ddlctrl);

    /** send RdyRx to SIU */
    link->setGTX(RORC_REG_DDL_CMD, 0x00000014);


    uint64_t last_bytes_received;
    uint64_t last_events_received;

    librorc::event_stream *eventStream;
    cout << "Prepare ES" << endl;
    if( !(eventStream = prepareEventStream(dev, bar, opts)) )
    { exit(-1); }
    eventStream->setEventCallback(event_callback);

    last_bytes_received = 0;
    last_events_received = 0;

    eventStream->printDeviceStatus();

    /** enable EBDM + RBDM + PKT */
    link->setPacketizer(RORC_REG_DMA_CTRL, 
            (link->packetizer(RORC_REG_DMA_CTRL) | 0x0d) );


    /** make clear what will be checked*/
    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE;
    if(opts.useRefFile)
    {
        sanity_check_mask |= CHK_FILE;
    }
    else
    {
        sanity_check_mask |= CHK_PATTERN | CHK_ID;
    }

    cout << "Event Loop Start" << endl;

    librorc::event_sanity_checker checker = 
        librorc::event_sanity_checker
        (
         eventStream->m_eventBuffer,
         opts.channelId,
         PG_PATTERN_INC, /** TODO */
         sanity_check_mask,
         logdirectory,
         opts.refname
        );


    /** Create event stream */
    uint64_t result = 0;

    /** Capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval current_time = start_time;


    /** Event loop */
    while( !done )
    {
        result = eventStream->handleChannelData( (void*)&(checker) );

        eventStream->m_bar1->gettime(&current_time, 0);

        if(gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
        {
            //here we need a callback
            printStatusLine
                (
                 last_time,
                 current_time,
                 eventStream->m_channel_status,
                 last_events_received,
                 last_bytes_received
                );

            last_bytes_received  = eventStream->m_channel_status->bytes_received;
            last_events_received = eventStream->m_channel_status->n_events;
        }
        last_time = current_time;
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(
            eventStream->m_channel_status,
            eventStream->m_start_time,
            eventStream->m_end_time);

    /** Cleanup */
    delete link;
    delete eventStream;
    delete bar;
    delete dev;
    return result;
}
