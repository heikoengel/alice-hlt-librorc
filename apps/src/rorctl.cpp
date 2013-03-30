#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "librorc.h"

using namespace std;

#define HELP_TEXT "rorctl usage :                            \n\
rorctl [value parameter(s)] [instruction parameter]          \n\
Value parameters :                                           \n\
  -n [0...255]    Target device ID (get a list with -l).     \n\
  -f <filename>   Filename of the flash file.                \n\
  -v              Be verbose                                 \n\
Instruction parameters :                                     \n\
  -l              List available devices.                    \n\
  -d              Dump device firmware to file.              \n\
                  Requires value parameters -n and f.        \n\
  -p              Program device flash.                      \n\
                  Requires value parameters -n and f.        \n\
  -e              Erase device flash (caution JTAG programmer\n\
                  needed afterwards)                         \n\
                  Requires value parameters -n               \n\
Examples :                                                   \n\
Dump firmware of device 0 to file firmware.bin :             \n\
  rorctl -n 0 -f firmware.bin -d                             \n\
Program firmware into device 0 from file firmware.bin :      \n\
  rorctl -n 0 -f firmware.bin -p                             \n\
Erase firmware from device 0:                                \n\
  rorctl -n 0 -e                                             \n\
List all available devices :                                 \n\
  rorctl -l                                                  \n\
"

#ifndef NOT_SET
    #define NOT_SET 0xFFFFFFFFFFFFFFFF
#endif

/* Parameter container */
typedef struct
{
    uint64_t  device_number;
    char     *filename;
    uint8_t   verbose;
}confopts;

void print_devices();

//static void inline
//init_flash
//(
//    DeviceOperator *dop,
//    confopts        options,
//    crorc_flash_t  *flash
//);
//
//int64_t
//dump_device
//(
//    DeviceOperator* dop,
//    confopts        options
//);
//
//int64_t
//flash_device
//(
//    DeviceOperator *dop,
//    confopts        options
//);
//
//int64_t
//erase_device
//(
//    DeviceOperator* dop,
//    confopts        options
//);

/*----------------------------------------------------------*/

int main
(
    int argc,
    char* const *argv
)
{
    if(argc<2)
    {
        printf( HELP_TEXT );
        return -1;
    }

    confopts options =
    {
        NOT_SET,
        NULL,
        0
    };

    rorcfs_device *dev = NULL;

    {
        opterr = 0;
        int c;
        while((c = getopt(argc, argv, "hvldepn:f:")) != -1)
        {
            switch(c)
            {
                case 'h':
                {
                    printf( HELP_TEXT );
                    goto ret_main;
                }
                break;

                case 'v':
                {
                    options.verbose = 1;
                }
                break;

                case 'l':
                {
                    print_devices();
                }
                break;

                case 'd':
                {
                    //dump_device(dop, options);
                }
                break;

                case 'e':
                {
                    //erase_device(dop, options);
                }
                break;

                case 'p':
                {
                    //flash_device(dop, options);
                }
                break;

                case 'n':
                {
                    options.device_number = atoi(optarg);
                    dev = new rorcfs_device();
                    if ( dev->init(0) == -1 )
                    {
                        cout << "ERROR: failed to initialize device!" << endl;
                        goto ret_main;
                    }
                }
                break;

                case 'f':
                {
                    options.filename = (char *)malloc(strlen(optarg)+2);
                    sprintf(options.filename, "%s", optarg);
                }
                break;

                default:
                {
                    printf("Unknown parameter (%c)!\n", c);
                    printf( HELP_TEXT );
                    goto ret_main;
                }
                break;
            }
        }
    }

ret_main:
    if(options.filename != NULL)
    {
        free(options.filename);
    }

    return -1;
}



void
print_devices()
{
    printf("Available CRORC devices:\n");
    uint64_t device_count  = 0;


    rorcfs_device *dev = NULL;
    for(uint8_t i=0; i>(-1); i++)
    {
        uint16_t domain_id = 0;
        uint8_t  bus_id = 0;
        uint8_t  device_id = 0;
        uint8_t  function_id = 0;
        char *description = (char*)malloc(1024 * sizeof(char));

        dev = new rorcfs_device();
        if ( dev->init(0) == -1 )
        {
            break;
        }

//        PdaDebugReturnCode ret = PDA_SUCCESS;
//        ret = DeviceOperator_getPciDevice(dop, &device, i);
//            ret += PciDevice_getDomainID(device, &domain_id);
//            ret += PciDevice_getBusID(device, &bus_id);
//            ret += PciDevice_getDeviceID(device, &device_id);
//            ret += PciDevice_getFunctionID(device, &function_id);
//            ret += PciDevice_getDescription(device, &description);
//
//        if( ret != PDA_SUCCESS )
//        {
//            abort();
//        }

        printf("Device [%u] %04x:%02x:%02x.%x : %s\n", i,
               domain_id, bus_id, device_id, function_id, description);

        delete dev;
        free(description);
    }
}
//
//int64_t
//dump_device
//(
//    DeviceOperator *dop,
//    confopts        options
//)
//{
//    crorc_flash_t flash;
//    init_flash(dop, options, &flash);
//
//    FILE *filep =
//        fopen(options.filename, "w");
//
//    uint64_t flash_words = (FLASH_SIZE/2);
//    uint16_t *flash_buffer =
//        (uint16_t*)calloc(flash_words, sizeof(uint16_t));
//
//    set_read_state(&flash, FLASH_READ_ARRAY, 0x00);
//    if(options.verbose == 1)
//    {
//        for(uint64_t i=0; i<flash_words; i++)
//        {
//            flash_buffer[i] = GET(flash, i);
//            printf("%" PRIx64 ": %04x\n", i, flash_buffer[i]);
//        }
//    }
//    else
//    {
//        for(uint64_t i=0; i<flash_words; i++)
//        {
//            flash_buffer[i] = GET(flash, i);
//        }
//    }
//
//    fwrite(flash_buffer, FLASH_SIZE, 1, filep);
//    fclose(filep);
//    free(flash_buffer);
//
//    return 0;
//}
//
//
//int64_t
//erase_device
//(
//    DeviceOperator *dop,
//    confopts        options
//)
//{
//    crorc_flash_t flash;
//    init_flash(dop, options, &flash);
//
//    uint64_t starting_block = 0;
//    uint64_t end_block = 258;
//
//    printf("Erasing : 100%%");
//    uint64_t blocks = end_block - starting_block;
//    for(uint64_t i=starting_block; i<=end_block; i++)
//    {
//        uint64_t addr = (255<<16) + ((i-255)<<14);
//        if(i<=255)
//		{
//            addr = (i<<16);
//		}
//
//        printf("\b\b\b\b%3" PRIu64 "%%", (i*100)/blocks);
//        fflush(stdout);
//
//        if(get_block_lock_configuration(&flash, addr) & 0x01)
//        {
//            unlock_block(&flash, addr);
//        }
//
//        if(erase_block(&flash, addr) != 0)
//        {
//			printf("Flash erase failed, Status (STS): %" PRIx16 "x\n",
//                    get_status_register(&flash) );
//			abort();
//		}
//    }
//
//    printf("\nErase complete.\n");
//
//    return 0;
//}
//
//
//int64_t
//flash_device
//(
//    DeviceOperator *dop,
//    confopts        options
//)
//{
//    if(options.filename == NULL)
//    {
//        printf("File was not given!\n");
//        abort();
//    }
//
//    struct stat stat_buf;
//    if( stat(options.filename, &stat_buf) != 0)
//    {
//        printf("Flash input file does not exist or is not accessable!\n");
//        return -1;
//    }
//
//    if(stat_buf.st_size > FLASH_FILE_SIZE)
//    {
//        printf("Flash file is to big!\n");
//        return -1;
//    }
//
//    crorc_flash_t flash;
//    init_flash(dop, options, &flash);
//
//    /* Prequesits flash */
//    uint64_t addr = (1<<23); //start address: +16MB
//    uint64_t block_count
//        = (unsigned int)(stat_buf.st_size>>17)+1;
//
//    printf("Bitfile Size: %ld bytes (%.3f MB)\n",
//           stat_buf.st_size,(double)(stat_buf.st_size/1024.0/1024.0) );
//	printf("Bitfile will be written to Flash starting at addr %" PRIx64 "\n", addr);
//	printf("Using %" PRIu64 " Blocks (%" PRIu64 " to %" PRIu64 ")\n",
//            block_count, addr>>16, (addr>>16)+block_count-1);
//
//    /* Open the flash file */
//    /* TODO : add some cons. checking here */
//    /* TODO : add preloading here */
//    int fd = open(options.filename, O_RDONLY);
//    if( fd==-1 )
//	{
//        printf("failed to open input file %s\n", options.filename);
//        abort();
//	}
//
//    /* Erase the flash first */
//    if(erase_device(dop, options)!=0)
//    {
//        printf("CRORC flash erase failed!\n");
//        return -1;
//    }
//
//    /* Program flash */
//	uint16_t *buffer
//        = (uint16_t *)malloc(512*sizeof(uint16_t));
//    size_t bytes_read = 0;
//    size_t bytes_programmed = 0;
//
//    while( (bytes_read=read(fd, buffer, 512*sizeof(uint16_t))) > 0 )
//	{
//        if(program_buffer(&flash, addr, (bytes_read/2), buffer) < 0)
//        {
//            printf("programBuffer failed, STS: %04x\n",
//                    get_status_register(&flash));
//            break;
//        }
//        bytes_programmed += bytes_read;
//        printf("\rProgramming : %03ld%%", (100*bytes_programmed)/stat_buf.st_size);
//        fflush(stdout);
//        addr += bytes_read/2;
//	}
//	printf("\nDONE.\n");
//
//    /* Close everything */
//	free(buffer);
//    close(fd);
//
//    /* TODO : Use dump_device to verify */
//
//    return 0;
//}
//
//
//static void inline
//init_flash
//(
//    DeviceOperator *dop,
//    confopts        options,
//    crorc_flash_t  *flash
//)
//{
//    PdaDebugReturnCode ret = PDA_SUCCESS;
//
//    if(options.device_number == NOT_SET)
//    {
//        printf("Device ID was not given!\n");
//        abort();
//    }
//
//    /* Get the PDA device object */
//    PciDevice *device = NULL;
//    ret =
//        DeviceOperator_getPciDevice
//            (dop, &device, options.device_number);
//    if(ret != PDA_SUCCESS)
//    {
//        printf("Unable to get device descriptor!\n");
//        abort();
//    }
//
//    /* Get the object of the first bar of the selected device */
//    Bar *bar_flash = NULL;
//    if(PciDevice_getBar(device, &bar_flash, 0) != PDA_SUCCESS)
//    {
//        printf("Unable to get BAR0 descriptor!\n");
//        abort();
//    }
//
//    /* Map the flash controller */
//    if
//    (
//        Bar_getMap(bar_flash, &flash->flash, &flash->flash_size)
//        != PDA_SUCCESS
//    )
//    {
//        printf("Unable to get flash pointer!\n");
//        abort();
//    }
//
//    /* Get status register and display it to the users */
//    uint16_t status=
//        get_status_register(flash);
//    printf("STS: %04x\n", status);
//
//    if(status != 0x0080)
//    {
//        clear_status_register(flash);
//        usleep(100);
//        if(status & 0x0084)
//        {
//            program_resume(flash);
//		}
//		status = get_status_register(flash);
//    }
//
//	printf("STS: %04x\n", status);
//
//	if(status!=0x80)
//	{
//		printf("ERROR: illegal Flash Status %04x - aborting.\n", status);
//		abort();
//	}
//
//	printf("Manufacturer Code: %04x\n", get_manufacturer_code(flash) );
//	printf("Device ID: %04x\n", get_device_id(flash) );
//	printf("RCR: %04x\n", get_read_configuration_register(flash) );
//}
