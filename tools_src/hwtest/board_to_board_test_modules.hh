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
