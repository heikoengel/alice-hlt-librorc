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

#include <iostream>
#include <cstring>
#include <librorc/defines.hh>

#ifndef LIBRORC_SYSMON_H
#define LIBRORC_SYSMON_H

#define LIBRORC_MAX_QSFP        3
#define LIBRORC_SYSMON_QSFP_NO_RATE_SELECTION  0
#define LIBRORC_SYSMON_QSFP_EXT_RATE_SELECTION 1
#define LIBRORC_SYSMON_QSFP_APT_RATE_SELECTION 2

namespace LIBRARY_NAME
{
    class bar;

    /**
     * @brief System monitor class
     *
     * This class can be attached to bar to provide access to the
     * static parts of the design, like PCIe status and SystemMonitor 
     * readings
     **/
    class sysmon
    {
        public:
             sysmon(bar *parent_bar);
            ~sysmon();

            /**
             * get PCIe Interface status
             * @return PCIe status consisting of:
             * pcie_status_q[31:26] <= pl_ltssm_state;
             * pcie_status_q[25:23] <= pl_initial_link_width;
             * pcie_status_q[22:20] <= pl_lane_reversal_mode;
             * pcie_status_q[19] <= pl_link_gen2_capable;
             * pcie_status_q[18] <= pl_link_partner_gen2_supported;
             * pcie_status_q[17] <= pl_link_upcfg_capable;
             * pcie_status_q[16:9] <= 7'b0;
             * pcie_status_q[8] <= pl_sel_link_rate;
             * pcie_status_q[7:2] <= 6'b0;
             * pcie_status_q[1:0] <= pl_sel_link_width;
            **/
            uint32_t PCIeStatus();

            /**
             * get FPGA Firmware Revision
             * @return Firmware Revision
            **/
            uint32_t FwRevision();

            /**
             * get FPGA Firmware Build Date
             * @return Firmware Build Date as combination of
             * year (bits [31:16]), month (bits[15:8]) and
             * day (bits[7:0]).
            **/
            uint32_t FwBuildDate();

            /**
             * get number of implemented DMA channels
             **/
            uint32_t numberOfChannels();

            uint16_t    firmwareType();
            bool        firmwareIsHltIn();
            bool        firmwareIsHltOut();
            bool        firmwareIsHltPciDebug();
            bool        firmwareIsHltInFcf();
            bool        firmwareIsHltHardwareTest();
            const char *firmwareDescription();

            uint32_t pcieNumberOfLanes();
            uint32_t pcieGeneration();

            /**
             * get FPGA Temperature
             * @return FPGA temperature in degree celsius
            **/
            double FPGATemperature();

            /**
             * get FPGA VCC_INT Voltage
             * @return VCCINT in Volts
            **/
            double VCCINT();

            /**
             * get FPGA VCC_AUX Voltage
             * @return VCCAUX in Volts
            **/
            double VCCAUX();

            bool   systemClockIsRunning();
            bool   systemFanIsEnabled();
            bool   systemFanIsRunning();
            double systemFanSpeed();

            /**
             * control FPGA fan. The fan is by default in automatic control
             * mode: as long as the FPGA temperature is below ~72 degC the
             * fan is disabled. If the temperature exceeds this threshold
             * the fan is activated. This default mode can be overridden by
             * manual controls.
             *
             * @param ovrd override fan control. 1 enables
             * override mode, 0 activates automatic fan control.
             * @param enable enable (1) or disable (0) the fan. This only
             * applies if override is non-zero
             **/
            void
            systemFanSetEnable
            (
                uint32_t ovrd,
                uint32_t enable
            );

            /**
             * get fan operating mode
             * @return true if automatic fan control is enabled, false if
             * manual fan override is active
             **/
            bool
            systemFanIsAutoMode();

            /**
             * get bracket LED mode
             * @return true if in blink mode, false if in auto mode
             **/
            bool
            bracketLedInBlinkMode();

            /**
             * set bracket LED mode
             * @param mode 0 for auto mode, 1 for blink mode
             **/
            void
            setBracketLedMode( uint32_t mode );


            /**
             * get current link mask
             * @return bitmask of links, 1 is active, 0 is masked
             **/
            uint32_t
            getLinkmask();

            /**
             * set link mask
             * @param mask bitmask of links, 1 is active, 0 is masked
             **/
            void
            setLinkmask( uint32_t mask );

            /**
             * get board uptime in seconds. This counter is only reset
             * when (re)configuring the FPGA and cannot be cleared with
             * software.
             * @return uptime in seconds
             **/
            uint64_t uptimeSeconds();

            /**
             * get the current dip switch setting
             * @return dipswitch value
             **/
            uint32_t dipswitch();


            bool
            qsfpIsPresent
            (
                uint8_t index
            );

            bool
            qsfpLEDIsOn
            (
                uint8_t qsfp_index,
                uint8_t LED_index
            );

            bool
            qsfpGetReset
            (
                uint8_t index
            );

            void
            qsfpSetReset
            (
                uint8_t index,
                uint8_t reset
            );

            std::string
            qsfpVendorName
            (
                uint8_t index
            );

            std::string
            qsfpPartNumber
            (
                uint8_t index
            );

            std::string
            qsfpRevisionNumber
            (
                uint8_t index
            );

            std::string
            qsfpSerialNumber
            (
                uint8_t index
            );

            float
            qsfpTemperature
            (
                uint8_t index
            );

            float
            qsfpVoltage
            (
                uint8_t index
            );

            float
            qsfpRxPower
            (
                uint8_t index,
                uint8_t channel
            );

            float
            qsfpTxBias
            (
                uint8_t index,
                uint8_t channel
            );

            float
            qsfpWavelength
            (
                uint8_t index
            );

            uint8_t
            qsfpTxFaultMap
            (
                 uint8_t index
            );

            uint8_t
            qsfpGetRateSelectionSupport
            (
                 uint8_t index
            );

            uint8_t
            qsfpPowerClass
            (
                 uint8_t index
            );

            bool
            qsfpHasRXCDR
            (
                 uint8_t index
            );

            bool
            qsfpHasTXCDR
            (
                 uint8_t index
            );


            /**
             * reset i2c bus
             * @param chain chain to be resetted
            **/
            void i2c_reset
            (
                uint8_t chain
            );

            /**
             * Read byte from i2c memory location.
             * Throws exception on error
             * @param chain i2c chain number
             * @param slvaddr slave address
             * @param memaddr memory address
             * @return data
            **/
            uint8_t
            i2c_read_mem
            (
                uint8_t chain,
                uint8_t slvaddr,
                uint8_t memaddr
            );

            /**
             * Write byte to i2c memory location.
             * Throws exception on error
             * @param chain i2c chain number
             * @param slvaddr slave address
             * @param memaddr memory address
             * @param data to be written
            **/
            void
            i2c_write_mem
            (
                uint8_t chain,
                uint8_t slvaddr,
                uint8_t memaddr,
                uint8_t data
            );

            /**
             * perform a write of two bytes to independent i2c
             * memory locations. Throws exception on error.
             * @param chain i2c chain number
             * @param slvaddr slave address
             * @param memaddr0 first memory address
             * @param data0 first byte to be written
             * @param memaddr1 second memory address
             * @param data1 second byte to be written
            **/
            void
            i2c_write_mem_dual
            (
                uint8_t chain,
                uint8_t slvaddr,
                uint8_t memaddr0,
                uint8_t data0,
                uint8_t memaddr1,
                uint8_t data1
            );

            /**
             * set I2C speed mode:
             * 0: 100 KHz operation
             * 1: 400 kHz operation
             * */
            void
            i2c_set_mode
            (
                uint8_t mode
            );

            /**
             * get I2C speed mode
             * @return 0 for 100 kHz, 1 for 400 kHZ operation
             * */
            uint8_t
            i2c_get_mode();


            void
            ddr3SetReset
            (
                uint32_t controller,
                uint32_t value
            );

            uint32_t
            ddr3GetReset
            (
                uint32_t controller
            );

            /**
             * write event to DDR3 memory
             * @param event_data pointer to data
             * @param num_dws number of DWs to be written
             * @param ddr3_start_addr DDR3 memory start address
             * @param channel channel number, allowed range: 0-7
             * @param last_event mark event as last event in DDR3
             *        memory for current channel
             * @return returns ddr3_address for next event
             * */
            uint32_t
            ddr3DataReplayEventToRam
            (
                uint32_t *event_data,
                uint32_t num_dws,
                uint32_t ddr3_start_addr,
                uint8_t channel,
                bool last_event
            );

            /**
             * check if DDR3 controller and module are ready to be used
             * @param controller 0 for SO-DIMM 0, 1 for SO-DIMM 1
             * @return true if ready, false if not ready or not implemented
             **/
            bool
            ddr3ModuleInitReady
            (
                uint32_t controller
            );

            /**
             * get Bitrate of DDR3 controller
             * @param controller 0 for SO-DIMM 0, 1 for SO-DIMM 1
             * @return 0 if not implemented in firmware else bitrate, e.g.
             * 1066, 800, 606, ...
             **/
            uint32_t
            ddr3Bitrate
            (
                uint32_t controller
            );

            /**
             * get maximum module size supported by firmware controller
             * @param controller 0 for SO-DIMM 0, 1 for SO-DIMM 1
             * @return modules size in bytes
             **/
            uint32_t
            ddr3ControllerMaxModuleSize
            (
                 uint32_t controller
            );


            /**
             * read from DDR3 SPD monitor
             * @param module 0 or 1 to select target SO-DIMM module
             * @param address target address to be read
             * @return 8 bit value read from SPD
             **/
            uint8_t
            ddr3SpdRead
            (
                uint8_t module,
                uint8_t address
            );

            /**
             * read string from DDR3 SPD monitor. This iteratively calls
             * ddr3SpdRead and returns the results as a string
             * @param module 0 or 1 to select target SO-DIMM module
             * @param start_address first address to read
             * @param end_address last address to read
             * @return string of the values from start_address to
             * end_address
             **/
            std::string
            ddr3SpdReadString
            (
                uint8_t module,
                uint8_t start_address,
                uint8_t end_address
            );

            /**
             * get PCIe/SlowControl error counters
             **/
            uint32_t pcieRequestCanceledCounter();
            uint32_t pcieTxTimeoutCounter();
            uint32_t pcieIllegalRequestCounter();
            uint32_t pcieMultiDwReadCounter();
            uint32_t pcieTxDestinationBusyCounter();
            uint32_t pcieTransmissionErrorCounter();

            /**
             * clear individual PCIe/SlowControl error counters
             **/
            void clearPcieRequestCanceledCounter();
            void clearPcieTxTimeoutCounter();
            void clearPcieIllegalRequestCounter();
            void clearPcieMultiDwReadCounter();
            void clearPcieTxDestinationBusyCounter();
            void clearPcieTransmissionErrorCounter();

            /**
             * clear all PCIe/SlowControl error counters
             **/
            void clearAllErrorCounters();

        protected:


            /**
             * write data block into DDR3 memory
             * @param start_addr start address of current block in DDR3
             * @param data pointer to data
             * @param mask bitmask of active DWs in dataset:
             *        0x0000: no DW active
             *        0x0001: lowest DW active
             *        0x7fff: all 15 DWs active
             * @param channel channel number, allowed range: 0-7
             * @param flags flags for current block. Allowed flags are
             *        DATA_REPLAY_END: last block for current channel
             *        DATA_REPLAY_EOE: add EOE-Word after current dataset
             * */
            void
            ddr3DataReplayBlockToRam
            (
                 uint32_t start_addr,
                 uint32_t *data,
                 uint16_t mask,
                 uint8_t channel,
                 uint32_t flags
            );

            void
            i2c_module_start
            (
                uint8_t chain,
                uint8_t slvaddr,
                uint8_t cmd,
                uint8_t mode,
                uint8_t bytes_enable
            );

            uint32_t i2c_wait_for_cmpl();

            /**
             * read string from QSFP i2c memory map
             * @param index target QSFP
             * @param start start address
             * @param end end address
             * @return string
             **/
            std::string
            qsfp_i2c_string_readout
            (
                uint8_t index,
                uint8_t start,
                uint8_t end
            );


            /**
             * select page 0 address space
             * @param index target QSFP
             **/
            void
            qsfp_select_page0
            (
                uint8_t index
            );

            /** base bar instance */
            bar *m_bar;

            /** high speed mode flag */
            uint8_t m_i2c_hsmode;
    };

}

#endif /** LIBRORC_SYSMON_H */
