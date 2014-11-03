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

             file_dumper(char *base_dir)
             {
                 m_base_dir = base_dir;
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
                ChannelStatus   *channel_status,
                uint64_t         event_id,
                uint64_t         last_event_id,
                buffer          *event_buffer,
                uint32_t         error_bit_mask,
                EventDescriptor  report
            )
            {
                if (channel_status->error_count < MAX_FILES_TO_DISK)
                {
                    uint32_t file_index = channel_status->error_count;
                    m_raw_event_buffer  = (uint32_t *)event_buffer->getMem();

                    openFiles(file_index, channel_status);
                    dumpReportBufferEntryToLog(event_id, last_event_id, channel_status, report);
                    dumpErrorTypeToLog(error_bit_mask);

                    //TODO : review this bool (dump event if something fails)?
                    bool
                    dump_event  =
                          calculatedIsLargerThanPhysical(report, event_buffer)
                        ? dumpCalculatedIsLargerThanPhysicalToLog(report, event_buffer)
                        : true;

                    dump_event &=
                          offsetIsLargerThanPhysical(report, event_buffer)
                        ? dumpOffsetIsLargerThanPhysicalToLog(report, event_buffer)
                        : true;

                    dump_event ? dumpEventToLog(error_bit_mask, report) : (void)0;

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

            void
            openFiles
            (
                uint32_t       file_index,
                ChannelStatus *channel_status
            )
            {
                int lengthOfDestinationFilePath =
                        snprintf(NULL, 0, "%s/dev%d_ch%d_%d.ddl", 
                                m_base_dir, channel_status->device, channel_status->channel, file_index);

                if (lengthOfDestinationFilePath < 0)
                { throw LIBRORC_FILE_DUMPER_ERROR_FILE_OPEN_FAILED; }

                snprintf(m_ddl_file_name, lengthOfDestinationFilePath+1, "%s/dev%d_ch%d_%d.ddl",
                         m_base_dir, channel_status->device, channel_status->channel, file_index);
                snprintf(m_log_file_name, lengthOfDestinationFilePath+1, "%s/dev%d_ch%d_%d.log",
                         m_base_dir, channel_status->device, channel_status->channel, file_index);

                m_fd_ddl = fopen(m_ddl_file_name, "w");
                if(m_fd_ddl == NULL)
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
                uint64_t         event_id,
                uint64_t         last_event_id,
                ChannelStatus   *channel_status,
                EventDescriptor  report
            )
            {
                fprintf
                (
                    m_fd_log,
                    "CH%2d - RB[%3ld]: \ncalc_size=%08x\n"
                    "reported_size=%08x\noffset=%lx\nEventID=%lx\nLastID=%lx\n",
                    channel_status->channel,
                    channel_status->index,
                    report.calc_event_size,
                    report.reported_event_size,
                    report.offset,
                    event_id,
                    last_event_id
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
                    !(error_bit_mask & CHK_DIU_ERR) ? 0 : fprintf(m_fd_log, " CHK_DIU_ERR");
                    !(error_bit_mask & CHK_CMPL)    ? 0 : fprintf(m_fd_log, " CHK_CMPL");
                }
                fprintf(m_fd_log, "\n\n");
            }

            bool
            calculatedIsLargerThanPhysical
            (
                EventDescriptor  report,
                buffer          *event_buffer
            )
            {
                return (report.calc_event_size & 0x3fffffff) > (event_buffer->getPhysicalSize() >> 2);
            }

            bool
            dumpCalculatedIsLargerThanPhysicalToLog
            (
                EventDescriptor  report,
                buffer           *event_buffer
            )
            {
                fprintf
                (
                    m_fd_log,
                    "calc_event_size (0x%x DWs) is larger"
                    " than physical buffer size (0x%lx DWs) - not dumping event.\n",
                    report.calc_event_size,
                    (event_buffer->getPhysicalSize() >> 2)
                );

                return false;
            }

            bool
            offsetIsLargerThanPhysical
            (
                EventDescriptor  report,
                buffer           *event_buffer
            )
            {
                return (report.offset > event_buffer->getPhysicalSize());
            }


            bool
            dumpOffsetIsLargerThanPhysicalToLog
            (
                EventDescriptor  report,
                buffer           *event_buffer
            )
            {
                fprintf
                (
                    m_fd_log,
                    "offset (0x%lx) is larger than physical buffer"
                    " size (0x%lx) - not dumping event.\n",
                    report.offset,
                    event_buffer->getPhysicalSize()
                );

                return false;
            }

            void
            dumpEventToLog
            (
                uint32_t         error_bit_mask,
                EventDescriptor  report
            )
            {
                uint32_t i;

                for
                (
                    i = 0;
                    i < (report.calc_event_size & 0x3fffffff);
                    i++
                )
                {
                    uint32_t ebword =
                        (uint32_t) *(
                                        m_raw_event_buffer +
                                        (report.offset >> 2) + i
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
                    (uint32_t) *(m_raw_event_buffer + (report.offset >> 2) + i)
                );

                size_t size_written =
                    fwrite
                    (
                            m_raw_event_buffer + (report.offset >> 2),
                            4,
                            (report.calc_event_size & 0x3fffffff),
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
    uint32_t           check_mask,
    char              *log_base_dir
)
{
    m_event_buffer        = event_buffer;
    m_raw_event_buffer    = (uint32_t *)(event_buffer->getMem());
    m_channel_id          = channel_id;
    m_check_mask          = check_mask;
    m_event               = NULL;
    m_reported_event_size = 0;
    m_log_base_dir        = log_base_dir;
    m_last_event_id       = EVENT_ID_UNDEFINED;
    m_ddl                 = NULL;
};



event_sanity_checker::event_sanity_checker
(
    buffer            *event_buffer,
    uint32_t           channel_id,
    uint32_t           check_mask,
    char              *log_base_dir,
    char              *ddl_reference_file_path
)
{
    m_event_buffer        = event_buffer;
    m_raw_event_buffer    = (uint32_t *)(event_buffer->getMem());
    m_channel_id          = channel_id;
    m_check_mask          = check_mask;
    m_event               = NULL;
    m_reported_event_size = 0;
    m_log_base_dir        = log_base_dir;
    m_last_event_id       = EVENT_ID_UNDEFINED;
    m_ddl                 = new ddl_reference_file(ddl_reference_file_path);
};



event_sanity_checker::~event_sanity_checker()
{
    if(m_ddl != NULL)
    { delete m_ddl; }
};



void
event_sanity_checker::check
(
    EventDescriptor  report,
    ChannelStatus   *channel_status,
    uint64_t         event_id
)
{
    uint64_t         report_buffer_index =  channel_status->index;
    EventDescriptor *report_pointer      =  &report;

    m_event               = rawEventPointer(report_pointer);
    m_reported_event_size = reportedEventSize(report_pointer);
    m_calc_event_size     = calculatedEventSize(report_pointer);
    m_error_flag          = errorFlag(report_pointer);
    m_comletion_status    = completionStatus(report_pointer);

    int error_code   = 0;
    error_code |= !(m_check_mask & CHK_DIU_ERR) ? 0 : checkDiuError(report_buffer_index);
    error_code |= !(m_check_mask & CHK_CMPL)    ? 0 : checkCompletion(report_buffer_index);
    error_code |= !(m_check_mask & CHK_SIZES)   ? 0 : compareCalculatedToReportedEventSizes(report_pointer, report_buffer_index);
    error_code |= !(m_check_mask & CHK_SOE)     ? 0 : checkStartOfEvent(report_pointer, report_buffer_index);
    error_code |= !(m_check_mask & CHK_PATTERN) ? 0 : checkPattern(report_pointer, report_buffer_index);
    error_code |= !(m_check_mask & CHK_FILE)    ? 0 : compareWithReferenceDdlFile(report_pointer, report_buffer_index);
    error_code |= !(m_check_mask & CHK_EOE)     ? 0 : checkEndOfEvent(report_pointer, report_buffer_index);
    error_code |= !(m_check_mask & CHK_ID)      ? 0 : checkForLostEvents(report_pointer, report_buffer_index);

    if(error_code != 0)
    {
        channel_status->error_count++;
        dumpError(report_pointer, report_buffer_index, error_code);

        file_dumper dumper(m_log_base_dir);

        dumper.dump
        (
           channel_status,
           event_id,
           m_last_event_id,
           m_event_buffer,
           error_code,
           report
        );
    }
}



uint64_t
event_sanity_checker::check
(
    EventDescriptor  report,
    ChannelStatus   *channel_status
)
{
    uint64_t event_id = getEventIdFromCdh(dwordOffset(&report));
    check(report, channel_status, event_id);
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
#ifdef MODELSIM
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
    volatile EventDescriptor *report_buffer,
             uint64_t         index,
             uint32_t         channel_number
)
{
    DEBUG_PRINTF
    (
        PDADEBUG_CONTROL_FLOW,
        "CH%2d - RB[%3ld]: calc_size=%08x reported_size=%08x offset=%lx E:%d S:%d\n",
        channel_number, index, m_calc_event_size, m_reported_event_size,
        report_buffer->offset, m_error_flag, m_comletion_status
    );
}



/** PROTECTED METHODS*/
int
event_sanity_checker::dumpError
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index,
             int32_t          error_code
)
{
    dumpEvent(m_raw_event_buffer, dwordOffset(report_buffer), m_calc_event_size);
    dumpReportBufferEntry(report_buffer, report_buffer_index, m_channel_id);
    return error_code;
}



int
event_sanity_checker::compareCalculatedToReportedEventSizes
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index
)
{
    if(m_calc_event_size != m_reported_event_size)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                        "calculated: 0x%x, reported: 0x%x\n"
                        "offset=0x%lx, rbdm_offset=0x%lx\n", m_channel_id,
                report_buffer_index, m_calc_event_size, m_reported_event_size,
                report_buffer->offset,
                report_buffer_index * sizeof(EventDescriptor));
        return CHK_SIZES;
    }

    return 0;
}

int
event_sanity_checker::checkDiuError
(
    uint64_t report_buffer_index
)
{
    if( m_error_flag )
    {
        DEBUG_PRINTF( PDADEBUG_ERROR, "CH%2d ERROR: Event[%ld] DIU Error flag set\n",
                m_channel_id, report_buffer_index);
        return CHK_DIU_ERR;
    }
    return 0;
}

int
event_sanity_checker::checkCompletion
(
    uint64_t report_buffer_index
)
{
    if( m_error_flag || m_comletion_status)
    {
        switch(m_comletion_status)
        {
            case 1:
                DEBUG_PRINTF(PDADEBUG_ERROR, "CH%d ERROR: Event[%ld] CMPL Status Unsupported Request\n", m_channel_id, report_buffer_index);
                break;
            case 2:
                DEBUG_PRINTF(PDADEBUG_ERROR, "CH%d ERROR: Event[%ld] CMPL Status Completer Abort\n", m_channel_id, report_buffer_index);
                break;
            case 3:
                DEBUG_PRINTF(PDADEBUG_ERROR, "CH%d ERROR: Event[%ld] CMPL Status unknown/reserved\n", m_channel_id, report_buffer_index);
                break;
            default:
                DEBUG_PRINTF(PDADEBUG_ERROR, "CH%d ERROR: Event[%ld] Completion Timeout\n", m_channel_id, report_buffer_index);
                break;
        }
        return CHK_CMPL;
    }
    return 0;
}




int
event_sanity_checker::checkStartOfEvent
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index
)
{
    if((uint32_t) * (m_event) != 0xffffffff)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
                        "offset=%ld, rbdm_offset=%ld\n", report_buffer_index,
                (uint32_t) * (m_event), report_buffer->offset,
                report_buffer_index * sizeof(EventDescriptor));
        return CHK_SOE;
    }

    return 0;
}



uint32_t
event_sanity_checker::nextPgWord
(
    uint32_t mode,
    uint32_t cur_word
)
{
    switch(mode)
    {
        case PG_PATTERN_INC:
        { return cur_word+1;}
        break;

        case PG_PATTERN_DEC:
        { return cur_word-1; }
        break;

        case PG_PATTERN_SHIFT:
        { return ((cur_word<<1) | (cur_word>>31)); }
        break;

        default: // PG_PATTERN_TOGGLE:
        { return ~cur_word; }
        break;
    }
}


int
event_sanity_checker::checkPattern
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index
)
{
    if( m_calc_event_size<=4 )
    { return CHK_PATTERN; }

    /** get pattern mode from CDH: Word3, Bits [4:3] */
    uint32_t pattern_mode = ((m_event[3]>>3) & 0x03);
    /** get pattern base value from CDH: Word4, Bits [31:0] */
    uint32_t pattern_base = m_event[4];
    /** CDH version from Word1, Bits [31:24] */
    uint32_t cdh_version = m_event[1]>>24;
    uint32_t expected_value = pattern_base;

    /** CDHv3 has 10 header words, any other has 8 */
    uint32_t cdh_count = 8;
    if( cdh_version==3 )
    { cdh_count = 10; }

    if( m_calc_event_size <= cdh_count )
    { return CHK_PATTERN; }

    for(uint32_t i=cdh_count; i<m_calc_event_size; i++)
    {
        if( m_event[i] != expected_value)
        {
            DEBUG_PRINTF( PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index, i,expected_value, m_event[i] );
            return CHK_PATTERN;
        }
        expected_value = nextPgWord(pattern_mode, expected_value);
    }
    return 0;
}



int
event_sanity_checker::compareWithReferenceDdlFile
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index
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
        return_value |= CHK_FILE;
    }

    for(uint64_t index = 0; index<m_calc_event_size; index++)
    {
        if( m_event[index] != ddl_mapping[index] )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                index,
                ddl_mapping[index],
                m_event[index]
            );
            return_value |= CHK_FILE;
        }
    }
    return return_value;
}



int
event_sanity_checker::checkEndOfEvent
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index
)
{
    uint32_t eoeword = (uint32_t) *(m_event + m_calc_event_size);
    uint32_t eoe_length = (eoeword & 0x7fffffff);
    if( eoe_length != m_reported_event_size )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: could not find matching reported event size, "
            "expected %08x, found %08x\n",
            m_reported_event_size,
            eoe_length
        );
        return CHK_EOE;
    }
    return 0;
}



int
event_sanity_checker::checkForLostEvents
(
    volatile EventDescriptor *report_buffer,
             uint64_t         report_buffer_index
)
{
    uint64_t cur_event_id = getEventIdFromCdh(dwordOffset(report_buffer));
    int result = 0;

    if ( (m_last_event_id != EVENT_ID_UNDEFINED) &&
        (cur_event_id != ((m_last_event_id + 1) & 0xffffffffful)) )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, current ID: %ld\n",
            m_channel_id, m_last_event_id, cur_event_id
        );
        result = CHK_ID;
    }
    m_last_event_id = cur_event_id;
    return result;
}


uint32_t
event_sanity_checker::reportedEventSize
(volatile EventDescriptor *report_buffer)
{
    /** upper two bits are reserved for flags */
    return(report_buffer->reported_event_size & 0x3fffffff);
}



uint32_t
event_sanity_checker::calculatedEventSize
(volatile EventDescriptor *report_buffer)
{
    /** upper two bits are reserved for flags */
    return(report_buffer->calc_event_size & 0x3fffffff);
}



uint32_t
event_sanity_checker::errorFlag
(volatile EventDescriptor *report_buffer)
{
    /** bit 30 of reported_event_size */
    return( (report_buffer->reported_event_size>>30)&1 );
}



uint32_t
event_sanity_checker::completionStatus
(volatile EventDescriptor *report_buffer)
{
    /** bits [31:30] of calc_event_size */
    return(report_buffer->calc_event_size>>30);
}



uint32_t*
event_sanity_checker::rawEventPointer(volatile EventDescriptor *report_buffer)
{
    return (uint32_t*)&m_raw_event_buffer[dwordOffset(report_buffer)];
}



uint64_t
event_sanity_checker::dwordOffset(volatile EventDescriptor *report_buffer)
{
    return(report_buffer->offset / 4);
}



uint64_t
event_sanity_checker::getEventIdFromCdh(uint64_t offset)
{
    /** TODO: check event boundaries before accessing memory! */
    uint64_t cur_event_id = (uint32_t) * (m_raw_event_buffer + offset + 2) & 0x00ffffff;
    cur_event_id <<= 12;
    cur_event_id |= (uint32_t) * (m_raw_event_buffer + offset + 1) & 0x00000fff;
    return cur_event_id;
}






}
