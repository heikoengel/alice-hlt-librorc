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
#ifndef LIBRORC_DDR3_H
#define LIBRORC_DDR3_H

#include <librorc/defines.hh>

namespace LIBRARY_NAME {

#define LIBRORC_DDR3_RESET 0
#define LIBRORC_DDR3_PHYINITDONE 1
#define LIBRORC_DDR3_PLLLOCK 2
#define LIBRORC_DDR3_RDLVL_START 3
#define LIBRORC_DDR3_RDLVL_DONE 4
#define LIBRORC_DDR3_RDLVL_ERROR 5
#define LIBRORC_DDR3_WRLVL_START 6
#define LIBRORC_DDR3_WRLVL_DONE 7
#define LIBRORC_DDR3_WRLVL_ERROR 8
#define LIBRORC_DDR3_TG_ERROR 15

    class bar;
    class ddr3 {
        public:
            ddr3(bar *bar, uint32_t moduleId);
            ~ddr3();

            void setReset(uint32_t value);
            uint32_t getReset();
            uint32_t getBitrate();
            bool isImplemented();
            uint32_t maxModuleSize();
            bool initSuccessful();
            bool initPhaseDone();
            uint16_t controllerState();
            uint32_t getTgErrorFlag();
            uint32_t id() { return m_id; }
            uint32_t getTgRdCount();
            uint32_t getTgWrCount();

        protected:
            bar *m_bar;
            uint32_t m_id;
            uint32_t m_offset;
    };
}
#endif
