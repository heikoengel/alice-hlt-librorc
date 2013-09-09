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
                printf(HELP_TEXT, argv[0]);
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
        printf(HELP_TEXT, argv);
        return false;
    }
    return true;
}



bool checkChannelID(int32_t channelID, char *argv)
{
    if( (channelID<0) || (channelID>MAX_CHANNEL) )
    {
        cout << "ChannelId invalid or not set: " << channelID << endl;
        printf(HELP_TEXT, argv);
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
        printf(HELP_TEXT, argv);
        return false;
    }
    return true;
}


//TODO: that should be a part of the dma channel class
channelStatus*
prepareSharedMemory
(
    DMAOptions opts
)
{
    channelStatus *chstats = NULL;

    /** allocate shared mem */
    int shID =
        shmget(SHM_KEY_OFFSET + opts.deviceId*SHM_DEV_OFFSET + opts.channelId,
            sizeof(channelStatus), IPC_CREAT | 0666);
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

    chstats = (channelStatus*)shm;

    /** Wipe SHM */
    memset(chstats, 0, sizeof(channelStatus));
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
    { eventStream = new librorc::event_stream(opts.deviceId, opts.channelId); }
    catch( int error )
    {
        switch(error)
        {
            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DEVICE_FAILED:
            { cout << "ERROR: failed to initialize device." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BAR_FAILED:
            { cout << "ERROR: failed to initialize BAR1." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_BUFFER_FAILED:
            { cout << "ERROR: failed to allocate buffer." << endl; }
            break;

            case LIBRORC_EVENT_STREAM_ERROR_CONSTRUCTOR_DCHANNEL_FAILED:
            { cout << "ERROR: failed to allocate DMA-channel." << endl; }
            break;
        }
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
    if(ddlref.map)
    {
        if( munmap(ddlref.map, ddlref.size)==-1 )
        { perror("ERROR: failed to unmap file"); }
    }

    if(ddlref.fd>=0)
    { close(ddlref.fd); }
}



/**
 * gettimeofday_diff
 * @param time1 earlier timestamp
 * @param time2 later timestamp
 * @return time difference in seconds as double
 * */
double
gettimeofdayDiff
(
    timeval time1,
    timeval time2
)
{
    timeval diff;
    diff.tv_sec = time2.tv_sec - time1.tv_sec;
    diff.tv_usec = time2.tv_usec - time1.tv_usec;
    while(diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec -= 1;
    }

    return (double)((double)diff.tv_sec + (double)((double)diff.tv_usec / 1000000));
}



timeval
printStatusLine
(
    timeval        last_time,
    timeval        cur_time,
    channelStatus *chstats,
    uint64_t      *last_events_received,
    uint64_t      *last_bytes_received
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
    channelStatus *chstats,
    DMAOptions     opts,
    timeval        start_time,
    timeval        end_time
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
printDeviceStatus
(
    librorc::event_stream *eventStream
)
{
    printf
    (
        "Bus %x, Slot %x, Func %x\n",
        eventStream->m_dev->getBus(),
        eventStream->m_dev->getSlot(),
        eventStream->m_dev->getFunc()
    );

    try
    {
        librorc::sysmon *sm = new librorc::sysmon(eventStream->m_bar1);
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setw(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setw(8) << sm->FwBuildDate() << dec << endl;
        delete sm;
    }
    catch(...)
    { cout << "Firmware Rev. and Date not available!" << endl; }
}
