/**
 * @file boardtest_modules.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-12-09
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

#ifndef BOARDTEST_MODULES_HH
#define BOARDTEST_MODULES_HH


#include <librorc.h>
#include "../dma/dma_handling.hh"

#define FPGA_TEMP_MIN 35.0
#define FPGA_TEMP_MAX 65.0
#define FPGA_VCCINT_MIN 0.9
#define FPGA_VCCINT_MAX 1.1
#define FPGA_VCCAUX_MIN 2.4
#define FPGA_VCCAUX_MAX 2.6

#define FPGA_FANSPEED_MIN 8000.0
#define FPGA_FANSPEED_MAX 10000.0

#define REFCLK_FREQ_MIN 212.4
#define REFCLK_FREQ_MAX 212.6

#define QSFP_TEMP_MIN 20.0
#define QSFP_TEMP_MAX 50.0
#define QSFP_VCC_MIN 3.2
#define QSFP_VCC_MAX 3.4
#define QSFP_RXPOWER_MIN 0.1
#define QSFP_RXPOWER_MAX 1.0
#define QSFP_TXBIAS_MIN 1.0
#define QSFP_TXBIAS_MAX 10.0

#define UC_DEVICE_SIGNATURE 0x15951e

#define HEXSTR(x, width) "0x" << setw(width) << setfill('0') << hex << x << setfill(' ')

void checkDdr3ModuleSpd(librorc::sysmon *sm, int module_id, int verbose);
void checkDdr3ModuleCalib(librorc::bar *bar, int module_id);
void checkDdr3ModuleTg(librorc::bar *bar, int module_id);

void checkMicrocontroller(librorc::bar *bar, int verbose);
void checkLvdsTester (librorc::bar *bar);
void checkFlash(librorc::device *dev, int chip_select, int verbose);
void checkRefClkGen (librorc::sysmon *sm, int verbose);

void checkQsfp(librorc::sysmon *sm, int module_id, int verbose);
void checkQsfpTemperature(librorc::sysmon *sm, int module_id, int verbose);
void checkQsfpVcc(librorc::sysmon *sm, int module_id, int verbose);
void checkQsfpOpticalLevels(librorc::sysmon *sm, int module_id, int verbose);

void checkFpgaSystemMonitor(librorc::sysmon *sm, int verbose);

void checkPcieState(librorc::sysmon *sm);

void checkFpgaFan(librorc::sysmon *sm, int verbose);
int checkSysClkAvailable(librorc::sysmon *sm);

void checkLinkState( librorc::link *link, uint32_t channel_id );

void testDmaChannel(uint32_t device_number, int timeout);
#endif
