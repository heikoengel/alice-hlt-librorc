#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include <iomanip>

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

#define FLASH_FILE_SIZE 16777216

/* Parameter container */
typedef struct
{
    uint64_t  device_number;
    char     *filename;
    uint8_t   verbose;
}confopts;

void print_devices();

inline
rorcfs_flash_htg *
init_flash
(
    confopts options
);

int64_t
dump_device
(
    confopts options
);

int64_t
erase_device
(
    confopts options
);

int64_t
flash_device
(
    confopts options
);



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

    //TODO revise this
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
                    cout << HELP_TEXT;
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
                    dump_device(options);
                }
                break;

                case 'e':
                {
                    erase_device(options);
                }
                break;

                case 'p':
                {
                    flash_device(options);
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
                    cout << "Unknown parameter (" << c << ")!" << endl;
                    cout << HELP_TEXT;
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
    cout << "Available CRORC devices:" << endl;

    rorcfs_device *dev = NULL;
    for(uint8_t i=0; i>(-1); i++)
    {
        dev = new rorcfs_device();
        if(dev->init(i) == -1)
        {
            break;
        }

        dev->printDeviceDescription();
        cout << endl;

        delete dev;
    }
}



int64_t
dump_device
(
    confopts options
)
{
    uint64_t flash_words = (FLASH_SIZE/2);
    uint16_t *flash_buffer =
        (uint16_t*)calloc(flash_words, sizeof(uint16_t));

    FILE *filep =
        fopen(options.filename, "w");
    if(filep == NULL)
    {
        cout << "File open failed!" << endl;
    }

    rorcfs_flash_htg *flash = init_flash(options);
    if(flash == NULL)
    {
        cout << "Flash init failed!" << endl;
    }

    if(options.verbose == 1)
    {
        for(uint64_t i=0; i<flash_words; i++)
        {
            flash_buffer[i] = flash->get(i);
            cout << i << " : "  << hex << setw(4) << flash_buffer[i];
        }
    }
    else
    {
        for(uint64_t i=0; i<flash_words; i++)
        {
            flash_buffer[i] = flash->get(i);
        }
    }

    size_t bytes_written =
       fwrite(flash_buffer, FLASH_SIZE, 1, filep);
    if(bytes_written != 1)
    {
        cout << "WARNING : writing to file failed!" << endl;
    }

    fclose(filep);
    free(flash_buffer);

    return 0;
}



int64_t
erase_device
(
    confopts options
)
{
    rorcfs_flash_htg *flash =
        init_flash(options);

    unsigned int addr = (1<<23); //start address: +16MB
    int block_count   = (unsigned int)((16<<20)>>17);

    for(uint64_t i=(addr>>16); i<((addr>>16)+block_count); i++)
    {
        uint64_t current_addr = (i<<16);
        printf("\rErasing block %d(%x)...", i, current_addr);
        fflush(stdout);

        if( flash->getBlockLockConfiguration(current_addr) & 0x01 )
        {
            flash->unlockBlock(current_addr);
        }

        if( flash->eraseBlock(current_addr)<0 )
        {
            printf("failed, STS: %04x\n",
                   flash->getStatusRegistercurrent_addr));
            abort();
        }

        fflush(stdout);
    }

    printf("\nErase complete.\n");

    return 0;
}



int64_t
flash_device
(
    confopts options
)
{
    if(options.filename == NULL)
    {
        printf("File was not given!\n");
        abort();
    }

    struct stat stat_buf;
    if( stat(options.filename, &stat_buf) != 0)
    {
        cout << "Flash input file does not exist or is not accessable!"
             << endl;
        return -1;
    }

    if(stat_buf.st_size > FLASH_FILE_SIZE)
    {
        printf("Flash file is to big!\n");
        return -1;
    }

    rorcfs_flash_htg *flash = init_flash(options);
    if(flash == NULL)
    {
        cout << "Flash init failed!" << endl;
    }

    /** Prequesits flash */
    uint64_t addr = (1<<23); //start address: +16MB
    uint64_t block_count
        = (unsigned int)(stat_buf.st_size>>17)+1;

    printf("Bitfile Size: %ld bytes (%.3f MB)\n",
           stat_buf.st_size,(double)(stat_buf.st_size/1024.0/1024.0) );
	printf("Bitfile will be written to Flash starting at addr %" PRIx64 "\n", addr);
	printf("Using %" PRIu64 " Blocks (%" PRIu64 " to %" PRIu64 ")\n",
            block_count, addr>>16, (addr>>16)+block_count-1);

    /** Open the flash file */
    /* TODO : add some cons. checking here */
    /* TODO : add preloading here */
    int fd = open(options.filename, O_RDONLY);
    if(fd == -1)
	{
        printf("failed to open input file %s\n", options.filename);
        abort();
	}

    /** Erase the flash first */
    if(erase_device(options)!=0)
    {
        printf("CRORC flash erase failed!\n");
        return -1;
    }

    /** Program flash */
    size_t bytes_read = 0;
    size_t bytes_programmed = 0;
    uint16_t *buffer
        = (uint16_t *)malloc(32*sizeof(uint16_t));

    while ( (bytes_read=read(fd, buffer, 32*sizeof(unsigned short))) > 0 )
    {
        printf("writing %d bytes to %x\n", bytes_read, addr);
        if ( flash->programBuffer(addr, bytes_read/2, buffer) < 0 )
        {
            printf("programBuffer failed, STS: %04x\n",
                flash->getStatusRegister(addr));
            break;
        }

        for(uint64_t i=0; i<bytes_read/2; i++)
        {
            uint16_t status = flash->get(addr+i);
            if( buffer[i] != status )
            {
                printf("write failed: written %04x, "
                    "read %04x, addr %x, bytes_read %d\n",
                    buffer[i], status, addr+i, bytes_read);
                break;
            }
        }

        bytes_programmed += bytes_read;
        printf("\r%03ld%%", (100*bytes_programmed)/stat_buf.st_size);
        fflush(stdout);
        addr += bytes_read/2;
    }
    printf("\nDONE.\n");

    /* Close everything */
	free(buffer);
    close(fd);

    /* TODO : Use dump_device to verify */

    return 0;
}



inline
rorcfs_flash_htg *
init_flash
(
    confopts options
)
{
    if(options.device_number == NOT_SET)
    {
        cout << "Device ID was not given!" << endl;
        abort();
    }

    rorcfs_device *dev
        = new rorcfs_device();
    if(dev->init(options.device_number) == -1)
    {
        cout << "Failed to initialize device "
             << options.device_number << endl;
        return(NULL);
    }

    rorcfs_bar *bar
        = new rorcfs_bar(dev, 0);
    if(bar->init() == -1)
    {
        cout << "BAR0 init failed" << endl;
        return(NULL);
    }

    rorcfs_flash_htg *flash
        = new rorcfs_flash_htg(bar);

    uint16_t status
        = flash->getStatusRegister(0);

    cout << "Status               : " << hex
         << setw(4) << status << endl;

    if( status != 0x0080 )
    {
        flash->clearStatusRegister(0);
        usleep(100);
        if ( status & 0x0084 )
        {
            flash->programResume(0);
        }
        status = flash->getStatusRegister(0);
    }

    cout << "Status               : " << hex
         << setw(4) << status << endl;

    cout << "Manufacturer Code    : " << hex << setw(4)
         << flash->getManufacturerCode() << endl;
    cout << "Device ID            : " << hex << setw(4)
         << flash->getDeviceID() << endl;
    cout << "Read Config Register : " << hex << setw(4)
         << flash->getReadConfigurationRegister() << endl;

    return(flash);
}
