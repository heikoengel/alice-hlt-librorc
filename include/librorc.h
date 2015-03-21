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

#ifndef LIBRORC_H
#define LIBRORC_H

#include "librorc/registers.h"
#include "librorc/defines.hh"
#include "librorc/device.hh"
#include "librorc/sysfs_handler.hh"
#include "librorc/bar.hh"
#include "librorc/buffer.hh"
#include "librorc/flash.hh"
#include "librorc/sysmon.hh"
#include "librorc/refclk.hh"
#include "librorc/microcontroller.hh"
#include "librorc/dma_channel.hh"
#include "librorc/event_stream.hh"
#include "librorc/high_level_event_stream.hh"
#include "librorc/event_sanity_checker.hh"
#include "librorc/patterngenerator.hh"
#include "librorc/fastclusterfinder.hh"
#include "librorc/datareplaychannel.hh"
#include "librorc/ddl.hh"
#include "librorc/diu.hh"
#include "librorc/siu.hh"
#include "librorc/gtx.hh"
#include "librorc/eventfilter.hh"
#include "librorc/ddr3.hh"
#include "librorc/error.hh"

#endif /** LIBRORC_H */
