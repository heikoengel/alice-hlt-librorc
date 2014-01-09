/**
 * @file board_to_board_test_modules.cpp
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

#include "board_to_board_test_modules.hh"

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
    gtxpll_settings cfg
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
    uint32_t gtxctrl = link->packetizer(RORC_REG_GTX_ASYNC_CFG);
    /** clear all reset bits (0,1,3) */
    gtxctrl &= ~(BIT_GTXRESET|BIT_RXRESET|BIT_TXRESET);
    /** set new reset bits */
    gtxctrl |= (reset & (BIT_GTXRESET|BIT_RXRESET|BIT_TXRESET));
    /** write back */
    link->setPacketizer( RORC_REG_GTX_ASYNC_CFG, gtxctrl);
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
            double dfeEye = (link->GTX(RORC_REG_GTX_RXDFE)>>21 & 0x1f)*200.0/31.0;
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
    uint32_t reset
)
{
    /** get number of links */
    uint32_t nchannels = bar->get32(RORC_REG_TYPE_CHANNELS) & 0xffff;
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
    gtxpll_settings pllcfg
)
{
    /** get number of links */
    uint32_t nchannels = bar->get32(RORC_REG_TYPE_CHANNELS) & 0xffff;
    for ( uint32_t i=0; i<nchannels; i++)
    {
        librorc::link *link = new librorc::link(bar, i);
        /** Write new PLL config */
        link->drpSetPllConfig(pllcfg);
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
    refclkopts cur_opts = rc->getCurrentOpts( LIBRORC_REFCLK_DEFAULT_FOUT );
        
    /** get new values for desired frequency */
    refclkopts new_opts = rc->getNewOpts(refclk_freq, cur_opts.fxtal);

    /** write new configuration to device */
    rc->setOpts(new_opts);

    delete rc;
}


uint32_t
waitForLinkUp
(
    librorc::bar *bar
)
{
    uint32_t lnkup = 0;
    uint32_t nchannels = bar->get32(RORC_REG_TYPE_CHANNELS) & 0xffff;
    uint32_t chmask = (1<<nchannels)-1;
    uint32_t timeout = WAIT_FOR_LINK_UP_TIMEOUT;

    while ( lnkup != chmask )
    {
        for ( uint32_t i=0; i<nchannels; i++ )
        {
            librorc::link *link = new librorc::link(bar, i);
            if ( link->isGtxDomainReady() && (link->GTX(RORC_REG_GTX_CTRL) & 1) )
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
    librorc::bar *bar
)
{
    uint32_t nchannels = bar->get32(RORC_REG_TYPE_CHANNELS) & 0xffff;
    for ( uint32_t i=0; i<nchannels; i++ )
    {
        librorc::link *link = new librorc::link(bar, i);
        if ( link->isGtxDomainReady() )
        {
            link->clearAllGtxErrorCounters();
            link->setGTX(RORC_REG_GTX_ERROR_CNT, 0);
        }
        delete link;
    }
}


