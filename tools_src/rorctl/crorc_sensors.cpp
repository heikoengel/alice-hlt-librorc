/**
 * @file crorc_sensors.cpp
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2014-08-25
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * */

#include <librorc.h>
#include <getopt.h>
#include <string>

#define MAX_LIBRORC_DEVICE_ID 31
#define NAME_VALUE(name) #name << ": " << name

#define HELP_TEXT                                                              \
  "crorc_sensors usage: \n"                                                    \
  "  -h|--help        print this help text\n"                                  \
  "  -d|--device [n]  select target C-RORC device. Uses device 0 if not\n"     \
  "                   specified\n"                                             \
  "  -l|--list        list all available sensors\n"                            \
  "  --fpga_temp      print FPGA Temperature in Degree Celsius\n"              \
  "  --fpga_fw_rev    print FPGA Firmware Revision\n"                          \
  "  --fpga_fw_date   print FPGA Firmware Date\n"                              \
  "  --fpga_uptime    print FPGA Uptime in Seconds\n"                          \
  "  --fpga_fw_descr  print Firmware Description string\n"                     \
  "  --fpga_vcc_int   print FPGA Core Voltage in V\n"                          \
  "  --fpga_vcc_aux   print FPGA Auxilliary Voltage in V\n"                    \
  "  --fan_speed       print FPGA Fan RPM or 0 if stopped\n"                    \
  "  --pcie_slot      print PCIe Domain:Bus:Slot.Funtion numbers\n"            \
  "  --pcie_lanes     print number of PCIe lanes used for C-RORC\n"            \
  "  --pcie_gen       print PCIe Generation used for C-RORC\n"                 \
  "  --pcie_tx_error  print PCIe TX Error Count\n"                             \
  "  --smbus_slv_addr print SMBus Slave Address for Configuration\n"           \
  "                   Controller access\n"                                     \
  "\n"

int main(int argc, char *argv[]) {
  //int sAll = 0;
  int sFpgaTemp = 0;
  int sFpgaFwDate = 0;
  int sFpgaFwRev = 0;
  int sFpgaUptime = 0;
  int sFpgaFwDesrc = 0;
  int sFpgaVccInt = 0;
  int sFpgaVccAux = 0;
  int sFpgaFan = 0;
  int sPcieSlot = 0;
  int sPcieLanes = 0;
  int sPcieGen = 0;
  int sPcieTxErr = 0;
  int sSmbusSlvAddr = 0;
  uint32_t deviceId = 0;

  static struct option long_options[] = {
    { "help", no_argument, 0, 'h' },
    { "device", required_argument, 0, 'd' },
    { "list", no_argument, 0, 'l' },
    { "fpga_temp", no_argument, &sFpgaTemp, 1 },
    { "fpga_fw_rev", no_argument, &sFpgaFwRev, 1 },
    { "fpga_fw_date", no_argument, &sFpgaFwDate, 1 },
    { "fpga_uptime", no_argument, &sFpgaUptime, 1 },
    { "fpga_fw_descr", no_argument, &sFpgaFwDesrc, 1 },
    { "fpga_vcc_int", no_argument, &sFpgaVccInt, 1 },
    { "fpga_vcc_aux", no_argument, &sFpgaVccAux, 1 },
    { "fan_speed", no_argument, &sFpgaFan, 1 },
    { "pcie_slot", no_argument, &sPcieSlot, 1 },
    { "pcie_lanes", no_argument, &sPcieLanes, 1 },
    { "pcie_gen", no_argument, &sPcieGen, 1 },
    { "pcie_tx_error", no_argument, &sPcieTxErr, 1 },
    { "smbus_slv_addr", no_argument, &sSmbusSlvAddr, 1 },
    { 0, 0, 0, 0 }
  };

  /** Parse command line arguments **/
  if (argc > 1) {
    while (1) {
      int opt = getopt_long(argc, argv, "hld:", long_options, NULL);
      if (opt == -1) {
        break;
      }

      switch (opt) {
      case 'h':
        printf(HELP_TEXT);
        return -1;
      case 'd':
        deviceId = strtol(optarg, NULL, 0);
        break;
      case 'l': {
        int iter = 0;
        while (long_options[iter].name != 0) {
          if (long_options[iter].val == 1) {
            printf("--%s\n", long_options[iter].name);
          }
          iter++;
        }
        return 0;
      }
      case '?':
        return -1;
      default:
        continue;
      }
    }
  } else {
    printf(HELP_TEXT);
    return -1;
  }

  librorc::device *dev = NULL;
  try {
    dev = new librorc::device(deviceId);
  }
  catch (...) {
    printf("Failed to initialize device %d\n", deviceId);
    return -1;
  }

  librorc::bar *bar = NULL;
  try {
#ifdef MODELSIM
    bar = new librorc::sim_bar(dev, 1);
#else
    bar = new librorc::rorc_bar(dev, 1);
#endif
  }
  catch (...) {
    delete dev;
    return -1;
  }

  librorc::sysmon *sm = NULL;
  try {
    sm = new librorc::sysmon(bar);
  }
  catch (...) {
    delete bar;
    delete dev;
    return -1;
  }

  if (sFpgaTemp) {
    printf("%.1f\n", sm->FPGATemperature());
  }
  if (sFpgaFwRev) {
    printf("0x%08x\n", sm->FwRevision());
  }
  if (sFpgaFwDate) {
    printf("%08x\n", sm->FwBuildDate());
  }
  if (sFpgaUptime) {
    printf("%ld\n", sm->uptimeSeconds());
  }
  if (sFpgaFwDesrc) {
    printf("%s\n", sm->firmwareDescription());
  }
  if (sFpgaVccInt) {
      printf("%.2f\n", sm->VCCINT());
  }
  if (sFpgaVccAux) {
      printf("%.2f\n", sm->VCCAUX());
  }
  if (sFpgaFan) {
    if (!sm->systemFanIsRunning()) {
      printf("0\n");
    } else {
      printf("%d\n", (int)(sm->systemFanSpeed()));
    }
  }
  if (sPcieSlot) {
    printf("%04d:%02d:%02d.%d\n", dev->getDomain(), dev->getBus(),
           dev->getSlot(), dev->getFunc());
  }
  if (sPcieLanes) {
    printf("%d\n", sm->pcieNumberOfLanes());
  }
  if (sPcieGen) {
    printf("%d\n", sm->pcieGeneration());
  }
  if (sSmbusSlvAddr) {
    printf("0x%02x\n", sm->dipswitch());
  }
  if (sPcieTxErr) {
      printf("%d\n", sm->pcieTransmissionErrorCounter());
  }


  if (sm) {
    delete sm;
  }
  if (bar) {
    delete bar;
  }
  if (dev) {
    delete dev;
  }

  return 0;
}
