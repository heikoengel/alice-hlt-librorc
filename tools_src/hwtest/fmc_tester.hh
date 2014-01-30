/**
 * @file fmc_tester.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-12-19i
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
#ifndef FMC_TESTER_HH
#define FMC_TESTER_HH

#include <librorc.h>

#define T_FMC_SETUP 100
#define FMC_I2C_TEMP_MIN 10
#define FMC_I2C_TEMP_MAX 60

void fmcResetTester(librorc::bar *bar);
void fmcDriveBit(librorc::bar *bar, int bit, int level);
int fmcCheckBit(librorc::bar *bar, int bit, int level);

int checkFmc(librorc::bar *bar, librorc::sysmon *sm, int verbose);

#endif
