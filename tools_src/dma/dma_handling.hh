#ifndef DMA_HANDLING_H
#define DMA_HANDLING_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>

#include <librorc.h>

/** Help text, that is displayed when -h is used */
#define HELP_TEXT "%s usage:                                 \n\
        %s [parameters]                                      \n\
parameters:                                                  \n\
        --device [0..255] Source device ID                   \n\
        --channel [0..11] Source DMA channel                 \n\
        --size [value]    PatternGenerator event size in DWs \n\
        --file [filename] DDL reference file                 \n\
        --help            Show this text                     \n"

#define DMA_ABORT_HANDLER bool done = 0; \
void abort_handler( int s )              \
{                                        \
    printf("Caught signal %d\n", s);     \
    if( done==true )                     \
    { exit(-1); }                        \
    else                                 \
    { done = true; }                     \
}

#define DMA_ABORT_HANDLER_REGISTER struct sigaction sigIntHandler; \
sigIntHandler.sa_handler = abort_handler;                          \
sigemptyset(&sigIntHandler.sa_mask);                               \
sigIntHandler.sa_flags = 0;                                        \
sigaction(SIGINT, &sigIntHandler, NULL);

/** maximum channel number allowed **/
#define MAX_CHANNEL 11



#ifndef EH_LEGACY
#define EH_LEGACY

    /** Shared memory parameters for the DMA monitor **/
    #define SHM_KEY_OFFSET 2048
    #define SHM_DEV_OFFSET 32

    /** struct to store statistics on received data for a single channel **/
    typedef struct
    {
        uint64_t n_events;
        uint64_t bytes_received;
        uint64_t min_epi;
        uint64_t max_epi;
        uint64_t index;
        uint64_t set_offset_count;
        uint64_t error_count;
        uint64_t last_id;
        uint32_t channel;
    }channelStatus;

#endif /** EH_LEGACY */


/** Struct to store command line parameters */
typedef struct
{
    int32_t       deviceId;
    int32_t       channelId;
    uint32_t      eventSize;
    char          refname[4096];
    bool          useRefFile;
    LibrorcEsType esType;
} DMAOptions;

/** Struct to hanlde DDL refernce file */
typedef struct
{
    uint64_t  size;
    int       fd;
    uint32_t *map;
} DDLRefFile;


DMAOptions evaluateArguments(int argc, char *argv[]);
bool checkDeviceID(int32_t deviceID, char *argv);
bool checkChannelID(int32_t channelID, char *argv);
bool checkEventSize(uint32_t eventSize, char *argv);

librorcChannelStatus *prepareSharedMemory(DMAOptions opts);
librorc::event_stream *prepareEventStream(DMAOptions opts);
DDLRefFile getDDLReferenceFile(DMAOptions opts);
void deleteDDLReferenceFile(DDLRefFile ddlref);

timeval
printStatusLine
(
    timeval               last_time,
    timeval               cur_time,
    librorcChannelStatus *chstats,
    uint64_t             *last_events_received,
    uint64_t             *last_bytes_received
);

void
printFinalStatusLine
(
    librorcChannelStatus *chstats,
    DMAOptions            opts,
    timeval               start_time,
    timeval               end_time
);

///////////////////////////////////////////////////////////////////////////////////

/** sanity checks **/
#define CHK_SIZES   (1<<0)
#define CHK_PATTERN (1<<1)
#define CHK_SOE     (1<<2)
#define CHK_EOE     (1<<3)
#define CHK_ID      (1<<4)
#define CHK_FILE    (1<<8)

/** Pattern Generator Mode: Ramp **/
#define PG_PATTERN_RAMP (1<<0)

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
             m_eventbuffer        = eventbuffer;
             m_channel_id         = channel_id;
             m_pattern_mode       = pattern_mode;
             m_check_mask         = check_mask;
             m_ddl_reference      = ddl_reference;
             m_ddl_reference_size = ddl_reference_size;
             m_event_index        = 0;
         };

        ~event_sanity_checker(){};


        uint64_t
        eventSanityCheck
        (
            volatile librorc_event_descriptor *reportbuffer,
                     uint64_t                  report_buffer_index,
                     int64_t                   last_id
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
                 uint32_t  m_event_index;

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
        checkPatternRamp
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
        rawEventPointer(volatile librorc_event_descriptor* report_buffer);

        uint64_t
        dwordOffset(volatile librorc_event_descriptor* report_buffer);

        /**
         *  Get EventID from CDH:
         *  lower 12 bits in CHD[1][11:0]
         *  upper 24 bits in CDH[2][23:0]
         */
        uint64_t
        getEventIdFromCdh(uint64_t offset);

};

#endif /** DMA_HANDLING_H */
