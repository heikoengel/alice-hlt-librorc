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

#define LIBRORC_INTERNAL
#include <cstdio>
#include "dma_handling.hh"

using namespace std;


DMAOptions
evaluateArguments(int argc, char *argv[])
{
    DMAOptions ret;
    memset(&ret, 0, sizeof(DMAOptions));

    ret.deviceId  = -1;
    ret.channelId = -1;
    ret.eventSize = 0;
    ret.useRefFile = false;
    ret.loadFcfMappingRam = false;

    /** command line arguments */
    static struct option long_options[] =
    {
        {"device"    , required_argument, 0, 'd'},
        {"channel"   , required_argument, 0, 'c'},
        {"file"      , required_argument, 0, 'f'},
        {"size"      , required_argument, 0, 's'},
        {"source"    , required_argument, 0, 'r'},
        {"fcfmapping", required_argument, 0, 'm'},
        {"help"      , no_argument      , 0, 'h'},
        {0, 0, 0, 0}
    };

    /** Figure out what app this is (pg or ddl)*/
    char *token = strtok (argv[0], "/");
    char *app_name = token;
    while(token != NULL)
    {
        app_name = token;
        token = strtok (NULL, "/");
    }

    argv[0] = app_name;

    /** dma_in defaults **/
    ret.esType = librorc::kEventStreamToHost;
    ret.datasource = ES_SRC_NONE;

    if( 0 == strncmp(app_name, "dma_out", strlen("dma_out")) )
    {
        ret.esType = librorc::kEventStreamToDevice;
        ret.datasource = ES_SRC_NONE;
    }

    /** Parse command line arguments **/
    while(1)
    {
        int opt = getopt_long(argc, argv, "", long_options, NULL);
        if ( opt == -1 )
        { break; }

        switch(opt)
        {
            case 'd':
            {
                ret.deviceId = strtol(optarg, NULL, 0);
            }
            break;

            case 'c':
            {
                ret.channelId = strtol(optarg, NULL, 0);
            }
            break;

            case 'f':
            {
                strcpy(ret.refname, optarg);
                ret.useRefFile = true;
            }
            break;

            case 'r':
            {
                if( 0 == strcmp(optarg, "pg") )
                { ret.datasource = ES_SRC_HWPG; break; }
                else if( 0 == strcmp(optarg, "diu") &&
                        ret.esType!=librorc::kEventStreamToDevice )
                { ret.datasource = ES_SRC_DIU; break; }
                else if( 0 == strcmp(optarg, "siu") &&
                        ret.esType!=librorc::kEventStreamToDevice )
                { ret.datasource = ES_SRC_SIU; break; }
                else if( 0 == strcmp(optarg, "ddr3") &&
                        ret.esType!=librorc::kEventStreamToDevice)
                { ret.datasource = ES_SRC_DDR3; break; }
                else if( 0 == strcmp(optarg, "none") )
                { ret.datasource = ES_SRC_NONE; break; }
                else if( 0 == strcmp(optarg, "dma") &&
                        ret.esType!=librorc::kEventStreamToHost)
                { ret.datasource = ES_SRC_DMA; break; }
                else if( 0 == strcmp(optarg, "raw") &&
                        ret.esType!=librorc::kEventStreamToDevice)
                { ret.datasource = ES_SRC_RAW; break; }
                else
                {
                    cout << "Invalid data source selected!"
                        << endl
                        << HELP_TEXT
                        << endl;
                    exit(0);
                }
            }
            break;

            case 's':
            {
                ret.eventSize = strtol(optarg, NULL, 0);
            }
            break;

            case 'm':
            {
                if( ret.esType!=librorc::kEventStreamToHost )
                {
                    cout << "FCF is not available for this datastream"
                         << endl;
                    exit(0);
                }
                strcpy(ret.fcfmappingfile, optarg);
                ret.loadFcfMappingRam = true;
            }
            break;

            case 'h':
            {
                printf(HELP_TEXT, app_name, app_name);
                exit(0);
            }
            break;

            default:
            {
                break;
            }
        }
    }

    return ret;
}



bool checkDeviceID(int32_t deviceID, char *argv)
{
    if( (deviceID<0) || (deviceID>255) )
    {
        cout << "DeviceId invalid or not set: " << deviceID << endl;
        printf(HELP_TEXT, argv, argv);
        return false;
    }
    return true;
}



bool checkChannelID(int32_t channelID, char *argv)
{
    if( (channelID<0) || (channelID>MAX_CHANNEL) )
    {
        cout << "ChannelId invalid or not set: " << channelID << endl;
        printf(HELP_TEXT, argv, argv);
        return false;
    }
    return true;
}



bool checkEventSize(uint32_t eventSize, char *argv)
{
    if( eventSize == 0 )
    {
        cout << "EventSize invalid or not set: 0x" << hex
        << eventSize << endl;
        printf(HELP_TEXT, argv, argv);
        return false;
    }
    return true;
}



librorc::high_level_event_stream *
prepareEventStream
(
    DMAOptions opts
)
{
    librorc::high_level_event_stream *hlEventStream = NULL;

    try
    { hlEventStream = new librorc::high_level_event_stream(opts.deviceId, opts.channelId, opts.esType); }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream." << endl;
        return(NULL);
    }

    if( opts.esType==librorc::kEventStreamToDevice && opts.datasource != ES_SRC_NONE)
    {
        /** override for max read request size to 128B for Supermicro */
        if( hlEventStream->overridePciePacketSize(128) )
        {
            cout << "Failed to override PCIe packet size" << endl;
            return NULL;
        }
    }

    if( hlEventStream->initializeDma(2*opts.channelId, EBUFSIZE) )
    { return NULL; }

    return(hlEventStream);
}



librorc::high_level_event_stream *
prepareEventStream
(
    librorc::device *dev,
    librorc::bar *bar,
    DMAOptions opts
)
{
    librorc::high_level_event_stream *hlEventStream = NULL;

    try
    { hlEventStream = new librorc::high_level_event_stream(dev, bar, opts.channelId, opts.esType); }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream: " << error << endl;
        return(NULL);
    }

    if( opts.esType==librorc::kEventStreamToDevice && opts.datasource != ES_SRC_NONE)
    {
        /** override for max read request size to 128B for Supermicro */
        if( hlEventStream->overridePciePacketSize(128) )
        {
            cout << "Failed to override PCIe packet size" << endl;
            return NULL;
        }
    }

    if( hlEventStream->initializeDma(2*opts.channelId, EBUFSIZE) )
    { return NULL; }

    return(hlEventStream);
}



uint64_t
eventCallBack
(
    void                     *userdata,
    librorc::EventDescriptor  report,
    const uint32_t           *event,
    librorc::ChannelStatus   *channel_status
)
{
    librorc::event_sanity_checker *checker = (librorc::event_sanity_checker*)userdata;

    try{ checker->check(report, channel_status); }
    catch(...){ abort(); }
    return 0;
}



uint64_t
printStatusLine
(
    timeval                 last_time,
    timeval                 current_time,
    librorc::ChannelStatus *channel_status,
    uint64_t                last_events_received,
    uint64_t                last_bytes_received
)
{
    if(librorc::gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
    {
        printf
        (
            "CH%d Events IN: %10ld, Size: %8.3f GB ",
            channel_status->channel,
            channel_status->n_events,
            (double)channel_status->bytes_received/(double)(1<<30)
        );

        if(channel_status->bytes_received - last_bytes_received)
        {
            printf
            (
                " Data Rate: %9.3f MB/s",
                (double)(channel_status->bytes_received - last_bytes_received)/
                librorc::gettimeofdayDiff(last_time, current_time)/(double)(1<<20)
            );
        }
        else
        { printf(" Data Rate: -"); }

        if(channel_status->n_events - last_events_received)
        {
            printf
            (
                " EventRate: %9.3f kHz",
                (double)(channel_status->n_events - last_events_received)/
                librorc::gettimeofdayDiff(last_time, current_time)/1000.0
            );
        }
        else
        { printf(" ( - )"); }

        printf(" Errors: %ld\n", channel_status->error_count);
    }

    return 0;
}



void
printFinalStatusLine
(
    librorc::ChannelStatus *chstats,
    DMAOptions              opts,
    timeval                 start_time,
    timeval                 end_time
)
{
    printf
    (
        "%ld Byte / %ld events in %.2f sec -> %.1f MB/s.\n",
        chstats->bytes_received,
        chstats->n_events,
        librorc::gettimeofdayDiff(start_time, end_time),
        ((float)chstats->bytes_received/librorc::gettimeofdayDiff(start_time, end_time))/(float)(1<<20)
    );

    if(!chstats->set_offset_count)
    { printf("CH%d: No Events\n", opts.channelId); }
    else
    {
        printf
        (
            "CH%d: Events %ld, max_epi=%ld, min_epi=%ld, avg_epi=%ld, set_offset_count=%ld\n",
            opts.channelId,
            chstats->n_events,
            chstats->max_epi,
            chstats->min_epi,
            chstats->n_events/chstats->set_offset_count,
            chstats->set_offset_count
        );
    }

}


void
configurePatternGenerator
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
    )
{
    librorc::patterngenerator *pg =
        hlEventStream->getPatternGenerator();
    if( pg )
    {
        pg->configureMode(
                PG_PATTERN_INC, // patternMode
                0x00000000,     // initialPattern
                0);             // numberOfEvents -> continuous
        pg->resetEventId();
        pg->setStaticEventSize(opts.eventSize);
        pg->useAsDataSource();
        //pg->setWaitTime(0);
        pg->enable();
    }
    else
    {
        cout << "PatternGenerator not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete pg;
}


void
unconfigurePatternGenerator
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
    )
{
    librorc::patterngenerator *pg =
        hlEventStream->getPatternGenerator();
    if( pg )
    {
        pg->disable();
    }
    else
    {
        cout << "PatternGenerator not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete pg;
}


void
configureDiu
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::diu *diu =
        hlEventStream->getDiu();
    if( diu )
    {
        diu->useAsDataSource();
        diu->setEnable(1);
        if(opts.datasource==ES_SRC_SIU)
        {
            if( diu->prepareForSiuData() < 0 )
            {
                cout << "Failed to enable DIU->SIU chain!";
            }
        }
        else
        {
            if( diu->prepareForDiuData() < 0 )
            {
                cout << "Failed to enable DIU->SIU chain!";
            }
        }
    }
    else
    {
        cout << "DIU not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete diu;
}


void
unconfigureDiu
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
    )
{
    librorc::diu *diu =
        hlEventStream->getDiu();
    if( diu )
    {
        if(opts.datasource==ES_SRC_SIU && diu->linkUp())
        { diu->sendFeeEndOfBlockTransferCmd(); }
        diu->setEnable(0);
    }
    else
    {
        cout << "DIU not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete diu;
}


void
configureRawReadout
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::ddl *rawddl =
        hlEventStream->getRawReadout();
    if( rawddl )
    {
        rawddl->setEnable(1);
    }
    else
    {
        cout << "Raw readout not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete rawddl;
}


void
unconfigureRawReadout
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::ddl *rawddl =
        hlEventStream->getRawReadout();
    if( rawddl )
    {
        rawddl->setEnable(0);
    }
    else
    {
        cout << "Raw readout not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete rawddl;
}



void
configureSiu
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::siu *siu =
        hlEventStream->getSiu();
    if( siu )
    {
        siu->setReset(0);
        siu->setEnable(1);
    }
    else
    {
        cout << "SIU not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete siu;
}


void
unconfigureSiu
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::siu *siu =
        hlEventStream->getSiu();
    if( siu )
    {
        /** TODO: send EOBTR? */
        siu->setEnable(0);
    }
    else
    {
        cout << "SIU not available for this FW/Channel!"
             << endl;
        abort();
    }
    delete siu;
}


/**
 * configure FCF if available for current hlEventStream
 **/
void
configureFcf
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::fastclusterfinder *fcf =
        hlEventStream->getFastClusterFinder();
    if( fcf )
    {
        fcf->setReset(1);
        fcf->setEnable(0);
        fcf->setBypass(0);
        fcf->clearErrors();
#ifndef MODELSIM
        if( opts.loadFcfMappingRam )
        { fcf->loadMappingRam(opts.fcfmappingfile); }
#endif
        fcf->setSinglePadSuppression(0);
        fcf->setBypassMerger(0);
        fcf->setDeconvPad(1);
        fcf->setSingleSeqLimit(0);
        fcf->setClusterLowerLimit(10);
        fcf->setMergerDistance(4);
        fcf->setMergerAlgorithm(1);
        fcf->setChargeTolerance(0);

        fcf->setReset(0);
        fcf->setEnable(1);

        delete fcf;
    }
}

void
unconfigureFcf
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    librorc::fastclusterfinder *fcf =
        hlEventStream->getFastClusterFinder();
    if( fcf )
    {
        fcf->setReset(1);
        fcf->setEnable(0);
        delete fcf;
    }
}



void
configureDataSource
    (
        librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    hlEventStream->m_link->waitForGTXDomain();
    hlEventStream->m_link->setFlowControlEnable(1);
    hlEventStream->m_link->setChannelActive(1);

    if( opts.esType==librorc::kEventStreamToDevice && opts.datasource != ES_SRC_NONE)
    {
        configureSiu(hlEventStream, opts);
    }

    /** configure FCF if available */
    configureFcf(hlEventStream, opts);

    switch(opts.datasource)
    {
        case ES_SRC_HWPG:
        { configurePatternGenerator(hlEventStream, opts); }
        break;

        case ES_SRC_SIU:
        case ES_SRC_DIU:
        { configureDiu(hlEventStream, opts); }
        break;

        case ES_SRC_DMA:
        { hlEventStream->m_link->setDefaultDataSource(); }
        break;

        case ES_SRC_RAW:
        { configureRawReadout(hlEventStream, opts); }
        break;

        case ES_SRC_DDR3:
        { hlEventStream->m_link->setDataSourceDdr3DataReplay(); }
        break;

        default: // "none" or invalid or unspecified
        break;
    }
}

void
unconfigureDataSource
(
    librorc::high_level_event_stream *hlEventStream,
    DMAOptions opts
)
{
    if( opts.esType==librorc::kEventStreamToHost )
    { hlEventStream->m_link->setFlowControlEnable(0); }

    hlEventStream->m_link->setChannelActive(0);

    unconfigureFcf(hlEventStream, opts);

    switch(opts.datasource)
    {
        case ES_SRC_HWPG:
        { unconfigurePatternGenerator(hlEventStream, opts); }
        break;

        case ES_SRC_DIU:
        case ES_SRC_SIU:
        { unconfigureDiu(hlEventStream, opts); }
        break;

        case ES_SRC_RAW:
            {
                unconfigureRawReadout(hlEventStream, opts);
            }
            break;

        default: // "none" or invalid or unspecified
        break;
    }    
    
    if( opts.esType==librorc::kEventStreamToDevice && opts.datasource != ES_SRC_NONE)
    { unconfigureSiu(hlEventStream, opts); }

}
