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
    char *last_token = token;
    while(token != NULL)
    {
        printf ("%s\n", token);
        last_token = token;
        token = strtok (NULL, "/");
    }

    ret.esType = LIBRORC_ES_PURE;
    if( 0 == strcmp(last_token, "pg_dma_continuous") )
    {
        ret.esType = LIBRORC_ES_PG;
    }

    if( 0 == strcmp(last_token, "ddl_dma_continuous") )
    {
        ret.esType = LIBRORC_ES_DDL;
    }

    if(ret.esType == LIBRORC_ES_PURE)
    {
        return ret;
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

            case 's':
            {
                ret.eventSize = strtol(optarg, NULL, 0);
            }
            break;

            case 'h':
            {
                printf(HELP_TEXT, last_token, last_token);
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


//TODO: that should be a part of the dma channel class
librorcChannelStatus*
prepareSharedMemory
(
    DMAOptions opts
)
{
    librorcChannelStatus *chstats = NULL;

    /** allocate shared mem */
    int shID =
        shmget(SHM_KEY_OFFSET + opts.deviceId*SHM_DEV_OFFSET + opts.channelId,
            sizeof(librorcChannelStatus), IPC_CREAT | 0666);
    if(shID==-1)
    {
        perror("Shared memory getching failed!");
        return(chstats);
    }

    /** attach to shared memory */
    char *shm = (char*)shmat(shID, 0, 0);
    if(shm==(char*)-1)
    {
        perror("Attaching of shared memory failed");
        return(chstats);
    }

    chstats = (librorcChannelStatus*)shm;

    /** Wipe SHM */
    memset(chstats, 0, sizeof(librorcChannelStatus));
    chstats->index = 0;
    chstats->last_id = 0xfffffffff;
    chstats->channel = (unsigned int)opts.channelId;

    return(chstats);
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



DDLRefFile
getDDLReferenceFile
(
    DMAOptions opts
)
{
    DDLRefFile ret;
    ret.size = 0;
    ret.fd   = 0;
    ret.map  = NULL;

    /** get optional DDL reference file */
    struct stat  ddlref_stat;
    if(opts.useRefFile==true)
    {
        ret.fd = open(opts.refname, O_RDONLY);
        if(ret.fd<0)
        {
            perror("failed to open reference DDL file");
            abort();
        }

        if(fstat(ret.fd, &ddlref_stat)==-1)
        {
            perror("fstat DDL reference file");
            abort();
        }

        ret.size = ddlref_stat.st_size;
        ret.map =
            (uint32_t *)
                mmap(0, ret.size, PROT_READ, MAP_SHARED, ret.fd, 0);
        if(ret.map == MAP_FAILED)
        {
            perror("failed to mmap file");
            abort();
        }
    }

    return(ret);
}



void
deleteDDLReferenceFile
(
    DDLRefFile ddlref
)
{
    if(ddlref.map != NULL)
    {
        if( munmap(ddlref.map, ddlref.size)==-1 )
        { perror("ERROR: failed to unmap file"); }
    }

    if(ddlref.fd >= 0)
    { close(ddlref.fd); }
}



timeval
printStatusLine
(
    timeval               last_time,
    timeval               cur_time,
    librorcChannelStatus *chstats,
    uint64_t             *last_events_received,
    uint64_t             *last_bytes_received
)
{
    if(gettimeofdayDiff(last_time, cur_time)>STAT_INTERVAL)
    {
        printf
        (
            "Events: %10ld, DataSize: %8.3f GB ",
            chstats->n_events,
            (double)chstats->bytes_received/(double)(1<<30)
        );

        if(chstats->bytes_received - *last_bytes_received)
        {
            printf
            (
                " DataRate: %9.3f MB/s",
                (double)(chstats->bytes_received - *last_bytes_received)/
                gettimeofdayDiff(last_time, cur_time)/(double)(1<<20)
            );
        }
        else
        { printf(" DataRate: -"); }

        if(chstats->n_events - *last_events_received)
        {
            printf
            (
                " EventRate: %9.3f kHz/s",
                (double)(chstats->n_events - *last_events_received)/
                gettimeofdayDiff(last_time, cur_time)/1000.0
            );
        }
        else
        { printf(" EventRate: -"); }

        printf(" Errors: %ld\n", chstats->error_count);
        last_time = cur_time;
        *last_bytes_received  = chstats->bytes_received;
        *last_events_received = chstats->n_events;
    }

    return last_time;
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
