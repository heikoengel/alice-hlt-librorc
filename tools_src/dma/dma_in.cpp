/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2013-08-23
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
 * @brief
 * Open a DMA Channel and read out data
 *
 **/

#include <librorc.h>

#include "dma_handling.hh"

using namespace std;

DMA_ABORT_HANDLER

uint64_t
eventCallBack
(
    void                     *userdata,
    uint64_t                  event_id,
    librorc_event_descriptor  report,
    const uint32_t           *event,
    librorcChannelStatus     *channel_status
)
{
    librorc::event_sanity_checker *checker = (librorc::event_sanity_checker*)userdata;

    try{ checker->check(report, channel_status, event_id); }
    catch(...){ abort(); }
    return 0;
}



int main(int argc, char *argv[])
{
    char logdirectory[] = "/tmp";
    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])    &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

    if
    (
        (opts.esType == LIBRORC_ES_IN_HWPG) &&
        !checkEventSize(opts.eventSize, argv[0])
    )
    { exit(-1); }

    DMA_ABORT_HANDLER_REGISTER

    librorc_event_callback event_callback = eventCallBack;

    librorc::event_stream *eventStream = prepareEventStream(opts);
    if( !eventStream )
    { exit(-1); }

    eventStream->printDeviceStatus();

    eventStream->setEventCallback(event_callback);

    /** make clear what will be checked*/
    int32_t sanity_check_mask = CHK_SIZES|CHK_SOE|CHK_EOE;
    if(opts.useRefFile)
    { sanity_check_mask |= CHK_FILE; }
    else
    { sanity_check_mask |= (CHK_PATTERN|CHK_ID); }


    librorc::event_sanity_checker checker =
        (opts.useRefFile) /** is DDL reference file enabled? */
        ?   librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC, /** TODO */
                sanity_check_mask,
                logdirectory,
                opts.refname
            )
        :   librorc::event_sanity_checker
            (
                eventStream->m_eventBuffer,
                opts.channelId,
                PG_PATTERN_INC,
                sanity_check_mask,
                logdirectory
            ) ;

    uint64_t result = eventStream->eventLoop((void*)&checker);

    printFinalStatusLine
    (
        eventStream->m_channel_status,
        opts,
        eventStream->m_start_time,
        eventStream->m_end_time
    );

    /** Cleanup */
    delete eventStream;

    return result;
}
