/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>, Dominic Eschweiler <eschweiler@fias.uni-frankfurt.de>
 * @date 2012-12-17
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
 * Open DMA Channel sourced by optical DDL
 *
 * */

#include <librorc.h>
#include <event_handling.h>

#include "dma_handling.hh"

using namespace std;

int done = 0;

void abort_handler( int s )
{
    printf("Caught signal %d\n", s);
    if( done==1 )
    { exit(-1); }
    else
    { done = 1; }
}



int main( int argc, char *argv[])
{
    int result = 0;

    DMAOptions opts = evaluateArguments(argc, argv);

    if
    (!(
        checkDeviceID(opts.deviceId, argv[0])   &&
        checkChannelID(opts.channelId, argv[0])
    ) )
    { exit(-1); }

//ready

    /** catch CTRL+C for abort */
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = abort_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    channelStatus *chstats
        = prepareSharedMemory(opts);
    if(chstats == NULL)
    { exit(-1); }

//THAT'S GTX SPECIFIC !!!
    /** get optional DDL reference file */
    int ddlref_fd = -1;
    uint64_t ddlref_size = 0;
    uint32_t *ddlref = NULL;
    struct stat ddlref_stat;
    if(opts.useRefFile==true)
    {
        ddlref_fd = open(opts.refname, O_RDONLY);
        if(ddlref_fd<0)
        {
            perror("failed to open reference DDL file");
            abort();
        }

        /** get file size */
        if(fstat(ddlref_fd, &ddlref_stat)==-1)
        {
            perror("fstat DDL reference file");
            abort();
        }

        ddlref_size = ddlref_stat.st_size;

        ddlref = (uint32_t *)mmap(0, ddlref_size,
                PROT_READ, MAP_SHARED, ddlref_fd, 0);
        if(ddlref == MAP_FAILED)
        {
            perror("failed to mmap file");
            abort();
        }
    }
//THAT'S GTX SPECIFIC !!!


    /** create new device instance */
    librorc::device *dev = NULL;
    try
    { dev = new librorc::device(opts.deviceId); }
    catch(...)
    {
        printf("ERROR: failed to initialize device.\n");
        abort();
    }

    printf("Bus %x, Slot %x, Func %x\n", dev->getBus(),
            dev->getSlot(),dev->getFunc());

    /** bind to BAR1 */
    librorc::bar *bar1 = NULL;
    try
    {
    #ifdef SIM
        bar1 = new librorc::sim_bar(dev, 1);
    #else
        bar1 = new librorc::rorc_bar(dev, 1);
    #endif
    }
    catch(...)
    {
        printf("ERROR: failed to initialize BAR1.\n");
        abort();
    }

    cout << "FirmwareDate: " << setw(8) << hex
         << bar1->get32(RORC_REG_FIRMWARE_DATE);
    cout << "FirmwareRevision: " << setw(8) << hex
         << bar1->get32(RORC_REG_FIRMWARE_REVISION);

    /** check if requested channel is implemented in firmware */
    if(opts.channelId >= (int32_t)(bar1->get32(RORC_REG_TYPE_CHANNELS) & 0xffff) )
    {
        printf("ERROR: Requsted channel %d is not implemented in "
                "firmware - exiting\n", opts.channelId);
        abort();
    }

    /** Create new DMA event buffer */
    librorc::buffer *ebuf;
    try
    { ebuf = new librorc::buffer(dev, EBUFSIZE, (2*opts.channelId), 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: ebuf->allocate");
        abort();
    }

    /** create new DMA report buffer */
    librorc::buffer *rbuf;
    try
    { rbuf = new librorc::buffer(dev, RBUFSIZE, 2*opts.channelId+1, 1, LIBRORC_DMA_FROM_DEVICE); }
    catch(...)
    {
        perror("ERROR: rbuf->allocate");
        abort();
    }

    /** clear report buffer */
    struct librorc_event_descriptor *reportbuffer
        = (struct librorc_event_descriptor *)rbuf->getMem();
    memset(reportbuffer, 0, rbuf->getMappingSize());

    /** Create DMA channel and bind channel to BAR1 */
    librorc::dma_channel *ch = new librorc::dma_channel();
    ch->init(bar1, opts.channelId);

    /** prepare EventBufferDescriptorManager with scatter-gather list */
    if(ch->prepareEB(ebuf) < 0)
    {
        perror("prepareEB()");
        abort();
    }

    /** prepare ReportBufferDescriptorManager with scatter-gather list */
    if(ch->prepareRB(rbuf) < 0)
    {
        perror("prepareRB()");
        abort();
    }

    /** set MAX_PAYLOAD, buffer sizes, #sgEntries, ... */
    if(ch->configureChannel(ebuf, rbuf, MAX_PAYLOAD) < 0)
    {
        perror("configureChannel()");
        abort();
    }

    /** enable BDMs */
    ch->setEnableEB(1);
    ch->setEnableRB(1);

    /** enable DMA channel */
    ch->setDMAConfig( ch->getDMAConfig() | 0x01 );


    /**
     * wait for GTX domain to be ready
     * read asynchronous GTX status
     * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
     * & !gtx_in_rst
     */
    printf("Waiting for GTX to be ready...\n");
    while( (ch->getPKT(RORC_REG_GTX_ASYNC_CFG) & 0x174) != 0x074 )
        { usleep(100); }

//GTX SPECIFIC
    /** set ENABLE, activate flow control (DIU_IF:busy) */
    ch->setGTX(RORC_REG_DDL_CTRL, 0x00000003);

    /** wait for riLD_N='1' */
    printf("Waiting for LD_N to deassert...\n");
    while( (ch->getGTX(RORC_REG_DDL_CTRL) & 0x20) != 0x20 )
        {usleep(100);}

    /** clear DIU_IF IFSTW, CTSTW */
    ch->setGTX(RORC_REG_DDL_IFSTW, 0);
    ch->setGTX(RORC_REG_DDL_CTSTW, 0);

    /** send EOBTR to close any open transaction */
    ch->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

    /** wait for command transmission status word (CTSTW) from DIU */
    uint32_t ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    while( ctstw == 0xffffffff )
    {
        usleep(100);
        ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    }

    uint8_t ddl_trn_id = 2 & 0x0f;

    printf("DIU CTSTW: %08x\n", ctstw);
    printf("DIU IFSTW: %08x\n", ch->getGTX(RORC_REG_DDL_IFSTW));

    /** clear DIU_IF IFSTW */
    ch->setGTX(RORC_REG_DDL_IFSTW, 0);
    ch->setGTX(RORC_REG_DDL_CTSTW, 0);

    /** send RdyRx to SIU */
    ch->setGTX(RORC_REG_DDL_CMD, 0x00000014);

    /** wait for command transmission status word (CTSTW) from DIU */
    ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    while( ctstw == 0xffffffff )
    {
        usleep(100);
        ctstw = ch->getGTX(RORC_REG_DDL_CTSTW);
    }
    ddl_trn_id = (ddl_trn_id+2) & 0x0f;

    /** clear DIU_IF IFSTW */
    ch->setGTX(RORC_REG_DDL_IFSTW, 0);
    ch->setGTX(RORC_REG_DDL_CTSTW, 0);
//GTX SPECIFIC


    /** capture starting time */
    timeval start_time;
    bar1->gettime(&start_time, 0);
    timeval last_time = start_time;
    timeval cur_time = start_time;

    uint64_t last_bytes_received = 0;
    uint64_t last_events_received = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    int32_t sanity_checks;
    while( !done )
    {

        /** this can be aborted by abort_handler(),
         * triggered from CTRL+C
         */

        if(ddlref && ddlref_size)
            {sanity_checks = CHK_FILE;}
        else
            {sanity_checks = CHK_SIZES;}

        result =  handle_channel_data(
                rbuf,
                ebuf,
                ch, // channel struct
                chstats, // stats struct
                sanity_checks, // do sanity check
                ddlref,
                ddlref_size);

        if(result < 0)
        {
            printf("handle_channel_data failed for channel %d\n", opts.channelId);
            abort();
        }
        else if(result==0)
        {
            /** no events available */
            usleep(100);
        }

        bar1->gettime(&cur_time, 0);

        /** print status line each second */
        if(gettimeofday_diff(last_time, cur_time)>STAT_INTERVAL)
        {
            printf("Events: %10ld, DataSize: %8.3f GB ",
                    chstats->n_events,
                    (double)chstats->bytes_received/(double)(1<<30));

            if(chstats->bytes_received-last_bytes_received)
            {
                printf(" DataRate: %9.3f MB/s",
                        (double)(chstats->bytes_received-last_bytes_received)/
                        gettimeofday_diff(last_time, cur_time)/(double)(1<<20));
            }
            else
            {
                printf(" DataRate: -");
                /** dump_dma_state(ch); */
                /** dump_diu_state(ch); */
            }

            if(chstats->n_events - last_events_received)
            {
                printf(" EventRate: %9.3f kHz/s",
                        (double)(chstats->n_events-last_events_received)/
                        gettimeofday_diff(last_time, cur_time)/1000.0);
            }
            else
            {
                printf(" EventRate: -");
            }
            printf(" Errors: %ld\n", chstats->error_count);
            last_time = cur_time;
            last_bytes_received = chstats->bytes_received;
            last_events_received = chstats->n_events;
        }

    }

    /** EOR */
    timeval end_time;
    bar1->gettime(&end_time, 0);

    /** print summary */
    printf("%ld Byte / %ld events in %.2f sec"
            "-> %.1f MB/s.\n",
            (chstats->bytes_received), chstats->n_events,
            gettimeofday_diff(start_time, end_time),
            ((float)chstats->bytes_received/
             gettimeofday_diff(start_time, end_time))/(float)(1<<20) );

    if(!chstats->set_offset_count) //avoid DivByZero Exception
    {
        printf("CH%d: No Events\n", opts.channelId);
    }
    else
        printf("CH%d: Events %ld, max_epi=%ld, min_epi=%ld, "
                "avg_epi=%ld, set_offset_count=%ld\n", opts.channelId,
                chstats->n_events, chstats->max_epi,
                chstats->min_epi,
                chstats->n_events/chstats->set_offset_count,
                chstats->set_offset_count);

    /** check if link is still up: LD_N == 1 */
    if ( ch->getGTX(RORC_REG_DDL_CTRL) & (1<<5) )
    {
        /** disable BUSY -> drop current data in chain */
        ch->setGTX(RORC_REG_DDL_CTRL, 0x00000001);

        /** wait for LF_N to go high */
        while(!(ch->getGTX(RORC_REG_DDL_CTRL) & (1<<4)))
            {usleep(100);}

        /** clear DIU_IF IFSTW */
        ch->setGTX(RORC_REG_DDL_IFSTW, 0);
        ch->setGTX(RORC_REG_DDL_CTSTW, 0);

        /** Send EOBTR command */
        ch->setGTX(RORC_REG_DDL_CMD, 0x000000b4); //EOBTR

        /** wait for command transmission status word (CTST)
         * in response to the EOBTR:
         * STS[7:4]="0000"
         */
        while(ch->getGTX(RORC_REG_DDL_CTSTW) & 0xf0)
            {usleep(100);}

        /** disable DIU_IF */
        ch->setGTX(RORC_REG_DDL_CTRL, 0x00000000);
    }
    else
    { /**link is down -> unable to send EOBTR */
        printf("Link is down - unable to send EOBTR\n");
    }

    /** disable EBDM -> no further sg-entries to PKT */
    ch->setEnableEB(0);

    /** wait for pending transfers to complete (dma_busy->0) */
    while( ch->getDMABusy() )
        {usleep(100);}

    /** disable RBDM */
    ch->setEnableRB(0);

    /** reset DFIFO, disable DMA PKT */
    ch->setDMAConfig(0X00000002);

    /** clear reportbuffer */
    memset(reportbuffer, 0, rbuf->getMappingSize());

    if(chstats)
    {
        shmdt(chstats);
        chstats = NULL;
    }

    if (ddlref)
    {
        if( munmap(ddlref, ddlref_size)==-1 )
        { perror("ERROR: failed to unmap file"); }
    }

    if (ddlref_fd>=0)
        close(ddlref_fd);

    if (ch)
        delete ch;
    if (ebuf)
        delete ebuf;
    if (rbuf)
        delete rbuf;

    if (bar1)
        delete bar1;
    if (dev)
        delete dev;

    return result;
}
