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

#include <librorc/error.hh>

namespace LIBRARY_NAME {

const errmsg_t table[] = {
    /** device **/
    {LIBRORC_DEVICE_ERROR_BUFFER_FREEING_FAILED, "PDA device failed to free librorc buffers"},
    {LIBRORC_DEVICE_ERROR_PDA_KMOD_MISMATCH, "PDA kernel adapter missing or version mismatch"},
    {LIBRORC_DEVICE_ERROR_PDA_VERSION_MISMATCH, "libpda version mismatch"},
    {LIBRORC_DEVICE_ERROR_PDADOP_FAILED, "failed to get PDA device operator"},
    {LIBRORC_DEVICE_ERROR_PDADEV_FAILED, "failed to get PDA PCI device"},
    {LIBRORC_DEVICE_ERROR_PDAGET_FAILED, "failed to get PCI params from PDA"},

    /** buffer**/
    {LIBRORC_BUFFER_ERROR_INVALID_SIZE, "requested buffer with invalid size"},
    {LIBRORC_BUFFER_ERROR_GETLENGTH_FAILED, "failed to get buffer size"},
    {LIBRORC_BUFFER_ERROR_GETMAP_FAILED, "failed to get buffer map"},
    {LIBRORC_BUFFER_ERROR_GETLIST_FAILED, "failed to get scatter-gather-list"},
    {LIBRORC_BUFFER_ERROR_DELETE_FAILED, "failed to delete buffer"},
    {LIBRORC_BUFFER_ERROR_ALLOC_FAILED, "failed to allocate buffer"},
    {LIBRORC_BUFFER_ERROR_WRAPMAP_FAILED, "failed to overmap buffer"},
    {LIBRORC_BUFFER_ERROR_GETBUF_FAILED, "failed to find buffer"},
    {LIBRORC_BUFFER_ERROR_GETMAPTWO_FAILED, "failed to get wrap-map state"},

    /** bar **/
    {LIBRORC_BAR_ERROR_CONSTRUCTOR_FAILED, "failed to get bar mapping"},

    /** event_stream **/
    {LIBRORC_EVENT_STREAM_ERROR_CHANNEL_NOT_AVAIL, "Requested channel not available in FW"},
    {LIBRORC_EVENT_STREAM_ERROR_CHANNEL_BUSY, "DMA Engine is already enabled"},
    {LIBRORC_EVENT_STREAM_ERROR_INV_ES_TYPE, "Invalid event stream type"},
    {LIBRORC_EVENT_STREAM_ERROR_ES_TYPE_NOT_AVAIL, "Requested event stream type not available in FW"},
    {LIBRORC_EVENT_STREAM_ERROR_INV_BUFFER_ID, "invalid/odd librorc buffer ID for event buffer"},
    {LIBRORC_EVENT_STREAM_ERROR_BUFFER_NOT_INITIALIZED, "cannot configure DMA engine with uninitialized buffers"},
    {LIBRORC_EVENT_STREAM_ERROR_BUFFER_SGLIST_EXCEEDED, "scatter-gather list does not fit into onboard buffer"},
    {LIBRORC_EVENT_STREAM_ERROR_DMA_CONFIG_FAILED, "unknown error configuring DMA engine"},
    {LIBRORC_EVENT_STREAM_ERROR_STS_GET_FAILED, "failed to get SysV SHM for librorc ChannelStatus"},
    {LIBRORC_EVENT_STREAM_ERROR_STS_ATTACH_FAILED, "failed to attach to SysV SHM for librorc ChannelStatus"},
    {LIBRORC_EVENT_STREAM_ERROR_STS_MALLOC_FAILED, "Failed to allocate memory for librorc ChannelStatus"},
    {LIBRORC_EVENT_STREAM_ERROR_STS_NOT_INITIALIZED, "librorc ChannelStatus is not initialized"},

    /** sysmon **/
    {LIBRORC_SYSMON_ERROR_CONSTRUCTOR_FAILED, "parent bar not initialized"},
    {LIBRORC_SYSMON_ERROR_I2C_RXACK, "failed to read from I2C"},
    {LIBRORC_SYSMON_ERROR_I2C_INVALID_PARAM, "invalid I2C chain selected"},
    {LIBRORC_SYSMON_ERROR_DATA_REPLAY_TIMEOUT, "data replay timeout"},
    {LIBRORC_SYSMON_ERROR_DATA_REPLAY_INVALID, "invalid replay channel selected"},

    /** refclk **/
    {LIBRORC_REFCLK_ERROR_CONSTRUCTOR_FAILED, "parent sysmon not initialized"},
    {LIBRORC_REFCLK_ERROR_INVALID_PARAMETER, "failed to find config for requested clock frequencies"},
};

const ssize_t table_len = sizeof(table) / sizeof(errmsg_t);

const char *errMsg(int err) {
  for (int i = 0; i < table_len; i++) {
    if (table[i].errcode == err) {
      return table[i].msg;
    }
  }
  return "unknown";
}

}
