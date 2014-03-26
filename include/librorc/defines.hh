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


typedef enum
{
    LIBRORC_ES_TO_HOST,
    LIBRORC_ES_TO_DEVICE,
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
