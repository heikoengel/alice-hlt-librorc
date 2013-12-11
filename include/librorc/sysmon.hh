/**
 * @file
 * @author Heiko Engel <hengel@cern.ch>
 * @version 0.1
 * @date 2011-11-16
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * */

#include "include_ext.hh"
#include "defines.hh"

#ifndef LIBRORC_SYSMON_H
#define LIBRORC_SYSMON_H

#define LIBRORC_SYSMON_ERROR_CONSTRUCTOR_FAILED  1
#define LIBRORC_SYSMON_ERROR_PCI_PROBLEM         5
#define LIBRORC_SYSMON_ERROR_RXACK              20
#define LIBRORC_SYSMON_ERROR_I2C_RESET_FAILED   30
#define LIBRORC_SYSMON_ERROR_I2C_READ_FAILED    40
#define LIBRORC_SYSMON_ERROR_I2C_INVALID_PARAM  50

#define QSFP_I2C_SLVADDR        0x50
#define LIBRORC_MAX_QSFP        3
#define I2C_READ                (1<<1)
#define I2C_WRITE               (1<<2)

#define LIBRORC_SYSMON_QSFP_NO_RATE_SELECTION  0
#define LIBRORC_SYSMON_QSFP_EXT_RATE_SELECTION 1
#define LIBRORC_SYSMON_QSFP_APT_RATE_SELECTION 2


#define LIBRORC_NUMBER_OF_FIRMWARE_MODES 5
const char
librorc_firmware_mode_descriptions[LIBRORC_NUMBER_OF_FIRMWARE_MODES][1024]
= {
"HLT-in",
"HLT-out",
"PCI Debug",
"HLT-in (fcf)",
"hwtest"
};

//const char librorc_firmware_mode_descriptions[] = "in";


/**
 * @class librorc_sysmon
 * @brief System monitor class
 *
 * This class can be attached to librorc::rorc_bar to provide access to the
 * static parts of the design, like PCIe status, SystemMonitor readings
 * and access to the ICAP interface
 **/
namespace LIBRARY_NAME
{
class bar;

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

            uint16_t    firmwareType();
            bool        FwIsHltIn();
            bool        FwIsHltOut();
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

            void
            systemFanSetEnable
            (
                uint32_t enable
            );

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

            string*
            qsfpVendorName
            (
                uint8_t index
            );

            string*
            qsfpPartNumber
            (
                uint8_t index
            );

            string*
            qsfpRevisionNumber
            (
                uint8_t index
            );

            string*
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
             * write to ICAP Interface
             * @param dword bit-reordered configuration word
             *
             * use this function to write already reordered
             * (*.bin-file) contents to the ICAP interface.
            **/
            //void setIcapDin( uint32_t dword );

            /**
             * write to ICAP Interface and do the bit reordering
             * @param dword not reordered configuration word
             *
             * use this function to write non-reordered
             * (*.bit-files) to ICAP and do the reordering
             * in the FPGA.
            **/
            //void setIcapDinReorder( uint32_t dword );


            /**
             * reset i2c bus
            **/
            void i2c_reset
            (
                uint8_t chain
            );

            /**
             * read byte from i2c memory location
             * @param slvaddr slave address
             * @param memaddr memory address
             * @param data pointer to unsigned char for
             * received data
             * @return 0 on success, -1 on errors
            **/
            uint8_t
            i2c_read_mem
            (
                uint8_t chain,
                uint8_t slvaddr,
                uint8_t memaddr
            );

            /**
             * write byte to i2c memory location
             * @param slvaddr slave address
             * @param memaddr memory address
             * @param data to be written
             * throws exception on error
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
             * write byte to i2c memory location
             * @param slvaddr slave address
             * @param memaddr memory address
             * @param data to be written
             * throws exception on error
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


        protected:

            /**
             * i2c_set_config
             * @param config i2c configuration consisting of
             * prescaler (31:16) and ctrl (7:0)
            **/
            void i2c_set_config(uint32_t config);

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

            string*
            qsfp_i2c_string_readout
            (
                uint8_t index,
                uint8_t start,
                uint8_t end
            );


            void
            qsfp_select_page0
            (
                uint8_t index
            );

            bar *m_bar;
            uint8_t m_i2c_hsmode;
    };

}

#endif /** LIBRORC_SYSMON_H */
