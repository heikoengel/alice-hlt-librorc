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
