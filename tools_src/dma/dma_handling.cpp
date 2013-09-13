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

/**
 * Dump reportbuffer entry
 * @param reportbuffer pointer to reportbuffer
 * @param i index of current librorc_event_descriptor within
 * reportbuffer
 * @param ch DMA channel number
 * */
void
dump_rb
(
    volatile librorc_event_descriptor *reportbuffer,
             uint64_t                  i,
             uint32_t                  ch
)
{
    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
            "CH%2d - RB[%3ld]: calc_size=%08x\t"
            "reported_size=%08x\t"
            "offset=%lx\n",
            ch, i, reportbuffer->calc_event_size,
            reportbuffer->reported_event_size,
            reportbuffer->offset);
}



/**
 * Dump Event
 * @param eventbuffer pointer to eventbuffer
 * @param offset offset of the current event within the eventbuffer
 * @param len size in DWs of the event
 * */
void
dump_event
(
    volatile uint32_t *eventbuffer,
    uint64_t offset,
    uint64_t len
)
{
#ifdef SIM
  uint64_t i = 0;
  for(i=0;i<len;i++) {
    printf("%03ld: %08x\n",
        i, (uint32_t)*(eventbuffer + offset +i));
  }
  if(len&0x01) {
    printf("%03ld: %08x (dummy)\n", i,
        (uint32_t)*(eventbuffer + offset + i));
    i++;
  }
  printf("%03ld: EOE reported_event_size: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
#if DMA_MODE==128
  i++;
  printf("%03ld: EOE calc_event_size: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
#endif
#endif
}

//TODO : this is going to be refactored into a class
int
event_sanity_checker::eventSanityCheck
(
    volatile librorc_event_descriptor *reportbuffer,
    volatile uint32_t                 *eventbuffer,
             uint64_t                  report_buffer_index,
             uint32_t                  channel_id,
             int64_t                   last_id,
             uint32_t                  pattern_mode,
             uint32_t                  check_mask,
             uint32_t                 *ddl_reference,
             uint64_t                  ddl_reference_size,
             uint64_t                 *event_id  //TODO : simply return this later
)
{
    //CONSTRUCTOR
    uint64_t offset = reportbuffer->offset / 4;
    uint32_t j;
    uint32_t *event = (uint32_t *)eventbuffer + reportbuffer->offset;
    uint64_t cur_event_id;
    int retval = 0;

    /** upper two bits of the sizes are reserved for flags */
    uint32_t reported_event_size = (reportbuffer->reported_event_size & 0x3fffffff);
    uint32_t calc_event_size = (reportbuffer->calc_event_size & 0x3fffffff);

    /** Bit31 of calc_event_size is read completion timeout flag */
    uint32_t timeout_flag = (reportbuffer->calc_event_size>>31);


    // compareCalculatedToReportedEventSizes
    if(check_mask & CHK_SIZES)
    {
        if (timeout_flag)
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
                channel_id, report_buffer_index
            );
            retval |= CHK_SIZES;
        }
        else if(calc_event_size != reported_event_size)
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                "calculated: 0x%x, reported: 0x%x\n"
                "offset=0x%lx, rbdm_offset=0x%lx\n", channel_id, report_buffer_index,
                calc_event_size,reported_event_size,
                reportbuffer->offset,
                report_buffer_index*sizeof(librorc_event_descriptor)
            );
            retval |= CHK_SIZES;
        }
    }

    // checkStartOfEvent
    // Each event has a CommonDataHeader (CDH) consisting of 8 DWs,
    // see also http://cds.cern.ch/record/1027339?ln=en
    if( (check_mask & CHK_SOE) && ((uint32_t)*(event)!=0xffffffff) )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
            "offset=%ld, rbdm_offset=%ld\n",
            report_buffer_index, (uint32_t)*(event),
            reportbuffer->offset,
            report_buffer_index*sizeof(librorc_event_descriptor)
        );

        dump_event(eventbuffer, offset, reported_event_size);
        dump_rb(reportbuffer, report_buffer_index, channel_id);

        retval |= CHK_SOE;
    }

    // checkPattern
    if( check_mask & CHK_PATTERN )
    {
        switch (pattern_mode)
        {
            /* Data pattern is a ramp */
            case PG_PATTERN_RAMP:
            {
                // checkPgPattern
                for (j=8;j<calc_event_size;j++)
                {
                    if ( (uint32_t)*(event + j) != j-8 )
                    {
                        DEBUG_PRINTF
                        (
                            PDADEBUG_ERROR,
                            "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                            report_buffer_index, j, j-8, (uint32_t)*(eventbuffer + offset + j)
                        );

                        dump_event(eventbuffer, offset, reported_event_size);
                        dump_rb(reportbuffer, report_buffer_index, channel_id);
                        retval |= CHK_PATTERN;
                    }
                }
            }
            break;

            default:
            {
                printf("ERROR: specified unknown pattern matching algorithm\n");
                retval |= CHK_PATTERN;
            }
        }
    }

    // compareWithReferenceDdlFile
    if( check_mask & CHK_FILE )
    {
        if ( ((uint64_t)calc_event_size<<2) != ddl_reference_size )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: Eventsize %lx does not match "
                "reference DDL file size %lx\n",
                ((uint64_t)calc_event_size<<2),
                ddl_reference_size
            );

            dump_event(eventbuffer, offset, reported_event_size);
            dump_rb(reportbuffer, report_buffer_index, channel_id);
            retval |= CHK_FILE;
        }

        for (j=0;j<calc_event_size;j++)
        {
            if ( event[j] != ddl_reference[j] )
            {
                DEBUG_PRINTF
                (
                    PDADEBUG_ERROR,
                    "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                    report_buffer_index, j, ddl_reference[j], event[j]
                );

                //TODO : this is redundant over the whole code -> refactor to dump and throw!
                dump_event(eventbuffer, offset, reported_event_size);
                dump_rb(reportbuffer, report_buffer_index, channel_id);
                retval |= CHK_FILE;
            }
        }
    }

    /**
    * 32 bit DMA mode
    *
    * DMA data is written in multiples of 32 bit. A 32bit EOE word
    * is directly attached after the last event data word.
    * The EOE word contains the EOE status word received from the DIU
    **/
    //checkEndOfEvent
    if( check_mask & CHK_EOE )
    {
        if( (uint32_t)*(event + calc_event_size) != reported_event_size )
        {
            DEBUG_PRINTF
            (
                PDADEBUG_ERROR,
                "ERROR: could not find matching reported event size "
                "at Event[%d] expected %08x found %08x\n",
                j, calc_event_size,
                (uint32_t)*(event + j)
            );

            dump_event(eventbuffer, offset, calc_event_size);
            dump_rb(reportbuffer, report_buffer_index, channel_id);
            retval |= CHK_EOE;
        }
    }

    // get EventID from CDH:
    // lower 12 bits in CHD[1][11:0]
    // upper 24 bits in CDH[2][23:0]
    cur_event_id = (uint32_t)*(eventbuffer + offset + 2) & 0x00ffffff;
    cur_event_id <<= 12;
    cur_event_id |= (uint32_t)*(eventbuffer + offset + 1) & 0x00000fff;

    // make sure EventIDs increment with each event.
    // missing EventIDs are an indication of lost event data
    if
    (
        (check_mask & CHK_ID) && (last_id != -1) && (cur_event_id & 0xfffffffff)
        !=
        ((last_id +1) & 0xfffffffff)
    )
    {
        DEBUG_PRINTF
        (
            PDADEBUG_ERROR,
            "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
            "current ID: %ld\n", channel_id, last_id, cur_event_id
        );

        dump_event(eventbuffer, offset, calc_event_size);
        dump_rb(reportbuffer, report_buffer_index, channel_id);
        retval |= CHK_EOE;
    }

    /** return event ID to caller */
    *event_id = cur_event_id;

    return retval;
}
