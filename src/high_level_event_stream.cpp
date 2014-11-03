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
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#define LIBRORC_INTERNAL
#include <librorc/high_level_event_stream.hh>
#include <librorc/event_stream.hh>
#include <librorc/device.hh>
#include <librorc/bar.hh>
#include <librorc/sysmon.hh>


namespace LIBRARY_NAME
{
    high_level_event_stream::high_level_event_stream
    (
        int32_t       deviceId,
        int32_t       channelId,
        LibrorcEsType esType
    ) : event_stream( deviceId, channelId, esType )
    {
        m_done             = false;
        m_event_callback   = NULL;
        m_status_callback  = NULL;
    }

    high_level_event_stream::high_level_event_stream
    (
        device *dev,
        bar    *bar,
        int32_t       channelId,
        LibrorcEsType esType
    ) : event_stream( dev, bar, channelId, esType )
    {
        m_done             = false;
        m_event_callback   = NULL;
        m_status_callback  = NULL;
    }

    high_level_event_stream::~high_level_event_stream(){};

    void
    high_level_event_stream::printDeviceStatus()
    {
        printf("EventBuffer size: 0x%lx bytes\n", m_eventBuffer->getPhysicalSize());
        printf("ReportBuffer size: 0x%lx bytes\n", m_reportBuffer->getPhysicalSize());
        printf("Bus %x, Slot %x, Func %x\n", m_dev->getBus(), m_dev->getSlot(), m_dev->getFunc() );

        try
        {
            std::cout << "CRORC FPGA" << std::endl
                 << "Firmware Rev. : " << std::hex << std::setw(8)
                 << m_sm->FwRevision()  << std::dec << std::endl
                 << "Firmware Date : " << std::hex << std::setw(8)
                 << m_sm->FwBuildDate() << std::dec << std::endl;
        }
        catch(...)
        { std::cout << "Firmware Rev. and Date not available!" << std::endl; }
    }

    uint64_t
    high_level_event_stream::eventLoop(void *user_data)
    {
        m_last_bytes_received  = 0;
        m_last_events_received = 0;

        /** Capture starting time */
        m_bar1->gettime(&m_start_time, 0);
        m_last_time     = m_start_time;
        m_current_time  = m_start_time;

        uint64_t result = 0;
        while( !m_done )
        {
            m_bar1->gettime(&m_current_time, 0);

            result = handleChannelData(user_data);

            if(gettimeofdayDiff(m_last_time, m_current_time)>STAT_INTERVAL)
            {
                m_status_callback
                ? m_status_callback
                  (
                      m_last_time,
                      m_current_time,
                      m_channel_status,
                      m_last_events_received,
                      m_last_bytes_received
                  ) : 0;

                m_last_bytes_received  = m_channel_status->bytes_received;
                m_last_events_received = m_channel_status->n_events;
                m_last_time = m_current_time;
            }

            if(result == 0)
            { usleep(200); } /** no events available */

        }

        m_bar1->gettime(&m_end_time, 0);

        return result;
    }

    uint64_t
    high_level_event_stream::dwordOffset(EventDescriptor report_entry)
    {
        return(report_entry.offset / 4);
    }



    uint64_t
    high_level_event_stream::getEventIdFromCdh(uint64_t offset)
    {
        // TODO: no event/offset size check here - prone to segmentation faults!
        uint64_t cur_event_id = (uint32_t) * (m_raw_event_buffer + offset + 2) & 0x00ffffff;
        cur_event_id <<= 12;
        cur_event_id |= (uint32_t) * (m_raw_event_buffer + offset + 1) & 0x00000fff;
        return cur_event_id;
    }

    uint64_t
    high_level_event_stream::handleEvent
    (
        uint64_t         events_processed,
        void            *user_data,
        EventDescriptor *report,
        const uint32_t  *event,
        uint64_t        *events_per_iteration
    )
    {

        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "New RB Entry: offset=0x%lx, calcSize=0x%x, repSize=0x%x\n",
                report->offset, report->calc_event_size, report->reported_event_size);

        events_processed++;

        if( m_event_callback != NULL )
        { m_event_callback(user_data, *report, event, m_channel_status); }

        updateChannelStatus(&m_reports[m_channel_status->index]);
        *events_per_iteration = *events_per_iteration + 1;
        DEBUG_PRINTF
        (
            PDADEBUG_CONTROL_FLOW,
            "CH %d - Event, %d DWs\n",
            m_channel_status->channel,
            report->calc_event_size
        );

        return events_processed;
    }

    uint64_t
    high_level_event_stream::handleChannelData(void *user_data)
    {
        uint64_t         events_processed     = 0;
        EventDescriptor *report               = NULL;
        const uint32_t  *event                = 0;
        uint64_t         events_per_iteration = 0;
        uint64_t         reference            = 0;
        uint64_t         init_reference       = 0;

        /** New event(s) received */
        if( getNextEvent(&report, &event, &init_reference) )
        {
            events_processed =
                handleEvent
                (
                    events_processed,
                    user_data,
                    report,
                    event,
                    &events_per_iteration
                );

            /** handle all following entries */
            while( getNextEvent(&report, &event, &reference) )
            {
                events_processed =
                    handleEvent
                    (
                        events_processed,
                        user_data,
                        report,
                        event,
                        &events_per_iteration
                    );

                releaseEvent(reference);
            }

            releaseEvent(init_reference);

            if(events_per_iteration > m_channel_status->max_epi)
            { m_channel_status->max_epi = events_per_iteration; }

            if(events_per_iteration < m_channel_status->min_epi)
            { m_channel_status->min_epi = events_per_iteration; }

            events_per_iteration = 0;
            m_channel_status->set_offset_count++;
        }

        return events_processed;
    }

    const uint32_t*
    high_level_event_stream::getRawEvent(EventDescriptor report)
    {
        return (const uint32_t*)&m_raw_event_buffer[report.offset/4];
    }
}
