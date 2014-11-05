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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#include <librorc.h>

#define HELP_TEXT                                                              \
  "crorc_flash usage:\n"                                                       \
  "  -n [0...255]    Select device ID (required)\n"                            \
  "  -c [0,1]        Select flash chip (required)\n"                           \
  "  -v              Be verbose\n"                                             \
  "  -h              Print this help screen\n"                                 \
  "  -d [filename]   Dump flash content firmware to file\n"                    \
  "  -w [filename]   Program device flash.\n"                                  \
  "  -e              Erase flash\n"                                            \
  "  -s              Show flash status\n"                                      \
  ""

using namespace std;

void flash_dump_status_errors(uint16_t status) {
  if (status == 0xffff) {
    cout << "Received Flash Status 0xffff - Flash access failed" << endl;
  } else if (status != 0x0080) {
    cout << setfill('0');
    cout << "Flash Status : " << hex << setw(4) << status << endl;

    if (status & (1 << 7)) {
      cout << "\tReady" << endl;
    } else {
      cout << "\tBusy" << endl;
    }

    if (status & (1 << 6)) {
      cout << "\tErase suspended" << endl;
    } else {
      cout << "\tErase in progress or completed" << endl;
    }

    if (status & (1 << 5)) {
      cout << "\tErase/blank check error" << endl;
    } else {
      cout << "\tErase/blank check sucess" << endl;
    }

    if (status & (1 << 4)) {
      cout << "\tProgram Error" << endl;
    } else {
      cout << "\tProgram sucess" << endl;
    }

    if (status & (1 << 3)) {
      cout << "\tVpp invalid, abort" << endl;
    } else {
      cout << "\tVpp OK" << endl;
    }

    if (status & (1 << 2)) {
      cout << "\tProgram Suspended" << endl;
    } else {
      cout << "\tProgram in progress or completed" << endl;
    }

    if (status & (1 << 1)) {
      cout << "\tProgram/erase on protected block, abort" << endl;
    } else {
      cout << "\tNo operation to protected block" << endl;
    }

    if (status & 1) {
      if (status & (1 << 7)) {
        cout << "\tNot Allowed" << endl;
      } else {
        cout << "\tProgram or erase operation in a bank other than the "
                "addressed bank" << endl;
      }
    } else {
      if (status & (1 << 7)) {
        cout << "\tNo program or erase operation in the device" << endl;
      } else {
        cout << "\tProgram or erase operation in addressed bank" << endl;
      }
    }
  }
}

void flash_print_status(librorc::flash *flash) {
  flash->clearStatusRegister(0);
  uint16_t flashstatus = flash->getStatusRegister(0);

  if (flashstatus != 0x80) {
    flash_dump_status_errors(flashstatus);
  } else {
    cout << "Status               : " << hex << setw(4) << flashstatus << endl;
    cout << "Manufacturer Code    : " << hex << setw(4)
         << flash->getManufacturerCode() << endl;
    cout << "Device ID            : " << hex << setw(4) << flash->getDeviceID()
         << endl;
    cout << "Read Config Register : " << hex << setw(4)
         << flash->getReadConfigurationRegister() << endl;
    cout << "Unique Device Number : " << hex << flash->getUniqueDeviceNumber()
         << endl;
  }

  flash->resetChip();
}

int main(int argc, char *argv[]) {
 
  bool device_id_set = false;
  uint32_t device_id = 0;
  
  bool flash_select_set = false;
  uint32_t flash_select = 0;

  char *input_filename = NULL;
  bool do_file_to_flash = false;

  char *output_filename = NULL;
  bool do_flash_to_file = false;

  bool do_erase_flash = false;
  librorc::librorc_verbosity_enum verbose = librorc::LIBRORC_VERBOSE_OFF;
  bool do_print_status = true;

  int arg;
  while ((arg = getopt(argc, argv, "n:c:d:w:evhs")) != -1) {
    switch (arg) {
    case 'h':
      cout << HELP_TEXT;
      return 0;
      break;
    case 'n':
      device_id = strtoul(optarg, NULL, 0);
      device_id_set = true;
      break;
    case 'c':
      flash_select = strtoul(optarg, NULL, 0);
      flash_select_set = true;
      break;
    case 'd':
      do_flash_to_file = true;
      do_print_status = false;
      output_filename = (char *)malloc(strlen(optarg) + 1);
      sprintf(output_filename, "%s", optarg);
      break;
    case 'w':
      do_file_to_flash = true;
      do_print_status = false;
      input_filename = (char *)malloc(strlen(optarg) + 1);
      sprintf(input_filename, "%s", optarg);
      break;
    case 'e':
      do_erase_flash = true;
      do_print_status = false;
      break;
    case 's':
      do_print_status = true;
      break;
    case 'v':
      verbose = librorc::LIBRORC_VERBOSE_ON;
      break;
    default:
      cout << "ERROR: Unknown parameter (" << arg << ")" << endl;
      cout << HELP_TEXT;
      abort();
    }
  }

  /** check parameters **/
  if (!device_id_set) {
    cout << "ERROR: No board ID given." << endl << HELP_TEXT << endl;
    abort();
  }

  if (!flash_select_set || flash_select > 1) {
    cout << "ERROR: No or invalid flash select given." << endl << HELP_TEXT
         << endl;
    abort();
  }

  /** initialize device **/
  librorc::device *dev = NULL;
  try {
    dev = new librorc::device(device_id);
  }
  catch (...) {
    cout << "ERROR: failed to initialize device " << device_id << endl;
    abort();
  }

  /** initialize BAR **/
  librorc::bar *bar = NULL;
  try {
#ifdef MODELSIM
    bar = new librorc::sim_bar(dev, 0);
#else
    bar = new librorc::rorc_bar(dev, 0);
#endif
  }
  catch (...) {
    cout << "ERROR: failed to initialize BAR0." << endl;
    abort();
  }

  /** initialize selected flash chip **/
  librorc::flash *flash = NULL;
  try {
    flash = new librorc::flash(bar, flash_select);
  }
  catch (...) {
    cout << "ERROR: failed to initialize flash." << endl;
    abort();
  }

  /** set asynchronous read mode */
  flash->setAsynchronousReadMode();

  uint16_t status = flash->resetChip();
  if (status) {
    cout << "WARNING: Flash resetChip failed " << endl;
    flash_dump_status_errors(status);
  }

  bool skip_further_steps = false;

  /** erase flash chip */
  if (do_erase_flash) {
    int32_t result = flash->erase((16 << 20), verbose);
    if (result < 0) {
      cout << "ERROR: flash erase failed" << endl;
      flash_dump_status_errors(-result);
      skip_further_steps = true;
    }
  }

  /** program flash chip */
  if (do_file_to_flash && !skip_further_steps) {
    int32_t result = flash->flashWrite(input_filename, verbose);
    if (result < 0) {
      cout << "ERROR: write to flash failed" << endl;
      skip_further_steps = true;
    }
  }

  /** dump flash contents to file */
  if (do_flash_to_file && !skip_further_steps) {
    int32_t result = flash->dump(output_filename, verbose);
    if (result < 0) {
      cout << "ERROR: dump to file failed" << endl;
      skip_further_steps = true;
    }
  }

  if (do_print_status || skip_further_steps) {
    flash_print_status(flash);
  }

  if (input_filename) {
    free(input_filename);
  }
  if (output_filename) {
    free(output_filename);
  }

  delete flash;
  delete bar;
  delete dev;

  return 0;
}
