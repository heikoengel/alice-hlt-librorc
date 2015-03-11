/**
 * Copyright (c) 2015, Heiko Engel <hengel@cern.ch>
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
 *     * Neither the name of University Frankfurt, CERN nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL A COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef LIBRORC_ERROR_H
#define LIBRORC_ERROR_H

#include <librorc/defines.hh>

namespace LIBRARY_NAME {

// device
#define LIBRORC_DEVICE_ERROR_BUFFER_FREEING_FAILED 0x0001
#define LIBRORC_DEVICE_ERROR_PDA_KMOD_MISMATCH 0x0002
#define LIBRORC_DEVICE_ERROR_PDA_VERSION_MISMATCH 0x0003
#define LIBRORC_DEVICE_ERROR_PDADOP_FAILED 0x0004
#define LIBRORC_DEVICE_ERROR_PDADEV_FAILED 0x0005
#define LIBRORC_DEVICE_ERROR_PDAGET_FAILED 0x0006

// buffer
#define LIBRORC_BUFFER_ERROR_INVALID_SIZE 0x1001
#define LIBRORC_BUFFER_ERROR_GETLENGTH_FAILED 0x1002
#define LIBRORC_BUFFER_ERROR_GETMAP_FAILED 0x1003
#define LIBRORC_BUFFER_ERROR_GETLIST_FAILED 0x1004
#define LIBRORC_BUFFER_ERROR_DELETE_FAILED 0x1005
#define LIBRORC_BUFFER_ERROR_ALLOC_FAILED 0x1006
#define LIBRORC_BUFFER_ERROR_WRAPMAP_FAILED 0x1007
#define LIBRORC_BUFFER_ERROR_GETBUF_FAILED 0x1008
#define LIBRORC_BUFFER_ERROR_GETMAPTWO_FAILED 0x1009

// bar
#define LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED 0x2001

// event_stream
#define LIBRORC_EVENT_STREAM_ERROR_CHANNEL_NOT_AVAIL 0x3001
#define LIBRORC_EVENT_STREAM_ERROR_CHANNEL_BUSY 0x3002
#define LIBRORC_EVENT_STREAM_ERROR_INV_ES_TYPE 0x3003
#define LIBRORC_EVENT_STREAM_ERROR_ES_TYPE_NOT_AVAIL 0x3004
#define LIBRORC_EVENT_STREAM_ERROR_INV_BUFFER_ID 0x3005
#define LIBRORC_EVENT_STREAM_ERROR_BUFFER_NOT_INITIALIZED 0x3006
#define LIBRORC_EVENT_STREAM_ERROR_BUFFER_SGLIST_EXCEEDED 0x3007
#define LIBRORC_EVENT_STREAM_ERROR_DMA_CONFIG_FAILED 0x3008
#define LIBRORC_EVENT_STREAM_ERROR_STS_GET_FAILED 0x3009
#define LIBRORC_EVENT_STREAM_ERROR_STS_ATTACH_FAILED 0x300a
#define LIBRORC_EVENT_STREAM_ERROR_STS_MALLOC_FAILED 0x300b
#define LIBRORC_EVENT_STREAM_ERROR_STS_NOT_INITIALIZED 0x300c

typedef struct {
    int errcode;
    const char *msg;
} errmsg_t;

const char *errMsg(int err);
}

#endif
