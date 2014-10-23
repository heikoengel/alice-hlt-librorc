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

#include "board_to_board_test_modules.hh"
#include "boardtest_modules.hh"

using namespace std;

void
initLibrorcInstances
(
    librorc::device **dev,
    uint32_t devnr,
    librorc::bar **bar,
    librorc::sysmon **sm
)
{
    /** Instantiate Device */
    try{
        *dev = new librorc::device(devnr);
    }
    catch(...)
    {
        cout << "FATAL: Failed to intialize device " << devnr << endl;
        abort();
    }

    /** Instantiate a new bar */
    try
    {
        *bar = new librorc::rorc_bar(*dev, 1);
    }
    catch(...)
    {
        cout << "FATAL: Device " << devnr
            << "failed to initialize BAR." << endl;
        abort();
    }

    /** Instantiate SystemManager */
    try
    {
        *sm = new librorc::sysmon(*bar);
    }
    catch(...)
    {
        cout << "FATAL: Device " << devnr
            << " Sysmon init failed!" << endl;
        abort();
    }

    /** Check Firmware Type */
    if ( !(*sm)->firmwareIsHltHardwareTest() )
    {
        cout << "FATAL: Device " << devnr
            << " Firmware is not for HW test!" << endl;
        abort();
    }
}


double
LinkRateFromPllSettings
(
    librorc::gtxpll_settings cfg
)
{
    return cfg.refclk * cfg.n1 * cfg.n2 / cfg.m * 2.0 / cfg.d / 1000.0;
}

void
resetGtx
(
    librorc::link *link,
    uint32_t reset
    )
{
    uint32_t gtxctrl = link->pciReg(RORC_REG_GTX_ASYNC_CFG);
    /** clear all reset bits (0,1,3) */
    gtxctrl &= ~(BIT_GTXRESET|BIT_RXRESET|BIT_TXRESET);
    /** set new reset bits */
    gtxctrl |= (reset & (BIT_GTXRESET|BIT_RXRESET|BIT_TXRESET));
    /** write back */
    link->setPciReg( RORC_REG_GTX_ASYNC_CFG, gtxctrl);
}



bool
waitForResetDone
(
    librorc::link *link
)
{
    uint32_t timeout = 0;
    while ( timeout < WAIT_FOR_RESET_DONE_TIMEOUT )
    {
        if ( link->isGtxDomainReady() )
        {
            /** make sure DFE eye is above threshold */
            double dfeEye = (link->gtxReg(RORC_REG_GTX_RXDFE)>>21 & 0x1f)*200.0/31.0;
            return (dfeEye > GTX_DFE_EYE_DAC_MIN);
        }
        usleep(100);
        timeout++;
    }
    return false;
}



void
resetAllGtx
(
    librorc::bar *bar,
    uint32_t nchannels,
    uint32_t reset
)
{
    for ( uint32_t i=0; i<nchannels; i++)
    {
        librorc::link *link = new librorc::link(bar, i);
        resetGtx( link, reset );
        if ( !reset )
        {
            uint32_t retry = WAIT_FOR_RESET_DONE_RETRY;
            while( !waitForResetDone(link) )
            {
                resetGtx( link, reset );
                if(retry==0)
                {
                    cout << "WARNING: Link " << i
                         << " waitForResetDone failed after "
                         << WAIT_FOR_RESET_DONE_RETRY
                         << " retries." << endl;
                    break;
                }
                retry--;
            }
        }
        delete link;
    }
}


void
resetAllQsfps
(
    librorc::sysmon *sm,
    uint32_t reset
)
{
    for ( int i=0; i<3; i++ )
    {
        if ( sm->qsfpIsPresent(i) )
        {
            sm->qsfpSetReset(i, reset);
        }
    }
}



void
configureAllGtx
(
    librorc::bar *bar,
    uint32_t nchannels,
    librorc::gtxpll_settings pllcfg
)
{
    for ( uint32_t i=0; i<nchannels; i++)
    {
        librorc::link *link = new librorc::link(bar, i);
        librorc::gtx *gtx = new librorc::gtx(link);
        /** Write new PLL config */
        gtx->drpSetPllConfig(pllcfg);
        delete gtx;
        delete link;
    }
}




void
configureRefclk
(
    librorc::sysmon *sm,
    float refclk_freq
)
{
    librorc::refclk *rc = new librorc::refclk(sm);

    /** Recall initial conditions */
    rc->setRFMCtrl( M_RECALL );

    /** Wait for RECALL to complete */
    rc->waitForClearance( M_RECALL );

    /** get current configuration */
    librorc::refclkopts cur_opts = rc->getCurrentOpts( LIBRORC_REFCLK_DEFAULT_FOUT );
        
    /** get new values for desired frequency */
    librorc::refclkopts new_opts = rc->getNewOpts(refclk_freq, cur_opts.fxtal);

    /** write new configuration to device */
    rc->setOpts(new_opts);

    delete rc;
}


uint32_t
waitForLinkUp
(
    librorc::bar *bar,
    uint32_t nchannels
)
{
    uint32_t lnkup = 0;
    uint32_t chmask = (1<<nchannels)-1;
    uint32_t timeout = WAIT_FOR_LINK_UP_TIMEOUT;

    while ( lnkup != chmask )
    {
        for ( uint32_t i=0; i<nchannels; i++ )
        {
            librorc::link *link = new librorc::link(bar, i);
            if ( link->isGtxDomainReady() && (link->gtxReg(RORC_REG_GTX_CTRL) & 1) )
            {
                lnkup |= (1<<i);
            }
            delete link;
        }

        //break on timeout
        if ( timeout==0 )
        {
            break;
        }
        timeout--;
        usleep(100);
    }

    return lnkup;
}


void
clearAllErrorCounters
(
    librorc::bar *bar,
    uint32_t nchannels
)
{
    for ( uint32_t i=0; i<nchannels; i++ )
    {
        librorc::link *link = new librorc::link(bar, i);
        if ( link->isGtxDomainReady() )
        {
            link->clearAllGtxErrorCounters();
            link->setGtxReg(RORC_REG_GTX_ERROR_CNT, 0);
        }
        delete link;
    }
}

void
countDown
(
    librorc::bar *bar,
    int time
)
{
    timeval start_time, cur_time, last_time;
    bar->gettime(&start_time, 0);
    cur_time = start_time;
    last_time = start_time;

    while( gettimeofdayDiff(start_time, cur_time) < time )
    {
        bar->gettime(&cur_time, 0);
        if( gettimeofdayDiff(last_time, cur_time) > COUNTDOWN_INTERVAL )
        {
            cout << "\r" << (int)(time - gettimeofdayDiff(start_time, cur_time))
                 << " sec. remaining..." << flush;
            last_time = cur_time;
        }
        usleep(1000);
    }
    cout << "\r";
}


void
checkErrorCounters
(
    librorc::bar *bar,
    uint32_t nchannels,
    uint32_t device_number
)
{
    uint32_t errors = 0;
    for ( uint32_t i=0; i<nchannels; i++ )
    {
        librorc::link *link = new librorc::link(bar, i);
        errors |= checkLinkState(link, i);
        delete link;
    }
    if ( errors )
    {
        cout << "ERROR: Device " << device_number
             << " link test failed." << endl;
    }
    else
    {
        cout << "INFO: Device " << device_number
             << " link teset passed." << endl;
    }
}



