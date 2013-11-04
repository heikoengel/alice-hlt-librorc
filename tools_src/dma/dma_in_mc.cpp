/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-08-23
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

#include <librorc.h>

#include "dma_handling.hh"

using namespace std;

#define MAX_CHANNELS 12

int handle_channel_data
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    librorcChannelStatus *stats,
    int                   do_sanity_check,
    bool                  ddl_reference_is_enabled,
    char                 *ddl_path
);



DMA_ABORT_HANDLER



int main(int argc, char *argv[])
{
    DMAOptions opts[MAX_CHANNELS];
    opts[0] = evaluateArguments(argc, argv);
    int32_t nChannels = opts[0].channelId;
    opts[0].channelId = 0;
    opts[0].esType = LIBRORC_ES_IN_HWPG;

    int i;

        cout << "using " << nChannels << " Channels" << endl;

    for (i=1; i<nChannels; i++)
    {
        opts[i] = opts[0];
        opts[i].channelId = i;
    }

    DMA_ABORT_HANDLER_REGISTER

    librorcChannelStatus *chstats[MAX_CHANNELS];
    for ( i=0; i<nChannels; i++ )
    {
        chstats[i] = prepareSharedMemory(opts[i]);
        if(chstats[i] == NULL)
        { exit(-1); }
    }

    /** Create event stream */
    uint64_t last_bytes_received = 0;
    uint64_t last_events_received = 0;

    librorc::device *dev;
    librorc::bar *bar;

    try
    {
        dev = new librorc::device(opts[0].deviceId);
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

    librorc::event_stream *eventStream[MAX_CHANNELS];
    for ( i=0; i<nChannels; i++ )
    {
        cout << "Prepare ES " << i << endl;
        if( !(eventStream[i] = prepareEventStream(dev, bar, opts[i])) )
        { exit(-1); }
    }

    //eventStream->printDeviceStatus();

    /** Capture starting time */
    timeval start_time;
    eventStream[0]->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;


    /** make clear what will be checked*/
    int     result        = 0;
    int32_t sanity_checks = CHK_SIZES|CHK_SOE|CHK_EOE;
    if(opts[0].useRefFile)
    {
        sanity_checks |= CHK_FILE;
    }
    else
    {
        sanity_checks |= CHK_PATTERN | CHK_ID;
    }
        
    cout << "Event Loop Start" << endl;

    /** Event loop */
    while( !done )
    {
    
        for ( i=0; i<nChannels; i++ )
        {
            result = handle_channel_data
                (
                 eventStream[i]->m_reportBuffer,
                 eventStream[i]->m_eventBuffer,
                 eventStream[i]->m_channel,
                 chstats[i],
                 sanity_checks,
                 opts[i].useRefFile,
                 opts[i].refname
                );

            if(result < 0)
            {
                printf("handle_channel_data failed for channel %d\n", 
                        opts[i].channelId);
                return result;
            }
        }

        eventStream[0]->m_bar1->gettime(&cur_time, 0);

        last_time = printStatusLine(last_time, cur_time, chstats[0],
             &last_events_received, &last_bytes_received);
    }

    timeval end_time;
    eventStream[0]->m_bar1->gettime(&end_time, 0);

    for ( i=0; i<nChannels; i++ )
    {
        printFinalStatusLine(chstats[i], opts[i], start_time, end_time);

        /** Cleanup */
        delete eventStream[i];
        shmdt(chstats[i]);
    }

    delete bar;
    delete dev;

    return result;
}



/**
 * handle incoming data
 *
 * check if there is a reportbuffer entry at the current polling index
 * if yes, handle all available reportbuffer entries
 * @param rbuf pointer to ReportBuffer
 * @param eventbuffer pointer to EventBuffer Memory
 * @param channel pointer
 * @param ch_stats pointer to channel stats struct
 * @param do_sanity_check mask of sanity checks to be done on the
 * received data. See CHK_* defines above.
 * @return number of events processed
 **/
//TODO: refactor this into a class and merge it with event stream afterwards
int handle_channel_data
(
    librorc::buffer      *report_buffer,
    librorc::buffer      *event_buffer,
    librorc::dma_channel *channel,
    librorcChannelStatus *channel_status,
    int                   sanity_check_mask,
    bool                  ddl_reference_is_enabled,
    char                 *ddl_path
)
{
    uint64_t    events_per_iteration = 0;
    int         events_processed     = 0;
    uint64_t    event_buffer_offset  = 0;
    uint64_t    report_buffer_offset = 0;
    uint64_t    starting_index       = 0;
    uint64_t    entry_size           = 0;
    uint64_t    event_id             = 0;
    char        log_directory_path[] = "/tmp";

    librorc_event_descriptor *reports
        = (librorc_event_descriptor *)(report_buffer->getMem());

    librorc::event_sanity_checker checker =
        ddl_reference_is_enabled
        ?
            librorc::event_sanity_checker
            (
                event_buffer,
                channel_status->channel,
                PG_PATTERN_INC, /** TODO */
                sanity_check_mask,
                log_directory_path,
                ddl_path
            )
        :
            librorc::event_sanity_checker
            (
                event_buffer,
                channel_status->channel,
                PG_PATTERN_INC,
                sanity_check_mask,
                log_directory_path
            )
        ;


    /** new event received */
    if( reports[channel_status->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        starting_index = channel_status->index;

        // handle all following entries
        while( reports[channel_status->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform selected validity tests on the received data
            // dump stuff if errors happen
            try
            { event_id = checker.check(reports, channel_status); }
            catch(...){ abort(); }

            channel_status->last_id = event_id;

            // increment the number of bytes received
            channel_status->bytes_received +=
                (reports[channel_status->index].calc_event_size<<2);

            // save new EBOffset
            event_buffer_offset = reports[channel_status->index].offset;

            // increment reportbuffer offset
            report_buffer_offset = ((channel_status->index)*sizeof(librorc_event_descriptor)) % report_buffer->getPhysicalSize();

            // wrap RB index if necessary
            if( channel_status->index < report_buffer->getMaxRBEntries()-1 )
            { channel_status->index++; }
            else
            { channel_status->index=0; }

            //increment total number of events received
            channel_status->n_events++;

            //increment number of events processed in this while-loop
            events_per_iteration++;
        }

        // clear processed reportbuffer entries
        entry_size = sizeof(librorc_event_descriptor);
        memset(&reports[starting_index], 0, events_per_iteration*entry_size);


        // update min/max statistics on how many events have been received
        // in the above while-loop
        if(events_per_iteration > channel_status->max_epi)
        { channel_status->max_epi = events_per_iteration; }

        if(events_per_iteration < channel_status->min_epi)
        { channel_status->min_epi = events_per_iteration; }

        events_per_iteration = 0;
        channel_status->set_offset_count++;

        // actually update the offset pointers in the firmware
        channel->setBufferOffsetsOnDevice(event_buffer_offset, report_buffer_offset);

        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Setting swptrs: RBDM=%016lx EBDM=%016lx\n",
            channel_status->channel,
            report_buffer_offset,
            event_buffer_offset
        );
    }

    return events_processed;
}





