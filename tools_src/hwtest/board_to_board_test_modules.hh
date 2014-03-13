/**
 * @file board_to_board_test_modules.hh
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2013-12-20
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

#ifndef _BOARD_TO_BOARD_MODULES_HH
#define _BOARD_TO_BOARD_MODULES_HH

#include <librorc.h>

#define WAIT_FOR_LINK_UP_TIMEOUT 10000
#define WAIT_FOR_RESET_DONE_TIMEOUT 10000
#define WAIT_FOR_RESET_DONE_RETRY 3
#define WAIT_LINK_TEST 20
#define COUNTDOWN_INTERVAL 1.0

#define GTX_DFE_EYE_DAC_MIN 120.0

const gtxpll_settings available_configs[] =
{
    //div,n1,n2,d, m, tdcc, cp, refclk
    {  9, 5, 2, 1, 1, 3, 0x0d, 212.50}, // 4.250 Gbps with RefClk=212.50 MHz
    { 10, 5, 2, 1, 1, 3, 0x0d, 250.00}, // 5.000 Gbps with RefClk=250.00 MHz
    {  4, 5, 4, 2, 1, 0, 0x0d, 100.00}, // 2.000 Gbps with RefClk=100.00 MHz
    //{ 10, 4, 2, 2, 1, 0, 0x0d, 250.00}, // 2.000 Gbps with RefClk=250.00 MHz
    //{  7, 5, 2, 1, 1, 3, 0x0d, 156.25}, // 3.125 Gbps with RefClk=156.25 MHz
    //{  5, 5, 5, 1, 2, 3, 0x07, 125.00}, // 3.125 Gbps with RefClk=125.00 MHz
    {  9, 5, 2, 2, 1, 0, 0x0d, 212.50}, // 2.125 Gbps with RefClk=212.50 MHz
};

#define BIT_GTXRESET (1<<0)
#define BIT_RXRESET (1<<1)
#define BIT_RXRESETDONE (1<<2)
#define BIT_TXRESET (1<<3)
#define BIT_TXRESETDONE (1<<4)

/**
 * get link rate from PLL settings struct
 * @param cfg struct gtxpll_settings
 * @return link rate
 **/
double
LinkRateFromPllSettings
(
    gtxpll_settings cfg
);

void
initLibrorcInstances
(
    librorc::device **dev,
    uint32_t devnr,
    librorc::bar **bar,
    librorc::sysmon **sm
);

void
resetAllGtx
(
    librorc::bar *bar,
    uint32_t nchannels,
    uint32_t reset
);


void
resetAllQsfps
(
    librorc::sysmon *sm,
    uint32_t reset
);


void
configureAllGtx
(
    librorc::bar *bar,
    uint32_t nchannels,
    gtxpll_settings pllcfg
);

void
configureRefclk
(
    librorc::sysmon *sm,
    float refclk_freq
);

uint32_t
waitForLinkUp
(
    librorc::bar *bar,
    uint32_t nchannels
);

bool
waitForResetDone
(
    librorc::link *link
);

void
clearAllErrorCounters
(
    librorc::bar *bar,
    uint32_t nchannels
);

void
countDown
(
    librorc::bar *bar,
    int time
);

void
checkErrorCounters
(
    librorc::bar *bar,
    uint32_t nchannels,
    uint32_t device_number
);

#endif
