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
sigIntHandler.sa_flags = 0;

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
        int64_t  last_id;
        uint32_t channel;
    }channelStatus;

#endif /** EH_LEGACY */

/** struct to store command line parameters */
typedef struct
{
    int32_t   deviceId;
    int32_t   channelId;
    uint32_t  eventSize;
    char      refname[4096];
    bool      useRefFile;
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

channelStatus *prepareSharedMemory(DMAOptions opts);
DDLRefFile getDDLReferenceFile(DMAOptions opts);
void deleteDDLReferenceFile(DDLRefFile ddlref);

timeval
printStatusLine
(
    timeval        last_time,
    timeval        cur_time,
    channelStatus *chstats,
    uint64_t      *last_events_received,
    uint64_t      *last_bytes_received
);

#endif /** DMA_HANDLING_H */
