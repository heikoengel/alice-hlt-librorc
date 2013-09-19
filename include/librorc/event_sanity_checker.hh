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


#ifndef LIBRORC_EVENT_SANITY_CHECKER_H
#define LIBRORC_EVENT_SANITY_CHECKER_H

#include <librorc/include_ext.hh>
#include <librorc/defines.hh>
#include <librorc/buffer.hh>
#include <librorc/dma_channel.hh>

/** limit the number of corrupted events to be written to disk **/
#define MAX_FILES_TO_DISK 100

/** sanity checks **/
#define CHK_SIZES   (1<<0)
#define CHK_PATTERN (1<<1)
#define CHK_SOE     (1<<2)
#define CHK_EOE     (1<<3)
#define CHK_ID      (1<<4)
#define CHK_FILE    (1<<8)

/** TODO :   Konfiguration muss dann auch in dma_channel_pg::configurePatternGenerator mit rein. */
/*
 *  The base value of the pattern generator is set to the register RORC_REG_DDL_PG_PATTERN.
 *  This register defaults to zero if not set.
 *
 *  Pattern Generator Modes :
 */
#define PG_PATTERN_INC    (1<<0) /** Increment value by 1 */
#define PG_PATTERN_DEC    (1<<1) /** Decrement value by 1 */
#define PG_PATTERN_SHIFT  (1<<2) /** Shifts the value to the left, the leftmost bit is inserted on the right side */
#define PG_PATTERN_TOGGLE (1<<3) /** Toggles between the value and the negated value : 0x000000A5 -> 0xffffff5A */

namespace LIBRARY_NAME
{
    class event_sanity_checker
    {
        public:
             event_sanity_checker(){};
             event_sanity_checker
             (
                 volatile uint32_t *eventbuffer,
                 uint32_t           channel_id,
                 uint32_t           pattern_mode,
                 uint32_t           check_mask,
                 uint32_t          *ddl_reference,
                 uint64_t           ddl_reference_size
             )
             {
                 m_eventbuffer         = eventbuffer;
                 m_channel_id          = channel_id;
                 m_pattern_mode        = pattern_mode;
                 m_check_mask          = check_mask;
                 m_ddl_reference       = ddl_reference;
                 m_ddl_reference_size  = ddl_reference_size;
                 m_event_index         = 0;
                 m_event               = NULL;
                 m_reported_event_size = 0;
             };

            ~event_sanity_checker(){};

            uint64_t
            check
            (
                volatile librorc_event_descriptor *report_buffer,
                         librorcChannelStatus     *channel_status
            );

            /**
             * Dump Event to console
             * @param pointer to eventbuffer
             * @param offset of the current event within the eventbuffer
             * @param size in DWs of the event
             * */
            void
            dumpEvent
            (
                volatile uint32_t *event_buffer,
                uint64_t offset,
                uint64_t length
            );

            /**
             * Dump reportbuffer entry to console
             * @param reportbuffer pointer to reportbuffer
             * @param index of current librorc_event_descriptor within reportbuffer
             * @param DMA channel number
             **/
            void
            dumpReportBufferEntry
            (
                volatile librorc_event_descriptor *reportbuffer,
                         uint64_t                  index,
                         uint32_t                  channel_number
            );

        protected:
            volatile uint32_t *m_eventbuffer;
                     uint32_t  m_channel_id;
                     uint32_t  m_pattern_mode;
                     uint32_t  m_check_mask;
                     uint32_t *m_ddl_reference;
                     uint64_t  m_ddl_reference_size;

                     /** check() portion */
                     uint32_t  m_event_index;
                     uint32_t *m_event;
                     uint32_t  m_reported_event_size;
                     uint32_t  m_calc_event_size;

            int
            dumpError
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index,
                         int32_t                   check_id
            );

            int
            compareCalculatedToReportedEventSizes
            (
                volatile librorc_event_descriptor *reportbuffer,
                         uint64_t                  report_buffer_index
            );

            /** Each event has a CommonDataHeader (CDH) consisting of 8 DWs,
             *  see also http://cds.cern.ch/record/1027339?ln=en
             */
            int
            checkStartOfEvent
            (
                volatile librorc_event_descriptor *reportbuffer,
                         uint64_t                  report_buffer_index
            );

            int
            checkPatternInc
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            int
            checkPatternDec
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            int
            checkPatternShift
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            int
            checkPatternToggle
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            int
            checkPattern
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            int
            compareWithReferenceDdlFile
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            /**
             * 32 bit DMA mode
             *
             * DMA data is written in multiples of 32 bit. A 32bit EOE word
             * is directly attached after the last event data word.
             * The EOE word contains the EOE status word received from the DIU
            **/
            int
            checkEndOfEvent
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index
            );

            /**
             * Make sure EventIDs increment with each event.
             * missing EventIDs are an indication of lost event data
             */
            int
            checkForLostEvents
            (
                volatile librorc_event_descriptor *report_buffer,
                         uint64_t                  report_buffer_index,
                         int64_t                   last_id
            );

            uint32_t
            reportedEventSize(volatile librorc_event_descriptor *report_buffer);

            uint32_t
            calculatedEventSize(volatile librorc_event_descriptor *report_buffer);

            uint32_t*
            rawEventPointer(volatile librorc_event_descriptor *report_buffer);

            uint64_t
            dwordOffset(volatile librorc_event_descriptor *report_buffer);

            /**
             *  Get EventID from CDH:
             *  lower 12 bits in CHD[1][11:0]
             *  upper 24 bits in CDH[2][23:0]
             */
            uint64_t
            getEventIdFromCdh(uint64_t offset);

    };
}

#endif /** LIBRORC_EVENT_SANITY_CHECKER_H */
