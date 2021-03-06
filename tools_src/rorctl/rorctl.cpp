/**
 * Copyright (c) 2014, Heiko Engel <hengel@cern.ch>
 * Copyright (c) 2014, Dominic Eschweiler <dominic.eschweiler@cern.ch>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of University Frankfurt, FIAS, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#define __STDC_FORMAT_MACROS

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstdio>
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
  -c              Select Flash Chip [0 or 1]                 \n\
Instruction parameters :                                     \n\
  -h              Print this help screen!                    \n\
  -l              List available devices.                    \n\
  -d              Dump device firmware to file.              \n\
                  Requires value parameters -n and f.        \n\
  -p              Program device flash.                      \n\
                  Requires value parameters -n and f.        \n\
  -e              Erase device flash (caution JTAG programmer\n\
                  needed afterwards)                         \n\
                  Requires value parameters -n               \n\
  -s              Show flash status                          \n\
  -r              Reset Flash                                \n\
  -m              Show CRORC monitor stats                   \n\
  -b [0,1]        Set CRORC bracket LED blinking on/off      \n\
Examples :                                                   \n\
Show status of Device 0 Flash 0:                             \n\
  rorctl -n 0 -c 0 -s                                        \n\
Dump firmware of device 0 to file firmware.bin :             \n\
  rorctl -n 0 -f firmware.bin -d                             \n\
Program firmware into device 0 from file firmware.bin :      \n\
  rorctl -n 0 -f firmware.bin -p                             \n\
Erase firmware from device 0:                                \n\
  rorctl -n 0 -e                                             \n\
List all available devices :                                 \n\
  rorctl -l                                                  \n\
"

#define MESSAGE_DEPRECATED "INFO: C-RORC flash access with \
rorctl is deprecated. Use crorc_flash instead."

#ifndef NOT_SET
    #define NOT_SET 0xFFFFFFFFFFFFFFFF
#endif

/** Parameter container */
typedef struct
{
    uint64_t                device_number;
    uint64_t                chip_select;
    char                   *filename;
    librorc::librorc_verbosity_enum  verbose;
    librorc::device        *dev;
}confopts;

/** Function signatures */
void print_devices();

void
print_device
(
    uint8_t index
);

void
set_led_blink_mode
(
    uint8_t index,
    uint32_t mode
);

inline
librorc::flash*
init_flash
(
    confopts options
);

int
show_device_monitor
(
    librorc::device *dev
);

librorc::flash*
flash_status
(
    librorc::flash *flash
);

void
dump_flash_status
(
    librorc::flash *flash
);



int main
(
    int argc,
    char* const *argv
)
{
    if(argc<2)
    {
        cout << HELP_TEXT;
        return -1;
    }

    confopts options =
    {
        NOT_SET,
        NOT_SET,
        NULL,
        librorc::LIBRORC_VERBOSE_OFF,
        NULL
    };

    {
        opterr = 0;
        int c;
        while((c = getopt(argc, argv, "hvlmdepn:f:c:srb:")) != -1)
        {
            switch(c)
            {
                case 'h':
                {
                    cout << HELP_TEXT;
                    goto ret_main;
                }
                break;

                case 'l':
                {
                    print_devices();
                    return(0);
                }
                break;

                case 'd':
                {
                    cout << MESSAGE_DEPRECATED << endl << endl;
                    cout << "Dumping device to file!" << endl;
                    librorc::flash *flash = init_flash(options);
                    if( flash != NULL )
                    { return( flash->dump(options.filename, options.verbose) ); }
                    else
                    { abort(); }
                }
                break;

                case 'p':
                {
                    cout << MESSAGE_DEPRECATED << endl << endl;
                    cout << "Flashing device!" << endl;
                    librorc::flash *flash = init_flash(options);
                    if( flash != NULL )
                    { return( flash->flashWrite(options.filename, options.verbose) ); }
                    else
                    { abort(); }
                }
                break;

                case 'e':
                {
                    cout << MESSAGE_DEPRECATED << endl << endl;
                    cout << "Erasing device!" << endl;
                    librorc::flash *flash = init_flash(options);
                    if( flash != NULL )
                    { return( flash->erase(16<<20, options.verbose) ); }
                    else
                    { abort(); }
                }
                break;

                case 's':
                {
                    cout << MESSAGE_DEPRECATED << endl << endl;
                    librorc::flash *flash = init_flash(options);
                    if( flash != NULL )
                    {
                        delete flash_status(flash);
                        return 0;
                    }
                    else
                    { abort(); }
                }
                break;

                case 'r':
                {
                    cout << MESSAGE_DEPRECATED << endl << endl;
                    cout << "Resetting Flash!" << endl;
                    librorc::flash *flash = init_flash(options);
                    if( flash != NULL )
                    { return( flash->resetChip() ); }
                    else
                    { abort(); }
                }
                break;

                case 'm':
                {
                    cout << "Monitor Device " << options.device_number
                         << endl << endl;
                    return( show_device_monitor(options.dev) );
                }
                break;

                /** Value parameter */
                case 'n':
                {
                    options.device_number = atoi(optarg);
                    try
                    { options.dev = new librorc::device
                        (options.device_number);
                    }
                    catch(...)
                    {
                        cout << "failed to initialize device "
                             << options.device_number << "-"
                             << " is the board detected with lspci?" << endl;
                        abort();
                    }
                }
                break;

                case 'f':
                {
                    options.filename = (char *)malloc(strlen(optarg)+2);
                    sprintf(options.filename, "%s", optarg);
                }
                break;

                case 'v':
                {
                    options.verbose = librorc::LIBRORC_VERBOSE_ON;
                }
                break;

                case 'c':
                {
                    options.chip_select = atoi(optarg);
                }
                break;

                case 'b':
                {
                    set_led_blink_mode(options.device_number, atoi(optarg));
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

    if(options.dev != NULL)
    {
        delete options.dev;
    }

    return -1;
}



void
print_devices()
{
    cout << "Available CRORC devices:" << endl;
    for(uint8_t i=0; i>(-1); i++)
    {
        print_device(i);
    }
}

void
set_led_blink_mode(
    uint8_t index,
    uint32_t mode
)
{
    /** Instantiate device with index <index> */
    librorc::device *dev = NULL;
    try{ dev = new librorc::device(index);}
    catch(...){ exit(0); }

    /** Instantiate a new bar */
    librorc::bar *bar = NULL;
    try
    {
        bar = new librorc::bar(dev, 1);
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR.\n");
        return;
    }

    bar->set32(RORC_REG_BRACKET_LED_CTRL, mode<<31);
    delete bar;
    delete dev;
}



void
print_device
(
    uint8_t index
)
{
    /** Instantiate device with index <index> */
    librorc::device *dev = NULL;
    try{ dev = new librorc::device(index);}
    catch(...){ exit(0); }

    /** Instantiate a new bar */
    librorc::bar *bar = NULL;
    try
    {
        bar = new librorc::bar(dev, 1);
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR.\n");
        return;
    }

    /** Instantiate a new sysmon */
    librorc::sysmon *sm;
    try
    { sm = new librorc::sysmon(bar); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar;
        delete dev;
        abort();
    }

    cout << "Device [" <<  (unsigned int)index << "] ";

    cout << setfill('0');
    cout << hex << setw(4) << (unsigned int)dev->getDomain() << ":"
         << hex << setw(2) << (unsigned int)dev->getBus()    << ":"
         << hex << setw(2) << (unsigned int)dev->getSlot()   << "."
         << hex << setw(1) << (unsigned int)dev->getFunc() ;

    cout << resetiosflags(std::ios::showbase);
    cout << " : " << dev->deviceDescription();

    cout << " (firmware date: " << hex << setw(8) << sm->FwBuildDate()
         << ", revision: "      << hex << setw(8) << sm->FwRevision()
         << " ["  << sm->firmwareDescription() << "])" << endl;

    delete sm;
    delete bar;
    delete dev;
}



inline
librorc::flash *
init_flash
(
    confopts options
)
{
    if(options.device_number == NOT_SET)
    {
        cout << "Device ID was not given!" << endl;
        return NULL;
    }

    if(options.chip_select == NOT_SET)
    {
        cout << "Flash Chip Select was not given, "
             << "using default Flash0" << endl;
        options.chip_select = 0;
    }

    librorc::bar *bar = NULL;
    try
    {
        bar = new librorc::bar(options.dev, 1);
    }
    catch(...)
    {
        cout << "BAR0 init failed" << endl;
        return(NULL);
    }

    /** get flash object */
    librorc::flash *flash = NULL;
    try
    {
        flash =
            new librorc::flash
                (bar, options.chip_select);
    }
    catch(int e)
    {
        switch (e)
        {
            case 1:
            { cout << "BAR is not initialized."; }
            break;

            case 2:
            { cout << "Illegal flash chip select"; }
            break;

            default:
            { cout << "Unknown Exception Nr. " << e << endl; }
            break;
        }
        return(NULL);
    }

    /** set asynchronous read mode */
    flash->setAsynchronousReadMode();

    uint16_t status = flash->resetChip();
    if ( status )
    { cout << "resetChip failed: " << hex << status << endl; }

    return(flash);
}



int
show_device_monitor
(
    librorc::device *dev
)
{
    cout << setfill('0');

    if(dev == NULL)
    {
        cout << "Device ID was not given!" << endl;
        return -1;
    }

    /** bind to BAR1 */
    librorc::bar *bar1 = NULL;
    try
    {
        bar1 = new librorc::bar(dev, 1);
    }
    catch(...)
    {
        return -1;
    }

    /** instantiate a new sysmon */
    librorc::sysmon *sm;
    try
    { sm = new librorc::sysmon(bar1); }
    catch(...)
    {
        cout << "Sysmon init failed!" << endl;
        delete bar1;
        abort();
    }

    /** Printout revision and date */
    try
    {
        cout << "CRORC FPGA" << endl
             << "Firmware Rev. : " << hex << setw(8) << sm->FwRevision()  << dec << endl
             << "Firmware Date : " << hex << setw(8) << sm->FwBuildDate() << dec << endl;
    }
    catch(...)
    {
        cout << "Reading Date returned wrong. This is likely a "
             << "PCIe access problem. Check lspci if the device is up"
             << "and the BARs are enabled" << endl;
        delete sm;
        delete bar1;
        abort();
    }

    /** Print Voltages and Temperature */
    cout << "Temperature   : " << sm->FPGATemperature() << " °C" << endl
         << "FPGA VCCINT   : " << sm->VCCINT() << " V"  << endl
         << "FPGA VCCAUX   : " << sm->VCCAUX() << " V"  << endl;

    /** Print and check reported PCIe link width/speed */
    cout << "Detected as   : PCIe Gen" << sm->pcieGeneration()
         << " x" << sm->pcieNumberOfLanes() << endl;

    if( (sm->pcieGeneration()!=2) || (sm->pcieNumberOfLanes()!=8) )
    { cout << " WARNING: FPGA reports unexpexted PCIe link parameters!" << endl; }

    /** Check if system clock is running */
    cout << "SysClk locked : " << sm->systemClockIsRunning() << endl;

    /** Check if fan is running */
    cout << "Fan speed     : " << sm->systemFanSpeed() << " rpm" << endl;
    if( sm->systemFanIsEnabled() == false)
    {
        cout << "WARNING: fan seems to be disabled!" << endl;
    }

    if( sm->systemFanIsRunning() == false)
    {
        cout << "WARNING: fan seems to be stopped!" << endl;
    }

    cout << "Uptime        : " << sm->uptimeSeconds() << "s" << endl;
    cout << "DipSwitch     : 0x" << hex << sm->dipswitch() << dec << endl;

    /** Show QSFP status */
    cout << endl << "QSFPs" << endl;
    for(uint32_t i=0; i<LIBRORC_MAX_QSFP; i++)
    {
        cout << endl << "-------------------------------------" << endl << endl;
        try
        {
            cout << "QSFP " << i << " present: " << sm->qsfpIsPresent(i)  << endl;
            cout << "QSFP " << i << " LED0 : "   << sm->qsfpLEDIsOn(i, 0)
                                 << " LED1 : "   << sm->qsfpLEDIsOn(i, 1) << endl;

            if( sm->qsfpIsPresent(i) )
            {
                cout << "Checking QSFP"  << i << " i2c access:"    << endl;
                cout << "Vendor Name : " << sm->qsfpVendorName(i)  << endl;
                cout << "Part Number : " << sm->qsfpPartNumber(i)  << endl;
                cout << "Temperature : " << sm->qsfpTemperature(i) << "°C" << endl;
            }
        }
        catch(...)
        {
            cout << "QSFP readout failed!" << endl;
        }
    }

    cout << endl;
    cout << "-------------------------------------" << endl;

    delete sm;
    delete bar1;

    return EXIT_SUCCESS;
}



librorc::flash*
flash_status
(
    librorc::flash *flash
)
{
    flash->clearStatusRegister(0);
    uint16_t flashstatus = flash->getStatusRegister(0);

    if ( flashstatus != 0x80 )
    {
        dump_flash_status(flash);
    }
    else
    {

        cout << "Status               : " << hex
             << setw(4) << flashstatus << endl;
        dump_flash_status(flash);

        cout << "Manufacturer Code    : "    << hex << setw(4)
             << flash->getManufacturerCode() << endl;
        dump_flash_status(flash);

        cout << "Device ID            : " << hex << setw(4)
             << flash->getDeviceID() << endl;
        dump_flash_status(flash);

        cout << "Read Config Register : " << hex << setw(4)
             << flash->getReadConfigurationRegister() << endl;
        dump_flash_status(flash);

        cout << "Unique Device Number : " << hex
             << flash->getUniqueDeviceNumber() << endl;
        dump_flash_status(flash);
    }

    flash->resetChip();

    return flash;
}



void
dump_flash_status
(
    librorc::flash *flash
)
{

    //TODO : put this into a proper interface and add signatures for the related states
    uint16_t status = flash->getStatusRegister(0);
    if ( status == 0xffff )
    {
        cout << "Received Flash Status 0xffff - Flash access failed"
             << endl;
    }
    else if ( status != 0x0080 )
    {
        cout << setfill('0');
        cout << "Status : " << hex << setw(4) << status << endl;

        if( status & (1<<7) )
        { cout << "\tReady" << endl; }
        else
        { cout << "\tBusy" << endl; }

        if( status & (1<<6) )
        { cout << "\tErase suspended" << endl; }
        else
        { cout << "\tErase in progress or completed" << endl; }

        if( status & (1<<5) )
        { cout << "\tErase/blank check error" << endl; }
        else
        { cout << "\tErase/blank check sucess" << endl; }

        if( status & (1<<4) )
        { cout << "\tProgram Error" << endl; }
        else
        { cout << "\tProgram sucess" << endl; }

        if( status & (1<<3) )
        { cout << "\tVpp invalid, abort" << endl; }
        else
        { cout << "\tVpp OK" << endl; }

        if( status & (1<<2) )
        { cout << "\tProgram Suspended" << endl; }
        else
        { cout << "\tProgram in progress or completed" << endl; }

        if( status & (1<<1) )
        { cout << "\tProgram/erase on protected block, abort" << endl; }
        else
        { cout << "\tNo operation to protected block" << endl; }

        if( status & 1 )
        {
            if( status & (1<<7) )
            { cout << "\tNot Allowed" << endl; }
            else
            { cout << "\tProgram or erase operation in a bank other than the addressed bank" << endl; }
        }
        else
        {
            if( status & (1<<7) )
            { cout << "\tNo program or erase operation in the device" << endl; }
            else
            { cout << "\tProgram or erase operation in addressed bank" << endl; }
        }

        flash->clearStatusRegister(0);
    }
}
