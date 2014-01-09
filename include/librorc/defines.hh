#ifndef LIBRORC_DEFINES_H
#define LIBRORC_DEFINES_H

#include <time.h>
#include <sys/time.h>

#define LIBRARY_NAME librorc

enum librorc_verbosity_enum
{
    LIBRORC_VERBOSE_OFF = 0,
    LIBRORC_VERBOSE_ON  = 1
};

typedef enum librorc_verbosity_enum librorc_verbosity;

#define PAGE_MASK ~(sysconf(_SC_PAGESIZE) - 1)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)


/**
 * TODO: which operating modes do we want to have here?
 * further options:
 * HLT_IN:
 * - generate event data by SW: no DMA transfers involved
 *     - from software PG, e.g. LIBRORC_ES_IN_SWPG
 *     - from local file(s), e.g. LIBRORC_ES_IN_FILE
 * HLT_OUT:
 * - use external interface to get event data
 *
 * NOTE:
 * HLT-OUT HWPG does not need an instance of event_stream to run,
 * as no DMA data transfer is involved in this mode.
 * */
typedef enum
{
    /** HLT-IN */
    LIBRORC_ES_IN_GENERIC, /** no source specific intializations */
    LIBRORC_ES_IN_DDL, /** use DIU as data source */
    LIBRORC_ES_IN_HWPG, /** use hardware PG as data source */
    /** HLT-OUT */
    LIBRORC_ES_OUT_GENERIC,
    LIBRORC_ES_OUT_SWPG, /** use software based PG as data source */
    LIBRORC_ES_OUT_FILE, /** use file as data source */
    /** NOT DEFINED */
    LIBRORC_ES_BOTH      /** bufferstats tool does need this */
} LibrorcEsType;


/**
 * gettimeofday_diff
 * @param time1 earlier timestamp
 * @param time2 later timestamp
 * @return time difference in seconds as double
**/
inline
double
gettimeofdayDiff
(
    timeval time1,
    timeval time2
)
{
    timeval diff;
    diff.tv_sec = time2.tv_sec - time1.tv_sec;
    diff.tv_usec = time2.tv_usec - time1.tv_usec;
    while(diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec -= 1;
    }

    return (double)((double)diff.tv_sec + (double)((double)diff.tv_usec / 1000000));
}

#endif /** LIBRORC_DEFINES_H */
