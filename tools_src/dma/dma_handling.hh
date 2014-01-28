#ifndef DMA_HANDLING_H
#define DMA_HANDLING_H

#include <sys/shm.h>
#include <signal.h>
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

#define DMA_ABORT_HANDLER librorc::event_stream *eventStream = NULL; \
void abort_handler( int s )                                          \
{                                                                    \
    printf("Caught signal %d\n", s);                                 \
    if( eventStream->m_done==true )                                  \
    { exit(-1); }                                                    \
    else                                                             \
    { eventStream->m_done = true; }                                  \
}

#define DMA_ABORT_HANDLER_REGISTER struct sigaction sigIntHandler;   \
sigIntHandler.sa_handler = abort_handler;                            \
sigemptyset(&sigIntHandler.sa_mask);                                 \
sigIntHandler.sa_flags = 0;                                          \
sigaction(SIGINT, &sigIntHandler, NULL);

/** maximum channel number allowed **/
#define MAX_CHANNEL 11

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



DMAOptions evaluateArguments(int argc, char *argv[]);
bool checkDeviceID(int32_t deviceID, char *argv);
bool checkChannelID(int32_t channelID, char *argv);
bool checkEventSize(uint32_t eventSize, char *argv);

librorc::event_stream *prepareEventStream(DMAOptions opts);
#ifdef LIBRORC_INTERNAL
librorc::event_stream *prepareEventStream(librorc::device *dev, librorc::bar *bar, DMAOptions opts);
#endif

uint64_t
eventCallBack
(
    void                     *userdata,
    uint64_t                  event_id,
    librorc_event_descriptor  report,
    const uint32_t           *event,
    librorcChannelStatus     *channel_status
);

uint64_t
printStatusLine
(
    timeval               last_time,
    timeval               cur_time,
    librorcChannelStatus *chstats,
    uint64_t              last_events_received,
    uint64_t              last_bytes_received
);

void
printFinalStatusLine
(
    librorcChannelStatus *chstats,
    DMAOptions            opts,
    timeval               start_time,
    timeval               end_time
);

#endif /** DMA_HANDLING_H */
