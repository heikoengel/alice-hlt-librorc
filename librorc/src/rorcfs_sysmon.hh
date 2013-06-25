/**
 * @file rorcfs_sysmon.hh
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

#ifndef RORCFS_SYSMON_H
#define RORCFS_SYSMON_H

#define LIBRORC_SYSMON_ERROR_CONSTRUCTOR_FAILED  1
#define LIBRORC_SYSMON_ERROR_PCI_PROBLEM         5
#define LIBRORC_SYSMON_ERROR_RXACK              20
#define LIBRORC_SYSMON_ERROR_I2C_RESET_FAILED   30
#define LIBRORC_SYSMON_ERROR_I2C_READ_FAILED    40

#include <librorc_registers.h>

/**
 * @class rorcfs_sysmon
 * @brief System monitor class
 *
 * This class can be attached to rorcfs_bar to provide access to the
 * static parts of the design, like PCIe status, SystemMonitor readings
 * and access to the ICAP interface
 **/
class rorcfs_sysmon
{
    public:
        rorcfs_sysmon(librorc_bar *parent_bar);
        ~rorcfs_sysmon();

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
         * get FPGA unique identifier (Device DNA)
         * @return 64bit Device DNA
        **/
        uint64_t DeviceDNA();

        /**
         * get Fan Tach Value
         * @return RPMs of the FPGA Fan
        **/
        uint32_t FanTachValue();

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
        void i2c_reset();

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
            uint8_t slvaddr,
            uint8_t memaddr
        );

        /**
         * read byte from i2c memory location
         * @param slvaddr slave address
         * @param memaddr memory address
         * @param data pointer to unsigned char for
         * received data
         * @return 0 on success, -1 on errors
        **/
        void
        i2c_write_mem
        (
            uint8_t slvaddr,
            uint8_t memaddr,
            uint8_t data
        );

        /**
         * i2c_set_config
         * @param config i2c configuration consisting of
         * prescaler (31:16) and ctrl (7:0)
        **/
        void i2c_set_config(uint32_t config);

    protected:

        uint32_t
        wait_for_tip_to_negate();

        void
        check_rxack_is_zero
        (
            uint32_t status
        );

        librorc_bar *m_bar;
};

#endif
