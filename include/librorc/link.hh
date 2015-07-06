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

#ifndef LIBRORC_LINK_H
#define LIBRORC_LINK_H

#include <librorc/defines.hh>
#include <librorc/bar.hh>


namespace LIBRARY_NAME
{
    class device;

    /**
     * @brief link class, TODO: rename & cleanup!
     **/
    class link
    {
        public:
            link
            (
                bar      *bar,
                uint32_t  link_number
            );

            ~link(){};


            /**
             * get BAR
             * @return bound librorc::bar
             **/
            bar *getBar()
            {
                return m_bar;
            }

            uint32_t base()
            {
                return m_base;
            }

            uint32_t linkNumber()
            { return m_link_number; }


            /**********************************************************
             *             Low Level Channel Access
             * *******************************************************/

            /**
             * set DW in GTX Domain
             * @param addr address in GTX component
             * @param data data to be written
             **/
            void
            setGtxReg
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from GTX Domain
             * @param addr address in GTX component
             * @return data read from GTX domain
             **/
            uint32_t gtxReg(uint32_t addr);

            /**
             * set DW in DDL Domain
             * @param addr address in DDL component
             * @param data data to be written
             **/
            void
            setDdlReg
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from DDL Domain
             * @param addr address in DDL component
             * @return data read from DDL domain
             **/
            uint32_t ddlReg(uint32_t addr);

            /**
             * set DW in Packtizer
             * @param addr address in PKT component
             * @param data data to be written
             **/
            void
            setPciReg
            (
                uint32_t addr,
                uint32_t data
            );

            /**
             * get DW from Packtizer
             * @param addr address in PKT component
             * @return data read from PKT
             **/
            uint32_t pciReg(uint32_t addr);

            /**
             * memcopy data to link registers. This is a wrapper around
             * bar::memcopy using m_base offset on the target register.
             * @param target target register in link memory range. Don't add the
             * link offset manually!
             * @param source source data
             * @param num number of bytes to be written
             **/
            void
            memcopy
            (
                 bar_address  target,
                 const void  *source,
                 size_t       num
            );

            /**
             * get GTX clock domain status
             * @return TRUE if up and running, FALSE if down
             * */
            bool isGtxDomainReady();

            /**
             * get DDL clock domain status
             * @return TRUE if up and running, FALSE if down
             * */
            bool isDdlDomainReady();

            /**
             * get GTX Link Status
             * @return TRUE if up and running, FALSE if down
             * */
            bool isGtxLinkUp();

            /**
             * wait for GTX domain to be ready read asynchronous GTX status
             * wait for rxresetdone & txresetdone & rxplllkdet & txplllkdet
             * & !gtx_in_rst
             * @return 0 on success, -1 on error/timeout
            **/
            int waitForGTXDomain();

            /**
             * get link type
             * @return link type. Possible values are
             * RORC_CFG_LINK_TYPE_XYZ, where XYZ can be
             * DIU, SIU, VIRTUAL or LINKTEST
             **/
            uint32_t linkType();


            /**********************************************************
             *             Data Path Configuration
             *********************************************************/

            /**
             * set default datasource
             **/
            void setDefaultDataSource();

            /**
             * set DDR3 Data Replay as data source
             **/
            void setDataSourceDdr3DataReplay();

            /**
             * set Pattern Generator as data source
             **/
            void setDataSourcePatternGenerator();

            /**
             * set DIU as data source
             **/
            void setDataSourceDiu();

            /**
             * use PCIe as data source (SIU only)
             **/
            void setDataSourcePcie();

            /**
             * get description string of current data source selection
             * @return string of "DDL", "DDR", "PG", "RAW", "PCI" on success
             * or "UNKNOWN" on error
             **/
            const char* getDataSourceDescr();


            /**
             * Enable/disable Flow Control
             * With flow control enabled the datapath respects full FIFOs
             * and throttles the data flow from source. No XOFF is sent
             * to DAQ as long as this is not enabled.
             * With flow control enabled the datapath from source
             * to DMA engine ignores any full FIFOs and will never
             * send XOFF to DAQ. This also means that DMA'ed events
             * may be incomplete.
             * @param enable 1 to enable, 0 to disable
             **/
            void
            setFlowControlEnable
            (
                uint32_t enable
            );


            /**
             * check if Flow Control is enabled
             * @return true if enabled, false if disabled
             **/
            bool
            flowControlIsEnabled();


            /**
             * set channel-active flag. Only an active channel can
             * push data into the DMA engine. A passive channel is
             * discarding all incoming data.
             * @param active 0 or 1
             **/
            void
            setChannelActive
            (
                uint32_t active
            );


            /**
             * check if channel is active
             * @return true if active, false if passive
             **/
            bool
            channelIsActive();


            /**
             * check if current firmware has a pattern genertor
             * implemented
             * @return true if available, else false
             **/
            bool
            patternGeneratorAvailable();


            /**
             * check if current firmware has DDR3 Data Replay
             * implemented for current link.
             * @return true if available, else false
             **/
            bool
            ddr3DataReplayAvailable();

            /**
             * check if current channel has a FastClusterFinder
             * instance in the data flow
             * @return true if available, else false
             **/
            bool
            fastClusterFinderAvailable();


        protected:
            bar      *m_bar;
            uint32_t  m_base;
            uint32_t  m_link_number;

            void setDataSourceMux( uint32_t value );
            uint32_t getDataSourceMux();
};

}

#endif /** LIBRORC_LINK_H */
