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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL A
 * COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 **/

#include <pthread.h>
#include <errno.h>
#include <sys/shm.h>
#include <cstdlib>

#include <librorc/event_stream.hh>

#include <librorc/error.hh>
#include <librorc/buffer.hh>
#include <librorc/device.hh>
#include <librorc/bar.hh>
#include <librorc/sysmon.hh>
#include <librorc/link.hh>
#include <librorc/dma_channel.hh>

#include <librorc/fastclusterfinder.hh>
#include <librorc/diu.hh>
#include <librorc/ddl.hh>
#include <librorc/siu.hh>
#include <librorc/patterngenerator.hh>

namespace LIBRARY_NAME {

#define EVENT_INDEX_UNDEFINED 0xffffffffffffffff

event_stream::event_stream(uint32_t deviceId, uint32_t channelId,
                           EventStreamDirection esType) {
  m_deviceId = deviceId;
  m_called_with_bar = false;
  m_channelId = channelId;
  m_esType = esType;

  initMembers();
  prepareSharedMemory();
}

event_stream::event_stream(device *dev, bar *bar, uint32_t channelId,
                           EventStreamDirection esType) {
  m_dev = dev;
  m_bar1 = bar;
  m_deviceId = dev->getDeviceId();
  m_called_with_bar = true;
  m_channelId = channelId;
  m_esType = esType;

  initMembers();
  prepareSharedMemory();
}

int event_stream::initializeDma(uint64_t bufferId, uint64_t bufferSize) {
  int result = initializeDmaBuffers(bufferId, bufferSize);
  if (result != 0) {
    /** possible return values:
     * EINVAL: odd bufferId
     * -1    : librorc::buffer() constructor failed
     **/
    return result;
  }
  result = initializeDmaChannel();
  if (result != 0) {
    /** possible return values:
     * ENODEV: on eventbuffer==NULL or reportbuffer==NULL
     * EFBIG : sglist does not fit into BD-RAM
     **/
    return result;
  }
  return 0;
}

void event_stream::initMembers() {
  m_raw_event_buffer = NULL;
  m_eventBuffer = NULL;
  m_reportBuffer = NULL;
  m_release_map = NULL;
  m_receive_index = EVENT_INDEX_UNDEFINED;
  m_release_index = 0;

  if (!m_called_with_bar) {
    m_dev = new device(m_deviceId);
    m_bar1 = new bar(m_dev, 1);
  }

  /** check if selected channel is available in current FW */
  m_sm = new sysmon(m_bar1);
  if (m_channelId >= m_sm->numberOfChannels()) {
    throw LIBRORC_EVENT_STREAM_ERROR_CHANNEL_NOT_AVAIL;
  }

  m_fwtype = m_sm->firmwareType();
  m_link = new link(m_bar1, m_channelId);
  m_channel = new dma_channel(m_link);
  m_linktype = m_link->linkType();

  if (m_channel->getEnable()) {
    throw LIBRORC_EVENT_STREAM_ERROR_CHANNEL_BUSY;
  }

  // get default pciePacketSize from device
  switch (m_esType) {
  case kEventStreamToDevice:
    m_pciePacketSize = m_dev->maxReadRequestSize();
    break;
  case kEventStreamToHost:
    m_pciePacketSize = m_dev->maxPayloadSize();
    break;
  default:
    throw LIBRORC_EVENT_STREAM_ERROR_INV_ES_TYPE;
    break;
  }

  pthread_mutex_init(&m_releaseEnable, NULL);
  pthread_mutex_init(&m_getEventEnable, NULL);

  /** make sure requested channel is available for selected esType */
  checkLinkTypeCompatibility();
}

event_stream::~event_stream() {
  m_channel->disable();
  deleteParts();

  if (m_channel_status != NULL) {
#ifdef SHM
    shmdt(m_channel_status);
#else
#pragma message "Compiling without SHM"
    free(m_channel_status);
#endif
  }

  pthread_mutex_destroy(&m_releaseEnable);
  pthread_mutex_destroy(&m_getEventEnable);
}

void event_stream::deleteParts() {
  if (m_sm) {
    delete m_sm;
  }
  if (m_channel) {
    delete m_channel;
  }
  if (m_link) {
    delete m_link;
  }
  if (m_eventBuffer) {
    delete m_eventBuffer;
  }
  if (m_reportBuffer) {
    delete m_reportBuffer;
  }
  if (m_release_map) {
    delete[] m_release_map;
  }
  if (!m_called_with_bar) {
    if (m_bar1) {
      delete m_bar1;
    }
    if (m_dev) {
      delete m_dev;
    }
  }
}

void event_stream::checkLinkTypeCompatibility() {
  bool fwOK;
  switch (m_esType) {
  case kEventStreamToHost: {
    fwOK = (m_linktype != RORC_CFG_LINK_TYPE_SIU);
  } break;
  case kEventStreamToDevice: {
    fwOK = (m_linktype == RORC_CFG_LINK_TYPE_SIU);
  } break;
  default: { fwOK = false; } break;
  }

  if (!fwOK) {
    throw LIBRORC_EVENT_STREAM_ERROR_ES_TYPE_NOT_AVAIL;
  }
}

int event_stream::initializeDmaBuffers(uint64_t eventBufferId,
                                       uint64_t eventBufferSize) {
  // only allow even eventBufferIds becaus the report buffer ID
  // is always eventBufferId+1
  if (eventBufferId & 1) {
    return LIBRORC_EVENT_STREAM_ERROR_INV_BUFFER_ID;
  }

  try {
    // allocate a new buffer if a size was provided, else connect
    // to existing buffer
    if (eventBufferSize) {
      m_eventBuffer = new buffer(m_dev, eventBufferSize, eventBufferId, 1);
    } else {
      m_eventBuffer = new buffer(m_dev, eventBufferId, 1);
    }
  } catch (int e) {
    return e;
  }

  uint64_t reportBufferSize =
      (m_eventBuffer->getPhysicalSize() / m_pciePacketSize) *
      sizeof(EventDescriptor);
  try {
    // ReportBuffer uses by default EventBuffer-ID + 1
    m_reportBuffer =
        new buffer(m_dev, reportBufferSize, (eventBufferId + 1), 1);
  } catch (int e) {
    return e;
  }

  m_raw_event_buffer = (uint32_t *)(m_eventBuffer->getMem());
  m_reports = (EventDescriptor *)m_reportBuffer->getMem();
  m_max_rb_entries = m_reportBuffer->getMaxRBEntries();
  m_release_map = new bool[m_max_rb_entries];

  for (uint64_t i = 0; i < m_max_rb_entries; i++) {
    m_release_map[i] = false;
  }
  return 0;
}

int event_stream::overridePciePacketSize(uint32_t pciePacketSize) {
  if ((pciePacketSize == 0) || (pciePacketSize % 4)) {
    return -1;
  }
  if (m_esType == kEventStreamToHost) {
    // DMA to Host: 256B write requests max.
    if (pciePacketSize > 256) {
      return -1;
    }
  } else {
    // DMA to Device: 512B read requests max.
    if (pciePacketSize > 512) {
      return -1;
    }
  }

  m_pciePacketSize = pciePacketSize;
  if (m_channel) {
    m_channel->setPciePacketSize(pciePacketSize);
  }
  return 0;
}

int event_stream::initializeDmaChannel() {
  if (m_eventBuffer == NULL || m_reportBuffer == NULL) {
    return LIBRORC_EVENT_STREAM_ERROR_BUFFER_NOT_INITIALIZED;
  }

  int ret = m_channel->configure(m_eventBuffer, m_reportBuffer, m_esType,
                                 m_pciePacketSize);
  /** return values:
   * ENODEV: buffers not initialized
   * EFBIG: sg-list does not fit into BDRAM
   **/
  if (ret != 0) {
    if (ret == ENODEV) {
      return LIBRORC_EVENT_STREAM_ERROR_BUFFER_NOT_INITIALIZED;
    } else if (ret == EFBIG) {
      return LIBRORC_EVENT_STREAM_ERROR_BUFFER_SGLIST_EXCEEDED;
    } else {
      return LIBRORC_EVENT_STREAM_ERROR_DMA_CONFIG_FAILED;
    }
  }

  m_reportBuffer->clear();

  m_channel->enable();
  return 0;
}

void event_stream::prepareSharedMemory() {
  m_channel_status = NULL;
  char *shm = NULL;

#ifdef SHM
  int shID = shmget(SHM_KEY_OFFSET + m_deviceId * SHM_DEV_OFFSET + m_channelId,
                    sizeof(ChannelStatus), IPC_CREAT | 0666);
  if (shID == -1) {
    throw(LIBRORC_EVENT_STREAM_ERROR_STS_GET_FAILED);
  }

  /** attach to shared memory */
  shm = (char *)shmat(shID, 0, 0);
  if (shm == (char *)-1) {
    throw(LIBRORC_EVENT_STREAM_ERROR_STS_ATTACH_FAILED);
  }
#else
#pragma message "Compiling without SHM"
  shm = (char *)malloc(sizeof(ChannelStatus));
  if (shm == NULL) {
    throw(LIBRORC_EVENT_STREAM_ERROR_STS_MALLOC_FAILED);
  }
#endif

  m_channel_status = (ChannelStatus *)shm;
  clearSharedMemory();
}

void event_stream::clearSharedMemory() {
  if (m_channel_status == NULL) {
    throw(LIBRORC_EVENT_STREAM_ERROR_STS_NOT_INITIALIZED);
  }

  memset(m_channel_status, 0, sizeof(ChannelStatus));

  //m_channel_status->index = EVENT_INDEX_UNDEFINED;
  //m_channel_status->shadow_index = 0;
  m_channel_status->channel = m_channelId;
  m_channel_status->device = m_deviceId;
}

bool event_stream::getNextEvent(EventDescriptor **report,
                                const uint32_t **event, uint64_t *reference) {
  pthread_mutex_lock(&m_getEventEnable);

  uint64_t tmp_index = 0;
  if (m_receive_index == EVENT_INDEX_UNDEFINED) {
    tmp_index = 0;
  } else {
    tmp_index = (m_receive_index < m_max_rb_entries - 1) ? (m_receive_index + 1) : 0;
  }

  if (m_reports[tmp_index].reported_event_size == 0) {
    pthread_mutex_unlock(&m_getEventEnable);
    return false;
  }

  m_receive_index = tmp_index;
  *reference = m_receive_index;
  *report = &m_reports[m_receive_index];
  *event = getRawEvent(**report);
  m_channel_status->receive_offset = m_reports[tmp_index].offset;
  pthread_mutex_unlock(&m_getEventEnable);
  return true;
}

uint64_t event_stream::getNumberOfPendingReleases() {
  uint64_t numberOfEvents = 0;
  for (uint64_t i = 0; i < m_max_rb_entries; i++) {
    if (m_release_map[i]) {
      numberOfEvents++;
    }
  }
  return numberOfEvents;
}

uint64_t event_stream::getRingbufferFillCount() {
  if (m_receive_index == EVENT_INDEX_UNDEFINED) {
    return 0;
  } else if (m_receive_index >= m_release_index) {
    return m_receive_index - m_release_index;
  } else {
    return m_max_rb_entries - (m_release_index - m_receive_index);
  }
}

void event_stream::updateChannelStatus(EventDescriptor *report) {
  uint32_t calc_event_size = (report->calc_event_size & 0x3fffffff);
  m_channel_status->bytes_received += (calc_event_size << 2);
  m_channel_status->n_events++;
}

void event_stream::updateBufferOffsets() {
  if (m_release_map[m_release_index] != true) {
    return;
  }

  uint64_t report_buffer_offset;
  uint64_t event_buffer_offset;
  uint64_t release_index_start = m_release_index;
  uint64_t release_index_count = 0;

  while (m_release_map[m_release_index] == true) {
    m_release_map[m_release_index] = false;
    event_buffer_offset = m_reports[m_release_index].offset;
    report_buffer_offset = ((m_release_index) * sizeof(EventDescriptor)) %
                           m_reportBuffer->getPhysicalSize();
    release_index_count++;
    m_release_index =
        (m_release_index < m_max_rb_entries - 1) ? (m_release_index + 1) : 0;

    // ring buffer wrap-around: clear up to the end and start over from 0
    if (m_release_index == 0) {
      memset(&m_reports[release_index_start], 0,
             release_index_count * sizeof(EventDescriptor));
      release_index_count = 0;
      release_index_start = 0;
    }
  }

  memset(&m_reports[release_index_start], 0,
         release_index_count * sizeof(EventDescriptor));
  m_channel->setBufferOffsetsOnDevice(event_buffer_offset,
                                      report_buffer_offset);
  m_channel_status->release_offset = event_buffer_offset;
}

int event_stream::releaseEvent(uint64_t reference) {
  if (reference >= m_max_rb_entries) {
    return -1;
  }
  pthread_mutex_lock(&m_releaseEnable);
  m_release_map[reference] = true;
  updateBufferOffsets();
  pthread_mutex_unlock(&m_releaseEnable);
  return 0;
}

const uint32_t *event_stream::getRawEvent(EventDescriptor report) {
  return (const uint32_t *)&m_raw_event_buffer[report.offset / 4];
}

/************************* Generators *************************/

patterngenerator *event_stream::getPatternGenerator() {
  if (m_link->patternGeneratorAvailable()) {
    return new patterngenerator(m_link);
  } else {
    return NULL;
  }
}

diu *event_stream::getDiu() {
  if (m_linktype == RORC_CFG_LINK_TYPE_DIU) {
    return new diu(m_link);
  } else {
    return NULL;
  }
}

siu *event_stream::getSiu() {
  if (m_linktype == RORC_CFG_LINK_TYPE_SIU) {
    return new siu(m_link);
  } else {
    return NULL;
  }
}

fastclusterfinder *event_stream::getFastClusterFinder() {
  if (m_link->fastClusterFinderAvailable()) {
    return new fastclusterfinder(m_link);
  } else {
    return NULL;
  }
}

ddl *event_stream::getRawReadout() {
  if (m_linktype == RORC_CFG_LINK_TYPE_VIRTUAL) {
    return new ddl(m_link);
  } else {
    return NULL;
  }
}
}
