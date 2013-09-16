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


namespace librorc
{



uint64_t
event_sanity_checker::check
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index,
             int64_t                   last_id
)
{
    m_event_index = 0;
    int retval    = 0;

    // copyEventToLocal

//    uint32_t *event
//        = (uint32_t*)malloc((reportedEventSize(report_buffer)+4)*sizeof(uint32_t));
//    if( event==NULL )
//    {
//        perror("Malloc EB");
//        return 0;
//    }
//
//    memcpy(event, (uint8_t *)m_eventbuffer + report_buffer->offset, (reportedEventSize(report_buffer)+4)*sizeof(uint32_t));

//    retval |= !(m_check_mask & CHK_SIZES)   ? 0 : compareCalculatedToReportedEventSizes(report_buffer, report_buffer_index);
    retval |= !(m_check_mask & CHK_SOE)     ? 0 : checkStartOfEvent(report_buffer, report_buffer_index);
    retval |= !(m_check_mask & CHK_PATTERN) ? 0 : checkPattern(report_buffer, report_buffer_index);
    retval |= !(m_check_mask & CHK_FILE)    ? 0 : compareWithReferenceDdlFile(report_buffer, report_buffer_index);
    retval |= !(m_check_mask & CHK_EOE)     ? 0 : checkEndOfEvent(report_buffer, report_buffer_index);
    retval |= !(m_check_mask & CHK_ID)      ? 0 : checkForLostEvents(report_buffer, report_buffer_index, last_id);

    if(retval != 0)
    { throw retval; }

//    free(event);

    return getEventIdFromCdh(dwordOffset(report_buffer));
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
    uint32_t reported_event_size = reportedEventSize(report_buffer);
    uint64_t offset              = (report_buffer->offset / 4);

    dumpEvent(m_eventbuffer, offset, reported_event_size);
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
    uint32_t reported_event_size = reportedEventSize(report_buffer);
    uint32_t calc_event_size     = calculatedEventSize(report_buffer);

    /** Bit 31 of calc_event_size is read completion timeout flag */
    uint32_t timeout_flag = (report_buffer->calc_event_size>>31);

    if(timeout_flag)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
                m_channel_id, report_buffer_index);
        abort();
        return(CHK_SIZES);
    }
    else if (calc_event_size != reported_event_size)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                        "calculated: 0x%x, reported: 0x%x\n"
                        "offset=0x%lx, rbdm_offset=0x%lx\n", m_channel_id,
                report_buffer_index, calc_event_size, reported_event_size,
                report_buffer->offset,
                report_buffer_index * sizeof(librorc_event_descriptor));
        abort();
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
    uint32_t *event = rawEventPointer(report_buffer);

    if ((uint32_t) * (event) != 0xffffffff)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
                        "offset=%ld, rbdm_offset=%ld\n", report_buffer_index,
                (uint32_t) * (event), report_buffer->offset,
                report_buffer_index * sizeof(librorc_event_descriptor));

        abort();
        return dumpError(report_buffer, report_buffer_index, CHK_SOE);
    }

    return 0;
}



int
event_sanity_checker::checkPatternRamp
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t *event           = rawEventPointer(report_buffer);
    uint32_t  calc_event_size = calculatedEventSize(report_buffer);

    for (m_event_index=8; m_event_index<calc_event_size; m_event_index++)
    {
        if ((uint32_t) * (event + m_event_index) != (m_event_index - 8))
        {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                    report_buffer_index, m_event_index, (m_event_index - 8),
                    (uint32_t)
                            * (m_eventbuffer + dwordOffset(report_buffer)
                                    + m_event_index));
            abort();
            return dumpError(report_buffer, report_buffer_index, CHK_PATTERN);
        }
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
        case PG_PATTERN_RAMP:
        { return( checkPatternRamp(report_buffer, report_buffer_index) ); }
        break;

        default:
        {
            printf("ERROR: specified unknown pattern matching algorithm\n");
            return CHK_PATTERN;
        }
    }

//    Hier die Liste aller momentan in der FW implementierten Pattern:
//    inc    : increment value by 1 (wird bei uns momentan PG_RAMP genannt)
//    dec    : decrement value by 1
//    shift  : shifts the value to the left, the leftmost bit is
//             inserted on the right side
//    toggle : toggles between the value and the negated value : 0x000000A5
//    -> 0xffffff5A
//    Basis ist jeweils der Wert, der in RORC_REG_DDL_PG_PATTERN geschrieben
//    wird (bei uns momentan unbenutzt, daher 0x00000000). Diese
//    Konfiguration muss dann auch in
//    dma_channel_pg::configurePatternGenerator mit rein.

    return 0;
}



int
event_sanity_checker::compareWithReferenceDdlFile
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    int       retval          = 0;
    uint32_t *event           = rawEventPointer(report_buffer);
    uint32_t  calc_event_size = calculatedEventSize(report_buffer);

    if( ((uint64_t) calc_event_size << 2) != m_ddl_reference_size )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: Eventsize %lx does not match "
            "reference DDL file size %lx\n",
            ((uint64_t) calc_event_size << 2),
            m_ddl_reference_size
        );

        abort();
        retval |= dumpError(report_buffer, report_buffer_index, CHK_FILE);
    }

    for(m_event_index = 0; m_event_index<calc_event_size; m_event_index++)
    {
        if( event[m_event_index] != m_ddl_reference[m_event_index] )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                m_ddl_reference[m_event_index],
                event[m_event_index]
            );

            abort();
            retval |= dumpError(report_buffer, report_buffer_index, CHK_FILE);
        }
    }

    return retval;
}



int
event_sanity_checker::checkEndOfEvent
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t *event              = rawEventPointer(report_buffer);
    uint32_t calc_event_size     = calculatedEventSize(report_buffer);
    uint32_t reported_event_size = reportedEventSize(report_buffer);

    if( (uint32_t) *(event + calc_event_size) != reported_event_size)
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: could not find matching reported event size "
            "at Event[%d] expected %08x found %08x\n",
            m_event_index,
            calc_event_size,
            (uint32_t) * (event + m_event_index)
        );

        abort();
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
    uint64_t offset       = dwordOffset(report_buffer);
    uint64_t cur_event_id = getEventIdFromCdh(offset);

    if
    (
        ((last_id != -1) && (cur_event_id & 0xfffffffff))
        !=
        ((last_id + 1) & 0xfffffffff)
    )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
                        "current ID: %ld\n", m_channel_id, last_id,
                cur_event_id);

        abort();
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
    //return m_eventbuffer + (uint32_t*)report_buffer->offset;

    return (uint32_t*)&m_eventbuffer[dwordOffset(report_buffer)];
}



uint64_t
event_sanity_checker::dwordOffset(volatile librorc_event_descriptor *report_buffer)
{
    return(report_buffer->offset / 4);
}



uint64_t
event_sanity_checker::getEventIdFromCdh(uint64_t offset)
{

    uint64_t cur_event_id = (uint32_t) * (m_eventbuffer + offset + 2) & 0x00ffffff;
    cur_event_id <<= 12;
    cur_event_id |= (uint32_t) * (m_eventbuffer + offset + 1) & 0x00000fff;
    return cur_event_id;
}



}
