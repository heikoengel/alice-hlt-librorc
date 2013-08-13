#include "dma_handling.hh"

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
