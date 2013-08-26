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
    chstats->last_id = -1;
    chstats->channel = (unsigned int)opts.channelId;

    return(chstats);
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
