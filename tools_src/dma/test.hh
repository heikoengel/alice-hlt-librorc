void
dump_rb
(
    librorc_event_descriptor *reportbuffer,
    uint64_t i,
    uint32_t ch
)
{
    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
            "CH%2d - RB[%3ld]: calc_size=%08x\t"
            "reported_size=%08x\t"
            "offset=%lx\n",
            ch, i, reportbuffer->calc_event_size,
            reportbuffer->reported_event_size,
            reportbuffer->offset);
}


void dump_event(
    volatile uint32_t *eventbuffer,
    uint64_t offset,
    uint64_t len)
{
#ifdef SIM
  uint64_t i = 0;
  for(i=0;i<len;i++) {
    printf("%03ld: %08x\n",
        i, (uint32_t)*(eventbuffer + offset +i));
  }
  if(len&0x01) {
    printf("%03ld: %08x (dummy)\n", i,
        (uint32_t)*(eventbuffer + offset + i));
    i++;
  }
  printf("%03ld: EOE reported_event_size: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
#if DMA_MODE==128
  i++;
  printf("%03ld: EOE calc_event_size: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
  i++;
  printf("%03ld: EOE dummy: %08x\n", i,
      (uint32_t)*(eventbuffer + offset + i));
#endif
#endif
}



int event_sanity_check
(
    librorc_event_descriptor *reportbuffer,
    volatile uint32_t *eventbuffer,
    uint64_t i,
    uint32_t ch,
    uint64_t last_id,
    uint32_t pattern_mode,
    uint32_t check_mask,
    uint32_t *ddlref,
    uint64_t ddlref_size,
    uint64_t *event_id
)
{
        printf("in:calculated: 0x%x, in:reported: 0x%x\n",
               (reportbuffer->calc_event_size & 0x3fffffff),
               (reportbuffer->reported_event_size & 0x3fffffff)
              );

  uint64_t offset;
  uint32_t j;
  uint32_t *eb = NULL;
  uint64_t cur_event_id;
  int retval = 0;

  /** upper two bits of the sizes are reserved for flags */
  uint32_t reported_event_size = (reportbuffer->reported_event_size & 0x3fffffff);
  uint32_t calc_event_size = (reportbuffer->calc_event_size & 0x3fffffff);

  /** Bit31 of calc_event_size is read completion timeout flag */
  uint32_t timeout_flag = (reportbuffer->calc_event_size>>31);

  // compare calculated and reported event sizes
  if (check_mask & CHK_SIZES)
  {
      if (timeout_flag)
      {
          DEBUG_PRINTF(PDADEBUG_ERROR,
                  "CH%2d ERROR: Event[%ld] Read Completion Timeout\n",
                  ch, i);
          retval |= CHK_SIZES;
      } else if (calc_event_size != reported_event_size)
      {
          DEBUG_PRINTF(PDADEBUG_ERROR,
                  "CH%2d ERROR: Event[%ld] sizes do not match: \n"
                  "calculated: 0x%x, reported: 0x%x\n"
                  "offset=0x%lx, rbdm_offset=0x%lx\n", ch, i,
                  calc_event_size,reported_event_size,
                  reportbuffer->offset,
                  i*sizeof(librorc_event_descriptor) );
          retval |= CHK_SIZES;
      }
  }

  /// reportbuffer->offset is a multiple of bytes
  offset = reportbuffer->offset/4;
  eb = (uint32_t*)malloc((reported_event_size+4)*sizeof(uint32_t));
  if ( eb==NULL ) {
    perror("Malloc EB");
    return -1;
  }
  // local copy of the event.
  // TODO: replace this with something more meaningful
  memcpy(eb, (uint8_t *)eventbuffer + reportbuffer->offset,
      (reported_event_size+4)*sizeof(uint32_t));

  // sanity check on SOF
  if( (check_mask & CHK_SOE) &&
          ((uint32_t)*(eb)!=0xffffffff) ) {
      DEBUG_PRINTF(PDADEBUG_ERROR,
              "ERROR: Event[%ld][0]!=0xffffffff -> %08x? \n"
              "offset=%ld, rbdm_offset=%ld\n",
              i, (uint32_t)*(eb),
              reportbuffer->offset,
              i*sizeof(librorc_event_descriptor) );
      dump_event(eventbuffer, offset, reported_event_size);
      dump_rb(reportbuffer, i, ch);
      retval |= CHK_SOE;
  }

  if ( check_mask & CHK_PATTERN )
  {
    switch (pattern_mode)
    {
      /* Data pattern is a ramp */
      case PG_PATTERN_RAMP:

        // check PG pattern
        for (j=8;j<calc_event_size;j++) {
          if ( (uint32_t)*(eb + j) != j-8 ) {
              DEBUG_PRINTF(PDADEBUG_ERROR,
                      "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                      i, j, j-8, (uint32_t)*(eventbuffer + offset + j));
              dump_event(eventbuffer, offset, reported_event_size);
              dump_rb(reportbuffer, i, ch);
              retval |= CHK_PATTERN;
          }
        }
        break;

      default:
        printf("ERROR: specified unknown pattern matching algorithm\n");
        retval |= CHK_PATTERN;
    }
  }


  // compare with reference DDL file
  if ( check_mask & CHK_FILE )
  {
    if ( ((uint64_t)calc_event_size<<2) != ddlref_size )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR,
                "ERROR: Eventsize %lx does not match "
                "reference DDL file size %lx\n",
                ((uint64_t)calc_event_size<<2),
                ddlref_size);
        dump_event(eventbuffer, offset, reported_event_size);
        dump_rb(reportbuffer, i, ch);
        retval |= CHK_FILE;
    }



    for (j=0;j<calc_event_size;j++) {
        if ( eb[j] != ddlref[j] ) {
            DEBUG_PRINTF(PDADEBUG_ERROR,
                    "ERROR: Event[%ld][%d] expected %08x read %08x\n",
                    i, j, ddlref[j], eb[j]);
            dump_event(eventbuffer, offset, reported_event_size);
            dump_rb(reportbuffer, i, ch);
            retval |= CHK_FILE;
      }
    }
  }




  if ( check_mask & CHK_EOE )
  {
      /**
       * 32 bit DMA mode
       *
       * DMA data is written in multiples of 32 bit. A 32bit EOE word
       * is directly attached after the last event data word.
       * The EOE word contains the EOE status word received from the DIU
       **/

      if ( (uint32_t)*(eb + calc_event_size) != reported_event_size ) {
          DEBUG_PRINTF(PDADEBUG_ERROR,
                  "ERROR: could not find matching reported event size "
                  "at Event[%d] expected %08x found %08x\n",
                  j, calc_event_size,
                  (uint32_t)*(eb + j) );
          dump_event(eventbuffer, offset, calc_event_size);
          dump_rb(reportbuffer, i, ch);
          retval |= CHK_EOE;
      }

  } // CHK_EOE


  // get EventID from CDH:
  // lower 12 bits in CHD[1][11:0]
  // upper 24 bits in CDH[2][23:0]
  cur_event_id = (uint32_t)*(eventbuffer + offset + 2) & 0x00ffffff;
  cur_event_id <<= 12;
  cur_event_id |= (uint32_t)*(eventbuffer + offset + 1) & 0x00000fff;

  // make sure EventIDs increment with each event.
  // missing EventIDs are an indication of lost event data
  if ( (check_mask & CHK_ID) &&
          (cur_event_id & 0xfffffffff) !=
          ((last_id+1) & 0xfffffffff) ) {
      DEBUG_PRINTF(PDADEBUG_ERROR,
              "ERROR: CH%d - Invalid Event Sequence: last ID: %ld, "
              "current ID: %ld\n", ch, last_id, cur_event_id);
      dump_event(eventbuffer, offset, calc_event_size);
      dump_rb(reportbuffer, i, ch);
      retval |= CHK_ID;
  }

  /** return event ID to caller */
  *event_id = cur_event_id;

  if(eb)
  {
      free(eb);
  }
  return retval;
}
