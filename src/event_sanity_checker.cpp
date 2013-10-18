/**
 * @author Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>, Heiko Engel <hengel@cern.ch>
 * @date 2013-09-15
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
 * @section DESCRIPTION
 *
 * The rorc_bar class represents a Base Address Register (BAR) file
 * mapping of the RORCs PCIe address space
 */

#include <librorc/event_sanity_checker.hh>


namespace LIBRARY_NAME
{

    /** Class that dumps errors to files */
    #define LIBRORC_FILE_DUMPER_ERROR_CONSTRUCTOR_FAILED   1
    #define LIBRORC_FILE_DUMPER_ERROR_FILE_OPEN_FAILED     2
    #define LIBRORC_FILE_DUMPER_ERROR_LOGGING_EVENT_FAILED 3

    /** limit the number of corrupted events to be written to disk **/
    #define MAX_FILES_TO_DISK 100

    class file_dumper
    {
    //TODO: add device number
        public:

             file_dumper
             (
                 char                     *base_dir,
                 librorc_event_descriptor *reports
             )
             {
                 m_base_dir = base_dir;
                 m_reports  = reports;
             };

            ~file_dumper(){ };

            /**
             * Dump event to files
             * @param channel status
             * @param index of according file, appears in file name
             * @param report buffer
             * @param event buffer
             * */
            void
            dump
            (
                librorcChannelStatus *channel_status,
                uint64_t              event_id,
                librorc::buffer      *event_buffer,
                uint32_t              error_bit_mask
            )
            {
                if (channel_status->error_count < MAX_FILES_TO_DISK)
                {
                    uint32_t                  file_index          = channel_status->error_count;
                                              m_raw_event_buffer  = (uint32_t *)event_buffer->getMem();

                    openFiles(file_index, channel_status);
                    dumpReportBufferEntryToLog(event_id, channel_status, m_reports);
                    dumpErrorTypeToLog(error_bit_mask);

                    //TODO : review this bool (dump event if something fails)?
                    bool
                    dump_event =
                          calculatedIsLargerThanPhysical(m_reports, channel_status, event_buffer)
                        ? dumpCalculatedIsLargerThanPhysicalToLog(m_reports, channel_status, event_buffer)
                        : true;

                    dump_event &=
                          offsetIsLargerThanPhysical(m_reports, channel_status, event_buffer)
                        ? dumpOffsetIsLargerThanPhysicalToLog(m_reports, channel_status, event_buffer)
                        : true;

                    dump_event ? dumpEventToLog(error_bit_mask, m_reports, channel_status) : (void)0;

                    closeFiles();
                }
            }



        protected:
            char     *m_base_dir;
            char      m_ddl_file_name[4096];
            FILE     *m_fd_ddl;
            char      m_log_file_name[4096];
            FILE     *m_fd_log;
            uint32_t *m_raw_event_buffer;
            librorc_event_descriptor *m_reports;

            void
            openFiles
            (
                uint32_t              file_index,
                librorcChannelStatus *channel_status
            )
            {
                int lengthOfDestinationFilePath =
                        snprintf(NULL, 0, "%s/ch%d_%d.ddl", m_base_dir, channel_status->channel, file_index);

                if (lengthOfDestinationFilePath < 0)
                { throw LIBRORC_FILE_DUMPER_ERROR_FILE_OPEN_FAILED; }

                snprintf(m_ddl_file_name, lengthOfDestinationFilePath+1, "%s/ch%d_%d.ddl",
                         m_base_dir, channel_status->channel, file_index);
                snprintf(m_log_file_name, lengthOfDestinationFilePath+1, "%s/ch%d_%d.log",
                         m_base_dir, channel_status->channel, file_index);

                m_fd_ddl = fopen(m_ddl_file_name, "w");
                if(m_fd_ddl < 0)
                { throw LIBRORC_FILE_DUMPER_ERROR_FILE_OPEN_FAILED; }

                m_fd_log = fopen(m_log_file_name, "w");
                if(m_fd_ddl == NULL)
                { throw LIBRORC_FILE_DUMPER_ERROR_FILE_OPEN_FAILED; }
            }

            void
            closeFiles()
            {
                fclose(m_fd_log);
                fclose(m_fd_ddl);
            }

            void
            dumpReportBufferEntryToLog
            (
                uint64_t                  event_id,
                librorcChannelStatus     *channel_status,
                librorc_event_descriptor *report_buffer_entry
            )
            {
                fprintf
                (
                    m_fd_log,
                    "CH%2d - RB[%3ld]: \ncalc_size=%08x\n"
                    "reported_size=%08x\noffset=%lx\nEventID=%lx\nLastID=%lx\n",
                    channel_status->channel,
                    channel_status->index,
                    report_buffer_entry[channel_status->index].calc_event_size,
                    report_buffer_entry[channel_status->index].reported_event_size,
                    report_buffer_entry[channel_status->index].offset,
                    event_id,
                    channel_status->last_id
                );
            }

            void
            dumpErrorTypeToLog(uint32_t error_bit_mask)
            {
                fprintf(m_fd_log, "Check Failed: ");
                {
                    !(error_bit_mask & CHK_SIZES)   ? 0 : fprintf(m_fd_log, " CHK_SIZES");
                    !(error_bit_mask & CHK_PATTERN) ? 0 : fprintf(m_fd_log, " CHK_PATTERN");
                    !(error_bit_mask & CHK_SOE)     ? 0 : fprintf(m_fd_log, " CHK_SOE");
                    !(error_bit_mask & CHK_EOE)     ? 0 : fprintf(m_fd_log, " CHK_EOE");
                    !(error_bit_mask & CHK_ID)      ? 0 : fprintf(m_fd_log, " CHK_ID");
                    !(error_bit_mask & CHK_FILE)    ? 0 : fprintf(m_fd_log, " CHK_FILE");
                }
                fprintf(m_fd_log, "\n\n");
            }

            bool
            calculatedIsLargerThanPhysical
            (
                librorc_event_descriptor *report_buffer_entry,
                librorcChannelStatus     *channel_status,
                librorc::buffer          *event_buffer
            )
            {
                return   ((report_buffer_entry[channel_status->index].calc_event_size) & 0x3fffffff)
                       > (event_buffer->getPhysicalSize() >> 2);
            }

            bool
            dumpCalculatedIsLargerThanPhysicalToLog
            (
                librorc_event_descriptor *report_buffer_entry,
                librorcChannelStatus     *channel_status,
                librorc::buffer          *event_buffer
            )
            {
                fprintf
                (
                    m_fd_log,
                    "calc_event_size (0x%x DWs) is larger"
                    " than physical buffer size (0x%lx DWs) - not dumping event.\n",
                    (report_buffer_entry[channel_status->index].calc_event_size & 0x3fffffff),
                    (event_buffer->getPhysicalSize() >> 2)
                );

                return false;
            }

            bool
            offsetIsLargerThanPhysical
            (
                librorc_event_descriptor* report_buffer_entry,
                librorcChannelStatus* channel_status,
                librorc::buffer* event_buffer
            )
            {
                return   report_buffer_entry[channel_status->index].offset
                       > event_buffer->getPhysicalSize();
            }


            bool
            dumpOffsetIsLargerThanPhysicalToLog
            (
                librorc_event_descriptor* report_buffer_entry,
                librorcChannelStatus* channel_status,
                librorc::buffer* event_buffer
            )
            {
                fprintf
                (
                    m_fd_log,
                    "offset (0x%lx) is larger than physical buffer"
                    " size (0x%lx) - not dumping event.\n",
                    report_buffer_entry[channel_status->index].offset,
                    event_buffer->getPhysicalSize()
                );

                return false;
            }

            void
            dumpEventToLog
            (
                uint32_t                  error_bit_mask,
                librorc_event_descriptor *report_buffer_entry,
                librorcChannelStatus     *channel_status
            )
            {
                uint32_t i;

                for
                (
                    i = 0;
                    i < report_buffer_entry[channel_status->index].calc_event_size;
                    i++
                )
                {
                    uint32_t ebword =
                        (uint32_t) *(
                                        m_raw_event_buffer +
                                        (report_buffer_entry[channel_status->index].offset >> 2) + i
                                    );

                    fprintf(m_fd_log, "%03d: %08x", i, ebword);

                    if((error_bit_mask & CHK_PATTERN) && (i > 7) && (ebword != i - 8))
                    { fprintf(m_fd_log, " expected %08x", i - 8); }

                    fprintf(m_fd_log, "\n");
                }

                fprintf
                (
                    m_fd_log,
                    "%03d: EOE reported_event_size: %08x\n",
                    i,
                    (uint32_t) *(m_raw_event_buffer + (report_buffer_entry[channel_status->index].offset >> 2) + i)
                );

                size_t size_written =
                    fwrite
                    (
                            m_raw_event_buffer + (report_buffer_entry[channel_status->index].offset >> 2),
                            4,
                            report_buffer_entry[channel_status->index].calc_event_size,
                            m_fd_ddl
                    );

                if( size_written < 0 )
                { throw LIBRORC_FILE_DUMPER_ERROR_LOGGING_EVENT_FAILED; }
            }

    };


    /** class to handle DDL reference files */
    #define LIBRORC_DDL_REFERENCE_FILE_ERROR_CONSTRUCTOR_FAILED   1

    class ddl_reference_file
    {
        public:
            ddl_reference_file
            (
                char *refname
            )
            {
                m_fd = open(refname, O_RDONLY);
                if(m_fd<0)
                { throw(LIBRORC_DDL_REFERENCE_FILE_ERROR_CONSTRUCTOR_FAILED); }

                struct stat ddlref_stat;
                if(fstat(m_fd, &ddlref_stat)==-1)
                { throw(LIBRORC_DDL_REFERENCE_FILE_ERROR_CONSTRUCTOR_FAILED); }

                m_size = ddlref_stat.st_size;
                m_map =
                    (uint32_t *)
                        mmap(0, m_size, PROT_READ, MAP_SHARED, m_fd, 0);
                if(m_map == MAP_FAILED)
                { throw(LIBRORC_DDL_REFERENCE_FILE_ERROR_CONSTRUCTOR_FAILED); }
            }

            ~ddl_reference_file()
             {
                munmap(m_map, m_size);
                close(m_fd);
             }

            uint64_t size()
            {
                return m_size;
            }

            uint32_t *mapping()
            {
                return m_map;
            }

        protected:
            uint64_t  m_size;
            int       m_fd;
            uint32_t *m_map;
    };

/*************event_sanit_checker - METHODS ********************************/

event_sanity_checker::event_sanity_checker
(
    buffer            *event_buffer,
    uint32_t           channel_id,
    uint32_t           pattern_mode,
    uint32_t           check_mask,
    char              *log_base_dir
)
{
    m_event_buffer        = event_buffer;
    m_raw_event_buffer    = (uint32_t *)(event_buffer->getMem());
    m_channel_id          = channel_id;
    m_pattern_mode        = pattern_mode;
    m_check_mask          = check_mask;
    m_event_index         = 0;
    m_event               = NULL;
    m_reported_event_size = 0;
    m_log_base_dir        = log_base_dir;

    m_ddl                 = NULL;
};



event_sanity_checker::event_sanity_checker
(
    buffer            *event_buffer,
    uint32_t           channel_id,
    uint32_t           pattern_mode,
    uint32_t           check_mask,
    char              *log_base_dir,
    char              *ddl_reference_file_path
)
{
    m_event_buffer        = event_buffer;
    m_raw_event_buffer    = (uint32_t *)(event_buffer->getMem());
    m_channel_id          = channel_id;
    m_pattern_mode        = pattern_mode;
    m_check_mask          = check_mask;
    m_event_index         = 0;
    m_event               = NULL;
    m_reported_event_size = 0;
    m_log_base_dir        = log_base_dir;

    m_ddl                 = new ddl_reference_file(ddl_reference_file_path);
};



event_sanity_checker::~event_sanity_checker()
{
    if(m_ddl != NULL)
    { delete m_ddl; }
};



uint64_t
event_sanity_checker::check
(
    librorc_event_descriptor *reports,
    librorcChannelStatus     *channel_status
)
{
    uint64_t                  report_buffer_index =  channel_status->index;
    librorc_event_descriptor *report_entry        = &reports[channel_status->index];
    int64_t                   last_id             =  channel_status->last_id;

    m_event               = rawEventPointer(report_entry);
    m_reported_event_size = reportedEventSize(report_entry);
    m_calc_event_size     = calculatedEventSize(report_entry);
    m_event_index         = 0;

    uint64_t event_id     = getEventIdFromCdh(dwordOffset(report_entry));
    int      error_code   = 0;
    {
        error_code |= !(m_check_mask & CHK_SIZES)   ? 0 : compareCalculatedToReportedEventSizes(report_entry, report_buffer_index);
        error_code |= !(m_check_mask & CHK_SOE)     ? 0 : checkStartOfEvent(report_entry, report_buffer_index);
        error_code |= !(m_check_mask & CHK_PATTERN) ? 0 : checkPattern(report_entry, report_buffer_index);
        error_code |= !(m_check_mask & CHK_FILE)    ? 0 : compareWithReferenceDdlFile(report_entry, report_buffer_index);
        error_code |= !(m_check_mask & CHK_EOE)     ? 0 : checkEndOfEvent(report_entry, report_buffer_index);
        error_code |= !(m_check_mask & CHK_ID)      ? 0 : checkForLostEvents(report_entry, report_buffer_index, last_id);
    }

    if(error_code != 0)
    {
        channel_status->error_count++;

        file_dumper dumper(m_log_base_dir, reports);

        dumper.dump
        (
           channel_status,
           event_id,
           m_event_buffer,
           error_code
        );
    }

    return event_id;
}



void
event_sanity_checker::dumpEvent
(
    volatile uint32_t *event_buffer,
    uint64_t offset,
    uint64_t length
)
{
#ifdef SIM
    uint64_t i = 0;
    for(i=0; i<length; i++)
    { printf("%03ld: %08x\n", i, (uint32_t)*(event_buffer + offset +i)); }

    if(length&0x01)
    {
        printf("%03ld: %08x (dummy)\n", i, (uint32_t)*(event_buffer + offset + i));
        i++;
    }

    printf("%03ld: EOE reported_event_size: %08x\n", i, (uint32_t)*(event_buffer + offset + i));
#endif
}



void
event_sanity_checker::dumpReportBufferEntry
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  index,
             uint32_t                  channel_number
)
{
    DEBUG_PRINTF
    (
        PDADEBUG_CONTROL_FLOW,
        "CH%2d - RB[%3ld]: calc_size=%08x\t"
        "reported_size=%08x\t"
        "offset=%lx\n",
        channel_number, index, report_buffer->calc_event_size,
        report_buffer->reported_event_size,
        report_buffer->offset
    );
}



/** PROTECTED METHODS*/
int
event_sanity_checker::dumpError
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index,
             int32_t                   check_id
)
{
    dumpEvent(m_raw_event_buffer, dwordOffset(report_buffer), m_reported_event_size);
    dumpReportBufferEntry(report_buffer, report_buffer_index, m_channel_id);
    return check_id;
}



int
event_sanity_checker::compareCalculatedToReportedEventSizes
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    /** Bit 31 of calc_event_size is read completion timeout flag */
    uint32_t timeout_flag = (report_buffer->calc_event_size>>31);

    if(timeout_flag)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
                m_channel_id, report_buffer_index);
        return(CHK_SIZES);
    }
    else if(m_calc_event_size != m_reported_event_size)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                        "calculated: 0x%x, reported: 0x%x\n"
                        "offset=0x%lx, rbdm_offset=0x%lx\n", m_channel_id,
                report_buffer_index, m_calc_event_size, m_reported_event_size,
                report_buffer->offset,
                report_buffer_index * sizeof(librorc_event_descriptor));
        return(CHK_SIZES);
    }

    return 0;
}



int
event_sanity_checker::checkStartOfEvent
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    if((uint32_t) * (m_event) != 0xffffffff)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
                        "offset=%ld, rbdm_offset=%ld\n", report_buffer_index,
                (uint32_t) * (m_event), report_buffer->offset,
                report_buffer_index * sizeof(librorc_event_descriptor));
        return dumpError(report_buffer, report_buffer_index, CHK_SOE);
    }

    return 0;
}



int
event_sanity_checker::checkPatternInc
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t base_value = 0;

    uint32_t *event     = (m_event+8);
    uint64_t  length    = (m_calc_event_size-8);
    m_event_index       = 8;
    for(uint32_t i = base_value; i<(length+base_value); i++)
    {
        if( event[i] != i )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                m_event_index,
                event[m_event_index]
            );
            m_event_index++;
            return dumpError(report_buffer, report_buffer_index, CHK_PATTERN);
        }
        m_event_index++;
    }



    return 0;
}



int
event_sanity_checker::checkPatternDec
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t *event  = (m_event+8);
    uint64_t  length = (m_calc_event_size-8);
    m_event_index    = 8;
    for(uint32_t i=0; i<length; i++)
    {
        if( event[i] != ((length-1)-i) )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                ((length-1)-i),
                event[m_event_index]
            );
            m_event_index++;
            return dumpError(report_buffer, report_buffer_index, CHK_PATTERN);
        }
        m_event_index++;
    }

    return 0;
}



int
event_sanity_checker::checkPatternShift
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t base_value = 0;

    uint32_t *event     = (m_event+8);
    uint64_t  length    = (m_calc_event_size-8);
    m_event_index       = 8;

    for(uint32_t i=0; i<length; i++)
    {
        if( event[i] != base_value )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                base_value,
                event[m_event_index]
            );
            m_event_index++;
            return dumpError(report_buffer, report_buffer_index, CHK_PATTERN);
        }

        base_value = (base_value<<1) | (base_value>>31);
        m_event_index++;
    }

    return 0;
}



int
event_sanity_checker::checkPatternToggle
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t base_value    = 0;

    uint32_t *event        = (m_event+8);
    uint64_t  length       = (m_calc_event_size-8);
    m_event_index          = 8;
    uint32_t toggled_value = base_value;


    for(uint32_t i=0; i<length; i++)
    {
        if( event[i] != toggled_value )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                toggled_value,
                event[m_event_index]
            );
            m_event_index++;
            return dumpError(report_buffer, report_buffer_index, CHK_PATTERN);
        }

        toggled_value = ~toggled_value;
        m_event_index++;
    }

    return 0;
}



int
event_sanity_checker::checkPattern
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    switch(m_pattern_mode)
    {
        case PG_PATTERN_INC:
        { return( checkPatternInc(report_buffer, report_buffer_index) ); }
        break;

        case PG_PATTERN_DEC:
        { return( checkPatternDec(report_buffer, report_buffer_index) ); }
        break;

        case PG_PATTERN_SHIFT:
        { return( checkPatternShift(report_buffer, report_buffer_index) ); }
        break;

        case PG_PATTERN_TOGGLE:
        { return( checkPatternToggle(report_buffer, report_buffer_index) ); }
        break;

        default:
        {
            printf("ERROR: specified unknown pattern matching algorithm\n");
            return CHK_PATTERN;
        }
    }

    return 0;
}



int
event_sanity_checker::compareWithReferenceDdlFile
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    int return_value = 0;
    uint32_t *ddl_mapping      = m_ddl->mapping();
    uint64_t  ddl_mapping_size = m_ddl->size();

    if( ((uint64_t) m_calc_event_size << 2) != ddl_mapping_size )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: Eventsize %lx does not match "
            "reference DDL file size %lx\n",
            ((uint64_t) m_calc_event_size << 2),
            ddl_mapping_size
        );
        return_value |= dumpError(report_buffer, report_buffer_index, CHK_FILE);
    }

    for(m_event_index = 0; m_event_index<m_calc_event_size; m_event_index++)
    {
        if( m_event[m_event_index] != ddl_mapping[m_event_index] )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                ddl_mapping[m_event_index],
                m_event[m_event_index]
            );
            return_value |= dumpError(report_buffer, report_buffer_index, CHK_FILE);
        }
    }

    return return_value;
}



int
event_sanity_checker::checkEndOfEvent
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    if( (uint32_t) *(m_event + m_calc_event_size) != m_reported_event_size)
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: could not find matching reported event size "
            "at Event[%d] expected %08x found %08x\n",
            m_event_index,
            m_calc_event_size,
            (uint32_t) * (m_event + m_event_index)
        );
        return dumpError(report_buffer, report_buffer_index, CHK_EOE);
    }
    return 0;
}



int
event_sanity_checker::checkForLostEvents
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index,
             int64_t                   last_id
)
{
    uint64_t cur_event_id = getEventIdFromCdh(dwordOffset(report_buffer));

    if
    (
        (last_id != -1) &&
        ((cur_event_id & 0xfffffffff) != ((last_id + 1) & 0xfffffffff))
    )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, current ID: %ld\n",
            m_channel_id, last_id, cur_event_id
        );
        return dumpError(report_buffer, report_buffer_index, CHK_ID);
    }
    return 0;
}



uint32_t
event_sanity_checker::reportedEventSize
(volatile librorc_event_descriptor *report_buffer)
{
    /** upper two bits are reserved for flags */
    return(report_buffer->reported_event_size & 0x3fffffff);
}



uint32_t
event_sanity_checker::calculatedEventSize
(volatile librorc_event_descriptor *report_buffer)
{
    /** upper two bits are reserved for flags */
    return(report_buffer->calc_event_size & 0x3fffffff);
}



uint32_t*
event_sanity_checker::rawEventPointer(volatile librorc_event_descriptor *report_buffer)
{
    return (uint32_t*)&m_raw_event_buffer[dwordOffset(report_buffer)];
}



uint64_t
event_sanity_checker::dwordOffset(volatile librorc_event_descriptor *report_buffer)
{
    return(report_buffer->offset / 4);
}



uint64_t
event_sanity_checker::getEventIdFromCdh(uint64_t offset)
{

    uint64_t cur_event_id = (uint32_t) * (m_raw_event_buffer + offset + 2) & 0x00ffffff;
    cur_event_id <<= 12;
    cur_event_id |= (uint32_t) * (m_raw_event_buffer + offset + 1) & 0x00000fff;
    return cur_event_id;
}






}
