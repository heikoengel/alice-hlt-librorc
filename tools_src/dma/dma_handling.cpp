#define LIBRORC_INTERNAL
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
    ret.useFcf = false;

    /** command line arguments */
    static struct option long_options[] =
    {
        {"device", required_argument, 0, 'd'},
        {"channel", required_argument, 0, 'c'},
        {"file", required_argument, 0, 'f'},
        {"size", required_argument, 0, 's'},
        {"source", required_argument, 0, 'r'},
        {"fcf", no_argument, 0, 'F'},
        {"help", no_argument, 0, 'h'},
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
    ret.esType = LIBRORC_ES_TO_HOST;
    ret.datasource = ES_SRC_NONE;

    if( 0 == strcmp(app_name, "dma_out") )
    {
        ret.esType = LIBRORC_ES_TO_DEVICE;
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
                        ret.esType!=LIBRORC_ES_TO_DEVICE )
                { ret.datasource = ES_SRC_DIU; break; }
                else if( 0 == strcmp(optarg, "ddr3") &&
                        ret.esType!=LIBRORC_ES_TO_DEVICE)
                { ret.datasource = ES_SRC_DDR3; break; }
                else if( 0 == strcmp(optarg, "none") )
                { ret.datasource = ES_SRC_NONE; break; }
                else if( 0 == strcmp(optarg, "dma") &&
                        ret.esType!=LIBRORC_ES_TO_HOST)
                { ret.datasource = ES_SRC_DMA; break; }
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

            case 'F':
            {
                if( ret.esType!=LIBRORC_ES_TO_HOST )
                {
                    cout << "FCF is not available for this datastream"
                         << endl;
                    exit(0);
                }
                ret.useFcf = true;
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



librorc::event_stream *
prepareEventStream
(
    DMAOptions opts
)
{
    librorc::event_stream *eventStream = NULL;

    try
    { eventStream = new librorc::event_stream(opts.deviceId,
            opts.channelId, opts.esType); }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream." << endl;
        return(NULL);
    }

    eventStream->clearSharedMemory();

    return(eventStream);
}



librorc::event_stream *
prepareEventStream
(
    librorc::device *dev,
    librorc::bar *bar,
    DMAOptions opts
)
{
    librorc::event_stream *eventStream = NULL;

    try
    { eventStream = new librorc::event_stream(dev, bar,
            opts.channelId, opts.esType); }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream." << endl;
        return(NULL);
    }
    eventStream->clearSharedMemory();

    return(eventStream);
}



uint64_t
eventCallBack
(
    void                     *userdata,
    librorc_event_descriptor  report,
    const uint32_t           *event,
    librorcChannelStatus     *channel_status
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
    timeval               last_time,
    timeval               current_time,
    librorcChannelStatus *channel_status,
    uint64_t              last_events_received,
    uint64_t              last_bytes_received
)
{
    if(gettimeofdayDiff(last_time, current_time)>STAT_INTERVAL)
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
                gettimeofdayDiff(last_time, current_time)/(double)(1<<20)
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
                gettimeofdayDiff(last_time, current_time)/1000.0
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
    librorcChannelStatus *chstats,
    DMAOptions            opts,
    timeval               start_time,
    timeval               end_time
)
{
    printf
    (
        "%ld Byte / %ld events in %.2f sec -> %.1f MB/s.\n",
        chstats->bytes_received,
        chstats->n_events,
        gettimeofdayDiff(start_time, end_time),
        ((float)chstats->bytes_received/gettimeofdayDiff(start_time, end_time))/(float)(1<<20)
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
    librorc::event_stream *eventStream,
    DMAOptions opts
    )
{
    librorc::patterngenerator *pg =
        eventStream->getPatternGenerator();
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
    librorc::event_stream *eventStream,
    DMAOptions opts
    )
{
    librorc::patterngenerator *pg =
        eventStream->getPatternGenerator();
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
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{
    librorc::diu *diu =
        eventStream->getDiu();
    if( diu )
    {
        diu->useAsDataSource();
        diu->enableInterface();
        diu->prepareForFeeData();
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
    librorc::event_stream *eventStream,
    DMAOptions opts
    )
{
    librorc::diu *diu =
        eventStream->getDiu();
    if( diu )
    {
        if(diu->linkUp())
        { diu->sendFeeEndOfBlockTransferCmd(); }
        diu->disableInterface();
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
configureSiu
(
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{
    librorc::siu *siu =
        eventStream->getSiu();
    if( siu )
    {
        siu->setReset(0);
        siu->enableInterface();
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
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{
    librorc::siu *siu =
        eventStream->getSiu();
    if( siu )
    {
        siu->disableInterface();
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
configureFcf
(
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{
    librorc::fastclusterfinder *fcf =
        eventStream->getFastClusterFinder();
    if( !fcf )
    {
        cout << "Clusterfinder not available for this FW/Channel!"
             << endl;
        abort();
    }

    fcf->setState(1, 0); // reset, not enabled
    // TODO: load mapping file
    fcf->setSinglePadSuppression(0);
    fcf->setBypassMerger(0);
    fcf->setDeconvPad(1);
    fcf->setSingleSeqLimit(0);
    fcf->setClusterLowerLimit(10);
    fcf->setMergerDistance(4);
    fcf->setMergerAlgorithm(1);
    fcf->setChargeTolerance(0);

    fcf->setState(0, 1);// not reset, enabled

    delete fcf;
}

void
unconfigureFcf
(
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{
    librorc::fastclusterfinder *fcf =
        eventStream->getFastClusterFinder();
    if( !fcf )
    {
        cout << "Clusterfinder not available for this FW/Channel!"
             << endl;
        abort();
    }

    fcf->setState(1, 0); // reset, not enabled
    delete fcf;
}



void
configureDataSource
(
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{
    eventStream->m_link->waitForGTXDomain();
    eventStream->m_link->enableFlowControl();
    if( opts.esType==LIBRORC_ES_TO_DEVICE &&
            opts.datasource != ES_SRC_NONE)
    {
        //configureSiu(eventStream, opts);
    }

    if( opts.useFcf )
    {
        configureFcf(eventStream, opts);
    }

    switch(opts.datasource)
    {
        case ES_SRC_HWPG:
            {
                configurePatternGenerator(eventStream, opts);
            }
            break;

        case ES_SRC_DIU:
            {
                configureDiu(eventStream, opts);
            }
            break;

        case ES_SRC_DMA:
            {
                eventStream->m_link->setDefaultDataSource();
            }
            break;

        case ES_SRC_DDR3:
            {
                eventStream->m_link->setDataSourceDdr3DataReplay();
                /*if(eventStream->m_link->ddr3DataReplayChannelIsInReset())
                {
                    eventStream->m_link->disableDdr3DataReplayChannel();
                    eventStream->m_link->setDdr3DataReplayChannelReset(0);
                }*/
            }
            break;

        default: // "none" or invalid or unspecified
            break;
    }
}

void
unconfigureDataSource
(
    librorc::event_stream *eventStream,
    DMAOptions opts
)
{

    if( opts.useFcf )
    {
        unconfigureFcf(eventStream, opts);
    }

    switch(opts.datasource)
    {
        case ES_SRC_HWPG:
            {
                unconfigurePatternGenerator(eventStream, opts);
            }
            break;

        case ES_SRC_DIU:
            {
                unconfigureDiu(eventStream, opts);
            }
            break;

        default: // "none" or invalid or unspecified
            break;
    }

    if( opts.esType==LIBRORC_ES_TO_DEVICE &&
            opts.datasource != ES_SRC_NONE)
    {
        //unconfigureSiu(eventStream, opts);
    }
}
