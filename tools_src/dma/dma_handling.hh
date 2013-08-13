#ifndef DMA_HANDLING_H
#define DMA_HANDLING_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>

#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <getopt.h>

/** Buffer Sizes (in Bytes) **/
#ifndef SIM
    #define EBUFSIZE (((uint64_t)1) << 28)
    #define RBUFSIZE (((uint64_t)1) << 26)
    #define STAT_INTERVAL 1.0
#else
    #define EBUFSIZE (((uint64_t)1) << 19)
    #define RBUFSIZE (((uint64_t)1) << 17)
    #define STAT_INTERVAL 0.00001
#endif

#define HELP_TEXT "%s usage:                                 \n\
        pgdma_continuous [parameters]                        \n\
parameters:                                                  \n\
        --device [0..255] Source device ID                   \n\
        --channel [0..11] Source DMA channel                 \n\
        --size [value]    PatternGenerator event size in DWs \n\
        --file [filename] DDL reference file                 \n\
        --help            Show this text                     \n"

/** maximum channel number allowed **/
#define MAX_CHANNEL 11

typedef struct
{
    int32_t   deviceId;
    int32_t   channelId;
    uint32_t  eventSize;
    char      refname[4096];
    bool      useRefFile;
} DMAOptions;


DMAOptions evaluateArguments(int argc, char *argv[]);
bool checkDeviceID(int32_t deviceID, char *argv);
bool checkChannelID(int32_t channelID, char *argv);
bool checkEventSize(uint32_t eventSize, char *argv);

#endif /** DMA_HANDLING_H */
