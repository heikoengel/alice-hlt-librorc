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
    {
        ret.esType = LIBRORC_ES_PG;
    }

    if( 0 == strcmp(app_name, "dma_in_ddl") )
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



///////////////////////////
int
event_sanity_checker::dumpError
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index,
             int32_t                   check_id
)
{
    uint32_t reported_event_size = reportedEventSize(report_buffer);
    uint64_t offset              = (report_buffer->offset / 4);

    dumpEvent(m_eventbuffer, offset, reported_event_size);
    dumpReportBufferEntry(report_buffer, report_buffer_index, m_channel_id);
    return check_id;
}

uint32_t
event_sanity_checker::reportedEventSize
(volatile librorc_event_descriptor *reportbuffer)
{
    /** upper two bits are reserved for flags */
    return(reportbuffer->reported_event_size & 0x3fffffff);
}

uint32_t
event_sanity_checker::calculatedEventSize
(volatile librorc_event_descriptor *report_buffer)
{
    /** upper two bits are reserved for flags */
    return(report_buffer->calc_event_size & 0x3fffffff);
}

uint32_t*
event_sanity_checker::rawEventPointer(volatile librorc_event_descriptor* reportbuffer)
{
    return (uint32_t*) (m_eventbuffer) + reportbuffer->offset;
}

uint64_t
event_sanity_checker::dwordOffset(volatile librorc_event_descriptor* report_buffer)
{
    return(report_buffer->offset / 4);
}

uint64_t
event_sanity_checker::getEventIdFromCdh(uint64_t tmp_offset)
{

    uint64_t cur_event_id = (uint32_t) * (m_eventbuffer + tmp_offset + 2) & 0x00ffffff;
    cur_event_id <<= 12;
    cur_event_id |= (uint32_t) * (m_eventbuffer + tmp_offset + 1) & 0x00000fff;
    return cur_event_id;
}



int
event_sanity_checker::eventSanityCheck
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index,
             int64_t                   last_id,
             uint64_t                 *event_id  //TODO : simply return this later
)
{
    m_event_index = 0;
    int retval = 0;

    //Slated for removal
    uint32_t *event = rawEventPointer(report_buffer);

    if(m_check_mask & CHK_SIZES)
    {
        retval |= compareCalculatedToReportedEventSizes(report_buffer, report_buffer_index);
    }

    if( (m_check_mask & CHK_SOE)  )
    {
        retval |= checkStartOfEvent(report_buffer, report_buffer_index);
    }

    if( (m_check_mask & CHK_PATTERN) )
    {
        retval |= checkPattern(report_buffer, report_buffer_index);
    }

    if( (m_check_mask & CHK_FILE) )
    {
        retval |= compareWithReferenceDdlFile(report_buffer, report_buffer_index);
    }

    if( (m_check_mask & CHK_EOE) )
    {
        retval |= checkEndOfEvent(event, report_buffer, report_buffer_index);
    }

    if( (m_check_mask & CHK_ID) )
    {
        retval |=
            checkForLostEvents(last_id, report_buffer, report_buffer_index);
    }

    /** return event ID to caller */
    *event_id = getEventIdFromCdh(dwordOffset(report_buffer));

    return retval;
}



void
event_sanity_checker::dumpEvent
(
    volatile uint32_t *eventbuffer,
    uint64_t offset,
    uint64_t len
)
{
#ifdef SIM
    uint64_t i = 0;
    for(i=0;i<len;i++)
    {
        printf("%03ld: %08x\n", i, (uint32_t)*(eventbuffer + offset +i));
    }

    if(len&0x01)
    {
        printf("%03ld: %08x (dummy)\n", i, (uint32_t)*(eventbuffer + offset + i));
        i++;
    }

    printf("%03ld: EOE reported_event_size: %08x\n", i, (uint32_t)*(eventbuffer + offset + i));

    #if DMA_MODE==128
        printf("%03ld: EOE calc_event_size: %08x\n", i+1, (uint32_t)*(eventbuffer + offset + i));
        printf("%03ld: EOE dummy %08x\n", i+2, (uint32_t)*(eventbuffer + offset + i));
        printf("%03ld: EOE dummy: %08x\n", i+3, (uint32_t)*(eventbuffer + offset + i));
    #endif
#endif
}



void
event_sanity_checker::dumpReportBufferEntry
(
    volatile librorc_event_descriptor *reportbuffer,
             uint64_t                  i,
             uint32_t                  ch
)
{
    DEBUG_PRINTF
    (
        PDADEBUG_CONTROL_FLOW,
        "CH%2d - RB[%3ld]: calc_size=%08x\t"
        "reported_size=%08x\t"
        "offset=%lx\n",
        ch, i, reportbuffer->calc_event_size,
        reportbuffer->reported_event_size,
        reportbuffer->offset
    );
}



int
event_sanity_checker::compareCalculatedToReportedEventSizes
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t reported_event_size = reportedEventSize(report_buffer);
    uint32_t calc_event_size = calculatedEventSize(report_buffer);

    /** Bit 31 of calc_event_size is read completion timeout flag */
    uint32_t timeout_flag = (report_buffer->calc_event_size>>31);

    if (timeout_flag)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
                m_channel_id, report_buffer_index);
        return(CHK_SIZES);
    }
    else if (calc_event_size != reported_event_size)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                        "calculated: 0x%x, reported: 0x%x\n"
                        "offset=0x%lx, rbdm_offset=0x%lx\n", m_channel_id,
                report_buffer_index, calc_event_size, reported_event_size,
                report_buffer->offset,
                report_buffer_index * sizeof(librorc_event_descriptor));
        return(CHK_SIZES);
    }

    return 0;
}



int
event_sanity_checker::checkStartOfEvent
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t *event = rawEventPointer(report_buffer);

    if ((uint32_t) * (event) != 0xffffffff)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
                        "offset=%ld, rbdm_offset=%ld\n", report_buffer_index,
                (uint32_t) * (event), report_buffer->offset,
                report_buffer_index * sizeof(librorc_event_descriptor));

        return dumpError(report_buffer, report_buffer_index, CHK_SOE);
    }

    return 0;
}



int
event_sanity_checker::checkPattern
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t *event           = rawEventPointer(report_buffer);
    uint32_t  calc_event_size = calculatedEventSize(report_buffer);

    switch(m_pattern_mode)
    {
        case PG_PATTERN_RAMP: /* Data pattern is a ramp */
        {
            for(m_event_index = 8; m_event_index < calc_event_size;m_event_index++)
            {
                if ((uint32_t) * (event + m_event_index) != (m_event_index - 8) )
                {
                    DEBUG_PRINTF
                    (
                        PDADEBUG_ERROR,
                        "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                        report_buffer_index, m_event_index, (m_event_index - 8),
                        (uint32_t)*(m_eventbuffer + dwordOffset(report_buffer) + m_event_index)
                    );

                    return dumpError(report_buffer, report_buffer_index, CHK_PATTERN);
                }
            }
        }
        break;

        default:
        {
            printf("ERROR: specified unknown pattern matching algorithm\n");
            return CHK_PATTERN;
        }
    }

    return 0;
}



int
event_sanity_checker::compareWithReferenceDdlFile
(
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    int       retval          = 0;
    uint32_t *event           = rawEventPointer(report_buffer);
    uint32_t  calc_event_size = calculatedEventSize(report_buffer);

    if( ((uint64_t) calc_event_size << 2) != m_ddl_reference_size )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: Eventsize %lx does not match "
            "reference DDL file size %lx\n",
            ((uint64_t) calc_event_size << 2),
            m_ddl_reference_size
        );
        retval |= dumpError(report_buffer, report_buffer_index, CHK_FILE);
    }

    for(m_event_index = 0; m_event_index<calc_event_size; m_event_index++)
    {
        if( event[m_event_index] != m_ddl_reference[m_event_index] )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                report_buffer_index,
                m_event_index,
                m_ddl_reference[m_event_index],
                event[m_event_index]
            );
            retval |= dumpError(report_buffer, report_buffer_index, CHK_FILE);
        }
    }

    return retval;
}



int
event_sanity_checker::checkEndOfEvent
(
             uint32_t                 *event,
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint32_t calc_event_size     = calculatedEventSize(report_buffer);
    uint32_t reported_event_size = reportedEventSize(report_buffer);

    if( (uint32_t) *(event + calc_event_size) != reported_event_size)
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: could not find matching reported event size "
            "at Event[%d] expected %08x found %08x\n",
            m_event_index,
            calc_event_size,
            (uint32_t) * (event + m_event_index)
        );
        return dumpError(report_buffer, report_buffer_index, CHK_EOE);
    }
    return 0;
}



int
event_sanity_checker::checkForLostEvents
(
             int64_t                   last_id,
    volatile librorc_event_descriptor *report_buffer,
             uint64_t                  report_buffer_index
)
{
    uint64_t offset       = dwordOffset(report_buffer);
    uint64_t cur_event_id = getEventIdFromCdh(offset);

    if
    (
        (last_id != -1) && (cur_event_id & 0xfffffffff)
        !=
        ((last_id + 1) & 0xfffffffff)
    )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
                        "current ID: %ld\n", m_channel_id, last_id,
                cur_event_id);
        return dumpError(report_buffer, report_buffer_index, CHK_ID);
    }

    return 0;
}
