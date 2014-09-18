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

#ifndef BOARDTEST_MODULES_HH
#define BOARDTEST_MODULES_HH

#define LIBRORC_INTERNAL

#include <librorc.h>
extern "C"{
#include <pci/pci.h>
}
#include "../dma/dma_handling.hh"
#include "crorc-smbus-ctrl.hh"

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

#define DMA_RATE_MIN 300.0
#define DMA_RATE_MAX 450.0

#define UC_DEVICE_SIGNATURE 0x15951e


#define HEXSTR(x, width) "0x" << setw(width) << setfill('0') << hex << x << setfill(' ') << dec

void printHeader(const char *title);

void checkDdr3ModuleSpd(librorc::sysmon *sm, int module_id, int verbose);
void checkDdr3ModuleCalib(librorc::bar *bar, int module_id, int verbose);
void checkDdr3ModuleTg(librorc::bar *bar, int module_id, int verbose);

uint8_t getDipSwitch(librorc::bar *bar);
void checkMicrocontroller(librorc::bar *bar, int firstrun, int verbose);

void checkLvdsTester (librorc::bar *bar, int verbose);
void checkRefClkGen (librorc::sysmon *sm, int verbose);
void checkFpgaSystemMonitor(librorc::sysmon *sm, int verbose);

bool flashManufacturerCodeIsValid( librorc::flash *flash, int verbose );
uint64_t checkFlash(librorc::device *dev, int chip_select, int verbose);
bool checkFlashDeviceNumbersAreValid( uint64_t devnr0, uint64_t devnr1 );

bool checkQsfp(librorc::sysmon *sm, int module_id, int verbose);
void checkQsfpTemperature(librorc::sysmon *sm, int module_id, int verbose);
void checkQsfpVcc(librorc::sysmon *sm, int module_id, int verbose);
void checkQsfpOpticalLevels(librorc::sysmon *sm, int module_id, int verbose);

struct pci_dev *initLibPciDev(librorc::device *dev);

void checkPcieState(librorc::sysmon *sm, int verbose);
uint64_t getPcieDSN(struct pci_dev *pdev);
bool checkForValidBarConfig(struct pci_dev *pdev);

void checkFpgaFan(librorc::sysmon *sm, int verbose);
int checkSysClkAvailable(librorc::sysmon *sm);

uint32_t checkCount(uint32_t channel_id, uint32_t reg, const char *name);
uint32_t checkLinkState( librorc::link *link, uint32_t channel_id );

void testDmaChannel(librorc::device *dev, librorc::bar *bar, uint32_t channel_id, int timeout, int verbose);

void printPcieInfos(librorc::device *dev, librorc::sysmon *sm);

void checkAndReleaseGtxReset(librorc::link *link, int verbose);
void checkAndReleaseQsfpResets(librorc::sysmon *sm, int verbose);

#endif
