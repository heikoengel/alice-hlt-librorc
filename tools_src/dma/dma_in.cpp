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


int handle_channel_data
(
    librorc::buffer      *rbuf,
    librorc::buffer      *ebuf,
    librorc::dma_channel *channel,
    librorcChannelStatus *stats,
    int                   do_sanity_check,
    uint32_t             *ddlref,
    uint64_t              ddlref_size
);

int dump_to_file
(
    char                     *base_dir,
    librorcChannelStatus     *stats,
    uint64_t                  EventID,
    uint32_t                  file_index,
    librorc_event_descriptor *reportbuffer,
    librorc::buffer          *ebuf,
    uint32_t                  error_flags
);


DMA_ABORT_HANDLER



int main(int argc, char *argv[])
{
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    if
    (
        !checkEventSize(opts.eventSize, argv[0]) &&
        (opts.esType == LIBRORC_ES_PG)
    )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    librorcChannelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

    DDLRefFile ddlref;
    if(opts.esType == LIBRORC_ES_DDL)
        { ddlref = getDDLReferenceFile(opts); }
    else if(opts.esType == LIBRORC_ES_PG)
    {
        ddlref.map  = NULL;
        ddlref.size = 0;
    }
    else
        { exit(-1); }

    /** Create event stream */
    librorc::event_stream *eventStream = NULL;
    if( !(eventStream = prepareEventStream(opts)) )
    { exit(-1); }

    eventStream->printDeviceStatus();

    /** Capture starting time */
    timeval start_time;
    eventStream->m_bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received  = 0;
    uint64_t last_events_received = 0;

    /** make clear what will be checked*/
    int     result        = 0;
    int32_t sanity_checks = 0xff; /** all checks defaults */
    if(ddlref.map && ddlref.size)
        {sanity_checks = CHK_FILE;}
    else if(opts.esType == LIBRORC_ES_DDL)
        {sanity_checks = CHK_SIZES;}

    /** Event loop */
    while( !done )
    {
        result = handle_channel_data
        (
            eventStream->m_reportBuffer,
            eventStream->m_eventBuffer,
            eventStream->m_channel,
            chstats,
            sanity_checks,
            ddlref.map,
            ddlref.size
        );

        if(result < 0)
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            return result;
        }
        else if(result==0)
        { usleep(200); } /** no events available */

        eventStream->m_bar1->gettime(&cur_time, 0);

        last_time =
            printStatusLine
                (last_time, cur_time, chstats,
                    &last_events_received, &last_bytes_received);
    }

    timeval end_time;
    eventStream->m_bar1->gettime(&end_time, 0);

    printFinalStatusLine(chstats, opts, start_time, end_time);

    /** Cleanup */
    delete eventStream;
    deleteDDLReferenceFile(ddlref);
    shmdt(chstats);

    return result;
}



/** limit the number of corrupted events to be written to disk **/
#define MAX_FILES_TO_DISK 100

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
    uint32_t             *ddl_reference,
    uint64_t              ddl_reference_size
)
{
    uint64_t    events_per_iteration = 0;
    int         events_processed     = 0;
    uint64_t    event_buffer_offset  = 0;
    uint64_t    report_buffer_offset = 0;
    uint64_t    starting_index       = 0;
    uint64_t    entry_size           = 0;
    uint64_t    event_id             = 0;
    int         retval               = 0;
    char        basedir[]            = "/tmp";



    librorc_event_descriptor *raw_report_buffer
        = (librorc_event_descriptor *)(report_buffer->getMem());
    volatile uint32_t *raw_event_buffer
        = (uint32_t *)(event_buffer->getMem());

    librorc::event_sanity_checker
        checker
        (
            raw_event_buffer,
            channel_status->channel,
            PG_PATTERN_INC, /** TODO */
            sanity_check_mask,
            ddl_reference,
            ddl_reference_size
        );


    /** new event received */
    if( raw_report_buffer[channel_status->index].calc_event_size!=0 )
    {
        // capture index of the first found reportbuffer entry
        starting_index = channel_status->index;

        // handle all following entries
        while( raw_report_buffer[channel_status->index].calc_event_size!=0 )
        {
            // increment number of events processed in this interation
            events_processed++;

            // perform validity tests on the received data (if enabled)
            if(sanity_check_mask)
            {
                librorc_event_descriptor report_buffer_entry
                    = raw_report_buffer[channel_status->index];

                try
                { event_id
                    = checker.check
                      (
                          &report_buffer_entry,
                          channel_status->index,
                          channel_status->last_id
                      );
                }
                catch( int error )
                { retval = error; }


                if ( retval!=0 )
                {
                    channel_status->error_count++;
                    if (channel_status->error_count < MAX_FILES_TO_DISK)
                    {
                        dump_to_file
                        (
                            basedir, // base dir
                            channel_status, // channel stats
                            event_id, // current EventID
                            channel_status->error_count, // file index
                            raw_report_buffer, // Report Buffer
                            event_buffer, // Event Buffer
                            retval // Error flags
                        );
                    }

                }

                channel_status->last_id = event_id;
            }

            // increment the number of bytes received
            channel_status->bytes_received +=
                (raw_report_buffer[channel_status->index].calc_event_size<<2);

            // save new EBOffset
            event_buffer_offset = raw_report_buffer[channel_status->index].offset;

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
        memset(&raw_report_buffer[starting_index], 0, events_per_iteration*entry_size);


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



/**
 * dump event to file(s)
 * @param base_dir pointer to destination directory string
 * @param pointer to stats channel stats struct
 * @param file_index index of according file, appears in file name
 * @param reportbuffer pointer to reportbuffer
 * @param ebuf pointer to librorc::buffer
 * @return 0 on sucess, -1 on error
 * */
//TODO: refactor this into a class and merge it with event_sanity_checker afterwards
int dump_to_file
(
    char                     *base_dir,
    librorcChannelStatus     *stats,
    uint64_t                  EventID,
    uint32_t                  file_index,
    librorc_event_descriptor *reportbuffer,
    librorc::buffer          *ebuf,
    uint32_t                  error_flags
)
{
    char *ddlname = NULL;
    char *logname = NULL;
    int length;
    int result;
    FILE *fd_ddl;
    FILE *fd_log;
    uint32_t i;
    uint32_t *eventbuffer = (uint32_t *)ebuf->getMem();

    // get length of destination file string
    length = snprintf(NULL, 0, "%s/ch%d_%d.ddl", base_dir, stats->channel, file_index);
    if(length<0)
    {
        perror("dump_to_file::snprintf failed");
        return -1;
    }

    // allocate memory for destination file string
    ddlname = (char *)malloc(length+1);
    if(ddlname==NULL)
    {
        perror("dump_to_file::malloc(ddlname) failed");
        return -1;
    }

    logname = (char *)malloc(length+1);
    if(logname==NULL)
    {
        perror("dump_to_file::malloc(logname) failed");
        return -1;
    }

    // fill destination file string
    snprintf(ddlname, length+1, "%s/ch%d_%d.ddl", base_dir, stats->channel, file_index);
    snprintf(logname, length+1, "%s/ch%d_%d.log", base_dir, stats->channel, file_index);

    // open DDL file
    fd_ddl = fopen(ddlname, "w");
    if( fd_ddl < 0 )
    {
        perror("failed to open destination DDL file");
        return -1;
    }

    // open log file
    fd_log =  fopen(logname, "w");
    if( fd_ddl == NULL )
    {
        printf("failed to open destination LOG file : %s\n", ddlname);
        return -1;
    }

    // dump RB entry to log
    fprintf
    (
        fd_log,
        "CH%2d - RB[%3ld]: \ncalc_size=%08x\n"
        "reported_size=%08x\n"
        "offset=%lx\n"
        "EventID=%lx\n"
        "LastID=%lx\n",
        stats->channel,
        stats->index,
        reportbuffer[stats->index].calc_event_size,
        reportbuffer[stats->index].reported_event_size,
        reportbuffer[stats->index].offset,
        EventID,
        stats->last_id
    );

    /** dump error type */
    fprintf(fd_log, "Check Failed: ");
    {
        !( error_flags & CHK_SIZES)    ? 0 : fprintf(fd_log, " CHK_SIZES");
        !( error_flags & CHK_PATTERN ) ? 0 : fprintf(fd_log, " CHK_PATTERN");
        !( error_flags & CHK_SOE )     ? 0 : fprintf(fd_log, " CHK_SOE");
        !( error_flags & CHK_EOE )     ? 0 : fprintf(fd_log, " CHK_EOE");
        !( error_flags & CHK_ID )      ? 0 : fprintf(fd_log, " CHK_ID");
        !( error_flags & CHK_FILE )    ? 0 : fprintf(fd_log, " CHK_FILE");
    }
    fprintf(fd_log, "\n\n");

    // check for reasonable calculated event size
    if(reportbuffer[stats->index].calc_event_size > (ebuf->getPhysicalSize()>>2))
    {
        fprintf
        (
            fd_log,
            "calc_event_size (0x%x DWs) is larger than physical "
            "buffer size (0x%lx DWs) - not dumping event.\n",
            reportbuffer[stats->index].calc_event_size,
            (ebuf->getPhysicalSize()>>2)
        );
    }
    else if (reportbuffer[stats->index].offset > ebuf->getPhysicalSize())// check for reasonable offset
    {
        fprintf(fd_log, "offset (0x%lx) is larger than physical "
        "buffer size (0x%lx) - not dumping event.\n",
        reportbuffer[stats->index].offset,
        ebuf->getPhysicalSize() );
    }
    else // dump event to log
    {
        for(i=0;i<reportbuffer[stats->index].calc_event_size;i++)
        {
            uint32_t ebword =
                (uint32_t)*(eventbuffer + (reportbuffer[stats->index].offset>>2) + i); // TODO: array this

            fprintf(fd_log, "%03d: %08x", i, ebword);

            if ( (error_flags & CHK_PATTERN) && (i>7) && (ebword != i-8) )
            { fprintf(fd_log, " expected %08x", i-8); }

            fprintf(fd_log, "\n");
        }

        fprintf
        (
            fd_log,
            "%03d: EOE reported_event_size: %08x\n",
            i,
            (uint32_t)*(eventbuffer + (reportbuffer[stats->index].offset>>2) + i)
        );

        //dump event to DDL file
        result =
            fwrite
            (
                eventbuffer + (reportbuffer[stats->index].offset>>2),
                4,
                reportbuffer[stats->index].calc_event_size,
                fd_ddl
            );
        if( result<0 )
        {
            perror("failed to copy event data into DDL file");
            return -1;
        }
    }

    fclose(fd_log);
    fclose(fd_ddl);

    return 0;
}

