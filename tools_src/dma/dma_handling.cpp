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

    /** command line arguments */
    static struct option long_options[] =
    {
        {"device", required_argument, 0, 'd'},
        {"channel", required_argument, 0, 'c'},
        {"file", required_argument, 0, 'f'},
        {"size", required_argument, 0, 's'},
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

    ret.esType = LIBRORC_ES_PURE;
    if( 0 == strcmp(app_name, "dma_in_pg") )
    { ret.esType = LIBRORC_ES_PG; }

    if( 0 == strcmp(app_name, "dma_in_ddl") )
    { ret.esType = LIBRORC_ES_DDL; }

    if(ret.esType == LIBRORC_ES_PURE)
    { return ret; }

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

            case 's':
            {
                ret.eventSize = strtol(optarg, NULL, 0);
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
    { eventStream = new librorc::event_stream(opts.deviceId, opts.channelId, opts.eventSize, opts.esType); }
    catch( int error )
    {
        cout << "ERROR: failed to initialize event stream." << endl;
        return(NULL);
    }

    return(eventStream);
}



void
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
            "Events: %10ld, DataSize: %8.3f GB ",
            channel_status->n_events,
            (double)channel_status->bytes_received/(double)(1<<30)
        );

        if(channel_status->bytes_received - last_bytes_received)
        {
            printf
            (
                " DataRate: %9.3f MB/s",
                (double)(channel_status->bytes_received - last_bytes_received)/
                gettimeofdayDiff(last_time, current_time)/(double)(1<<20)
            );
        }
        else
        { printf(" DataRate: -"); }

        if(channel_status->n_events - last_events_received)
        {
            printf
            (
                " EventRate: %9.3f kHz/s",
                (double)(channel_status->n_events - last_events_received)/
                gettimeofdayDiff(last_time, current_time)/1000.0
            );
        }
        else
        { printf(" EventRate: -"); }

        printf(" Errors: %ld\n", channel_status->error_count);
    }
}



void
printFinalStatusLine
(
    librorcChannelStatus *chstats,
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
    { printf("CH%d: No Events\n", chstats->channel); }
    else
    {
        printf
        (
            "CH%d: Events %ld, max_epi=%ld, min_epi=%ld, avg_epi=%ld, set_offset_count=%ld\n",
            chstats->channel,
            chstats->n_events,
            chstats->max_epi,
            chstats->min_epi,
            chstats->n_events/chstats->set_offset_count,
            chstats->set_offset_count
        );
    }

}
