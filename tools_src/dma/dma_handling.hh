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
        pgdma_continuous [parameters]                        \n\
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

/** struct to store command line parameters */
typedef struct
{
    int32_t       deviceId;
    int32_t       channelId;
    uint32_t      eventSize;
    char          refname[4096];
    bool          useRefFile;
    LibrorcEsType esType;
} DMAOptions;

/** sruct to hanlde DDL refernce file */
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

void printDeviceStatus(librorc::event_stream *eventStream);

#endif /** DMA_HANDLING_H */
