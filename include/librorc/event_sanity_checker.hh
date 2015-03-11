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
 **/


#ifndef LIBRORC_EVENT_SANITY_CHECKER_H
#define LIBRORC_EVENT_SANITY_CHECKER_H

#include <librorc/defines.hh>
#include <librorc/event_stream.hh>

/** limit the number of corrupted events to be written to disk **/
#define MAX_FILES_TO_DISK 100

/** sanity checks **/
#define CHK_SIZES   (1<<0)
#define CHK_PATTERN (1<<1)
#define CHK_SOE     (1<<2)
#define CHK_EOE     (1<<3)
#define CHK_ID      (1<<4)
#define CHK_DIU_ERR (1<<5)
#define CHK_FILE    (1<<8)
#define CHK_CMPL    (1<<9)

/** TODO :   Konfiguration muss dann auch in dma_channel_pg::configurePatternGenerator mit rein. */
/*
 *  The base value of the pattern generator is set to the register RORC_REG_DDL_PG_PATTERN.
 *  This register defaults to zero if not set.
 *
 *  Pattern Generator Modes :
 */
#define PG_PATTERN_INC    0 /** Increment value by 1 */
#define PG_PATTERN_DEC    2 /** Decrement value by 1 */
#define PG_PATTERN_SHIFT  1 /** Shifts the value to the left, the leftmost bit is inserted on the right side */
#define PG_PATTERN_TOGGLE 3 /** Toggles between the value and the negated value : 0x000000A5 -> 0xffffff5A */

namespace LIBRARY_NAME
{

#define EVENT_ID_UNDEFINED 0xfffffffffffffffful

class buffer;
class ddl_reference_file;

    class event_sanity_checker
    {
        public:
             event_sanity_checker(){};
             event_sanity_checker
             (
                 buffer            *event_buffer,
                 uint32_t           channel_id,
                 uint32_t           check_mask,
                 char              *log_base_dir
             );

             event_sanity_checker
             (
                 buffer            *event_buffer,
                 uint32_t           channel_id,
                 uint32_t           check_mask,
                 char              *log_base_dir,
                 char              *ddl_reference_file_path
             );

            ~event_sanity_checker();

            int
            check
            (
                EventDescriptor  report,
                ChannelStatus   *channel_status,
                uint64_t         event_id
            );

            uint64_t
            check
            (
                EventDescriptor  report,
                ChannelStatus   *channel_status
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
             * @param index of current EventDescriptor within reportbuffer
             * @param DMA channel number
             **/
            void
            dumpReportBufferEntry
            (
                volatile EventDescriptor *reportbuffer,
                         uint64_t         index,
                         uint32_t         channel_number
            );

        protected:
            volatile uint32_t           *m_raw_event_buffer;
                     buffer             *m_event_buffer;
                     uint32_t            m_channel_id;
                     uint32_t            m_check_mask;
                     ddl_reference_file *m_ddl;
                     char               *m_log_base_dir;

                     /** check() portion */
                     uint32_t *m_event;
                     uint32_t  m_reported_event_size;
                     uint32_t  m_calc_event_size;
                     uint32_t  m_error_flag;
                     uint32_t  m_comletion_status;
                     uint64_t  m_last_event_id;

            int
            dumpError
            (
                volatile EventDescriptor *report_buffer,
                         uint64_t         report_buffer_index,
                         int32_t          check_id
            );

            int
            compareCalculatedToReportedEventSizes
            (
                volatile EventDescriptor *reportbuffer,
                         uint64_t         report_buffer_index
            );

            /** Each event has a CommonDataHeader (CDH) consisting of 8 DWs,
             *  see also http://cds.cern.ch/record/1027339?ln=en
             */
            int
            checkStartOfEvent
            (
                volatile EventDescriptor *reportbuffer,
                         uint64_t         report_buffer_index
            );

            /**
             * compute next expected word from pattern generator based on
             * current word and pattern generator mode
             * @param cur_word current/last word
             * @return next word
             **/
            uint32_t
            nextPgWord
            (
                uint32_t mode,
                uint32_t cur_word
            );

            int
            checkPattern
            (
                volatile EventDescriptor *report_buffer,
                         uint64_t         report_buffer_index
            );


            int
            compareWithReferenceDdlFile
            (
                volatile EventDescriptor *report_buffer,
                         uint64_t         report_buffer_index
            );

            int
            checkDiuError
            (
                         uint64_t         report_buffer_index
            );

            int
            checkCompletion
            (
                         uint64_t         report_buffer_index
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
                volatile EventDescriptor *report_buffer,
                         uint64_t         report_buffer_index
            );

            /**
             * Make sure EventIDs increment with each event.
             * missing EventIDs are an indication of lost event data
             */
            int
            checkForLostEvents
            (
                volatile EventDescriptor *report_buffer,
                         uint64_t         report_buffer_index
            );

            uint32_t
            reportedEventSize(volatile EventDescriptor *report_buffer);

            uint32_t
            errorFlag(volatile EventDescriptor *report_buffer);

            uint32_t
            completionStatus(volatile EventDescriptor *report_buffer);

            uint32_t
            calculatedEventSize(volatile EventDescriptor *report_buffer);

            uint32_t*
            rawEventPointer(volatile EventDescriptor *report_buffer);

            uint64_t
            dwordOffset(volatile EventDescriptor *report_buffer);

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
