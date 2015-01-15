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

#ifndef DMA_HANDLING_H
#define DMA_HANDLING_H

#include <sys/shm.h>
#include <signal.h>
#include <getopt.h>
#include <cstdio>
#include <librorc.h>

#define ES_SRC_NONE 0
#define ES_SRC_HWPG 1
#define ES_SRC_DIU 2
#define ES_SRC_DMA 3
#define ES_SRC_DDR3 4
#define ES_SRC_RAW 5
#define ES_SRC_SIU 6

/** Help text, that is displayed when -h is used */
#define HELP_TEXT "%s usage:                                          \n\
        %s [parameters]                                               \n\
parameters:                                                           \n\
        --device [0..255]       Source device ID                      \n\
        --channel [0..11]       Source DMA channel                    \n\
        --source [name]         use specific datasource.              \n\
                                available options:                    \n\
                                none,pg,ddr3,diu,dma,raw              \n\
        --size [value]          PatternGenerator event size in DWs    \n\
        --fcfmapping [filename] File to be loaded as FCF mapping file \n\
        --file [filename]       DDL reference file                    \n\
        --help                  Show this text                        \n"

#define DMA_ABORT_HANDLER                                            \
librorc::high_level_event_stream *hlEventStream = NULL;              \
void abort_handler( int s )                                          \
{                                                                    \
    printf("Caught signal %d\n", s);                                 \
    if( hlEventStream->m_done==true )                                \
    { exit(-1); }                                                    \
    else                                                             \
    { hlEventStream->m_done = true; }                                \
}

#define DMA_ABORT_HANDLER_REGISTER struct sigaction sigIntHandler;   \
sigIntHandler.sa_handler = abort_handler;                            \
sigemptyset(&sigIntHandler.sa_mask);                                 \
sigIntHandler.sa_flags = 0;                                          \
sigaction(SIGINT, &sigIntHandler, NULL);

/** maximum channel number allowed **/
#define MAX_CHANNEL 11

/** Buffer Sizes (in Bytes) **/
#ifndef MODELSIM
    #define EBUFSIZE (((uint64_t)1) << 28)
#else
    #define EBUFSIZE (((uint64_t)1) << 19)
#endif

/** Struct to store command line parameters */
typedef struct
{
    int32_t       deviceId;
    int32_t       channelId;
    uint32_t      eventSize;
    char          refname[4096];
    char          fcfmappingfile[4096];
    uint32_t      datasource;
    bool          useRefFile;
    bool          loadFcfMappingRam;
    librorc::EventStreamDirection esType;
} DMAOptions;



DMAOptions evaluateArguments(int argc, char *argv[]);
bool checkDeviceID(int32_t deviceID, char *argv);
bool checkChannelID(int32_t channelID, char *argv);
bool checkEventSize(uint32_t eventSize, char *argv);

librorc::high_level_event_stream *prepareEventStream(DMAOptions opts);
#ifdef LIBRORC_INTERNAL
librorc::high_level_event_stream *prepareEventStream(librorc::device *dev, librorc::bar *bar, DMAOptions opts);
#endif

uint64_t
eventCallBack
(
    void                     *userdata,
    librorc::EventDescriptor  report,
    const uint32_t           *event,
    librorc::ChannelStatus    *channel_status
);

uint64_t
printStatusLine
(
    timeval                 last_time,
    timeval                 cur_time,
    librorc::ChannelStatus *chstats,
    uint64_t                last_events_received,
    uint64_t                last_bytes_received
);

void
printFinalStatusLine
(
    librorc::ChannelStatus *chstats,
    DMAOptions              opts,
    timeval                 start_time,
    timeval                 end_time
);


void
configureDataSource
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
);

void
unconfigureDataSource
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
);
#endif /** DMA_HANDLING_H */
