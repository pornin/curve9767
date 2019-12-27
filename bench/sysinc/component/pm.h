/**
 * \file
 *
 * \brief Component description for PM
 *
 * Copyright (c) 2016 Atmel Corporation,
 *                    a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAMD20_PM_COMPONENT_
#define _SAMD20_PM_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR PM */
/* ========================================================================== */
/** \addtogroup SAMD20_PM Power Manager */
/*@{*/

#define PM_U2206
#define REV_PM                      0x202

#define PM_CTRL_OFFSET              0x00         /**< \brief (PM_CTRL offset) Control */
#define PM_CTRL_RESETVALUE          _U(0x00);    /**< \brief (PM_CTRL reset_value) Control */

#define PM_CTRL_MASK                _U(0x00)     /**< \brief (PM_CTRL) MASK Register */

#define PM_SLEEP_OFFSET             0x01         /**< \brief (PM_SLEEP offset) Sleep Mode */
#define PM_SLEEP_RESETVALUE         _U(0x00);    /**< \brief (PM_SLEEP reset_value) Sleep Mode */

#define PM_SLEEP_IDLE_Pos           0            /**< \brief (PM_SLEEP) Idle Mode Configuration */
#define PM_SLEEP_IDLE_Msk           (_U(0x3) << PM_SLEEP_IDLE_Pos)
#define PM_SLEEP_IDLE(value)        (PM_SLEEP_IDLE_Msk & ((value) << PM_SLEEP_IDLE_Pos))
#define   PM_SLEEP_IDLE_CPU_Val           _U(0x0)   /**< \brief (PM_SLEEP) The CPU clock domain is stopped */
#define   PM_SLEEP_IDLE_AHB_Val           _U(0x1)   /**< \brief (PM_SLEEP) The CPU and AHB clock domains are stopped */
#define   PM_SLEEP_IDLE_APB_Val           _U(0x2)   /**< \brief (PM_SLEEP) The CPU, AHB and APB clock domains are stopped */
#define PM_SLEEP_IDLE_CPU           (PM_SLEEP_IDLE_CPU_Val         << PM_SLEEP_IDLE_Pos)
#define PM_SLEEP_IDLE_AHB           (PM_SLEEP_IDLE_AHB_Val         << PM_SLEEP_IDLE_Pos)
#define PM_SLEEP_IDLE_APB           (PM_SLEEP_IDLE_APB_Val         << PM_SLEEP_IDLE_Pos)
#define PM_SLEEP_MASK               _U(0x03)     /**< \brief (PM_SLEEP) MASK Register */

#define PM_CPUSEL_OFFSET            0x08         /**< \brief (PM_CPUSEL offset) CPU Clock Select */
#define PM_CPUSEL_RESETVALUE        _U(0x00);    /**< \brief (PM_CPUSEL reset_value) CPU Clock Select */

#define PM_CPUSEL_CPUDIV_Pos        0            /**< \brief (PM_CPUSEL) CPU Prescaler Selection */
#define PM_CPUSEL_CPUDIV_Msk        (_U(0x7) << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV(value)     (PM_CPUSEL_CPUDIV_Msk & ((value) << PM_CPUSEL_CPUDIV_Pos))
#define   PM_CPUSEL_CPUDIV_DIV1_Val       _U(0x0)   /**< \brief (PM_CPUSEL) Divide by 1 */
#define   PM_CPUSEL_CPUDIV_DIV2_Val       _U(0x1)   /**< \brief (PM_CPUSEL) Divide by 2 */
#define   PM_CPUSEL_CPUDIV_DIV4_Val       _U(0x2)   /**< \brief (PM_CPUSEL) Divide by 4 */
#define   PM_CPUSEL_CPUDIV_DIV8_Val       _U(0x3)   /**< \brief (PM_CPUSEL) Divide by 8 */
#define   PM_CPUSEL_CPUDIV_DIV16_Val      _U(0x4)   /**< \brief (PM_CPUSEL) Divide by 16 */
#define   PM_CPUSEL_CPUDIV_DIV32_Val      _U(0x5)   /**< \brief (PM_CPUSEL) Divide by 32 */
#define   PM_CPUSEL_CPUDIV_DIV64_Val      _U(0x6)   /**< \brief (PM_CPUSEL) Divide by 64 */
#define   PM_CPUSEL_CPUDIV_DIV128_Val     _U(0x7)   /**< \brief (PM_CPUSEL) Divide by 128 */
#define PM_CPUSEL_CPUDIV_DIV1       (PM_CPUSEL_CPUDIV_DIV1_Val     << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV2       (PM_CPUSEL_CPUDIV_DIV2_Val     << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV4       (PM_CPUSEL_CPUDIV_DIV4_Val     << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV8       (PM_CPUSEL_CPUDIV_DIV8_Val     << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV16      (PM_CPUSEL_CPUDIV_DIV16_Val    << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV32      (PM_CPUSEL_CPUDIV_DIV32_Val    << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV64      (PM_CPUSEL_CPUDIV_DIV64_Val    << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_CPUDIV_DIV128     (PM_CPUSEL_CPUDIV_DIV128_Val   << PM_CPUSEL_CPUDIV_Pos)
#define PM_CPUSEL_MASK              _U(0x07)     /**< \brief (PM_CPUSEL) MASK Register */

#define PM_APBASEL_OFFSET           0x09         /**< \brief (PM_APBASEL offset) APBA Clock Select */
#define PM_APBASEL_RESETVALUE       _U(0x00);    /**< \brief (PM_APBASEL reset_value) APBA Clock Select */

#define PM_APBASEL_APBADIV_Pos      0            /**< \brief (PM_APBASEL) APBA Prescaler Selection */
#define PM_APBASEL_APBADIV_Msk      (_U(0x7) << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV(value)   (PM_APBASEL_APBADIV_Msk & ((value) << PM_APBASEL_APBADIV_Pos))
#define   PM_APBASEL_APBADIV_DIV1_Val     _U(0x0)   /**< \brief (PM_APBASEL) Divide by 1 */
#define   PM_APBASEL_APBADIV_DIV2_Val     _U(0x1)   /**< \brief (PM_APBASEL) Divide by 2 */
#define   PM_APBASEL_APBADIV_DIV4_Val     _U(0x2)   /**< \brief (PM_APBASEL) Divide by 4 */
#define   PM_APBASEL_APBADIV_DIV8_Val     _U(0x3)   /**< \brief (PM_APBASEL) Divide by 8 */
#define   PM_APBASEL_APBADIV_DIV16_Val    _U(0x4)   /**< \brief (PM_APBASEL) Divide by 16 */
#define   PM_APBASEL_APBADIV_DIV32_Val    _U(0x5)   /**< \brief (PM_APBASEL) Divide by 32 */
#define   PM_APBASEL_APBADIV_DIV64_Val    _U(0x6)   /**< \brief (PM_APBASEL) Divide by 64 */
#define   PM_APBASEL_APBADIV_DIV128_Val   _U(0x7)   /**< \brief (PM_APBASEL) Divide by 128 */
#define PM_APBASEL_APBADIV_DIV1     (PM_APBASEL_APBADIV_DIV1_Val   << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV2     (PM_APBASEL_APBADIV_DIV2_Val   << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV4     (PM_APBASEL_APBADIV_DIV4_Val   << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV8     (PM_APBASEL_APBADIV_DIV8_Val   << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV16    (PM_APBASEL_APBADIV_DIV16_Val  << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV32    (PM_APBASEL_APBADIV_DIV32_Val  << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV64    (PM_APBASEL_APBADIV_DIV64_Val  << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_APBADIV_DIV128   (PM_APBASEL_APBADIV_DIV128_Val << PM_APBASEL_APBADIV_Pos)
#define PM_APBASEL_MASK             _U(0x07)     /**< \brief (PM_APBASEL) MASK Register */

#define PM_APBBSEL_OFFSET           0x0A         /**< \brief (PM_APBBSEL offset) APBB Clock Select */
#define PM_APBBSEL_RESETVALUE       _U(0x00);    /**< \brief (PM_APBBSEL reset_value) APBB Clock Select */

#define PM_APBBSEL_APBBDIV_Pos      0            /**< \brief (PM_APBBSEL) APBB Prescaler Selection */
#define PM_APBBSEL_APBBDIV_Msk      (_U(0x7) << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV(value)   (PM_APBBSEL_APBBDIV_Msk & ((value) << PM_APBBSEL_APBBDIV_Pos))
#define   PM_APBBSEL_APBBDIV_DIV1_Val     _U(0x0)   /**< \brief (PM_APBBSEL) Divide by 1 */
#define   PM_APBBSEL_APBBDIV_DIV2_Val     _U(0x1)   /**< \brief (PM_APBBSEL) Divide by 2 */
#define   PM_APBBSEL_APBBDIV_DIV4_Val     _U(0x2)   /**< \brief (PM_APBBSEL) Divide by 4 */
#define   PM_APBBSEL_APBBDIV_DIV8_Val     _U(0x3)   /**< \brief (PM_APBBSEL) Divide by 8 */
#define   PM_APBBSEL_APBBDIV_DIV16_Val    _U(0x4)   /**< \brief (PM_APBBSEL) Divide by 16 */
#define   PM_APBBSEL_APBBDIV_DIV32_Val    _U(0x5)   /**< \brief (PM_APBBSEL) Divide by 32 */
#define   PM_APBBSEL_APBBDIV_DIV64_Val    _U(0x6)   /**< \brief (PM_APBBSEL) Divide by 64 */
#define   PM_APBBSEL_APBBDIV_DIV128_Val   _U(0x7)   /**< \brief (PM_APBBSEL) Divide by 128 */
#define PM_APBBSEL_APBBDIV_DIV1     (PM_APBBSEL_APBBDIV_DIV1_Val   << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV2     (PM_APBBSEL_APBBDIV_DIV2_Val   << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV4     (PM_APBBSEL_APBBDIV_DIV4_Val   << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV8     (PM_APBBSEL_APBBDIV_DIV8_Val   << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV16    (PM_APBBSEL_APBBDIV_DIV16_Val  << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV32    (PM_APBBSEL_APBBDIV_DIV32_Val  << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV64    (PM_APBBSEL_APBBDIV_DIV64_Val  << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_APBBDIV_DIV128   (PM_APBBSEL_APBBDIV_DIV128_Val << PM_APBBSEL_APBBDIV_Pos)
#define PM_APBBSEL_MASK             _U(0x07)     /**< \brief (PM_APBBSEL) MASK Register */

#define PM_APBCSEL_OFFSET           0x0B         /**< \brief (PM_APBCSEL offset) APBC Clock Select */
#define PM_APBCSEL_RESETVALUE       _U(0x00);    /**< \brief (PM_APBCSEL reset_value) APBC Clock Select */

#define PM_APBCSEL_APBCDIV_Pos      0            /**< \brief (PM_APBCSEL) APBC Prescaler Selection */
#define PM_APBCSEL_APBCDIV_Msk      (_U(0x7) << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV(value)   (PM_APBCSEL_APBCDIV_Msk & ((value) << PM_APBCSEL_APBCDIV_Pos))
#define   PM_APBCSEL_APBCDIV_DIV1_Val     _U(0x0)   /**< \brief (PM_APBCSEL) Divide by 1 */
#define   PM_APBCSEL_APBCDIV_DIV2_Val     _U(0x1)   /**< \brief (PM_APBCSEL) Divide by 2 */
#define   PM_APBCSEL_APBCDIV_DIV4_Val     _U(0x2)   /**< \brief (PM_APBCSEL) Divide by 4 */
#define   PM_APBCSEL_APBCDIV_DIV8_Val     _U(0x3)   /**< \brief (PM_APBCSEL) Divide by 8 */
#define   PM_APBCSEL_APBCDIV_DIV16_Val    _U(0x4)   /**< \brief (PM_APBCSEL) Divide by 16 */
#define   PM_APBCSEL_APBCDIV_DIV32_Val    _U(0x5)   /**< \brief (PM_APBCSEL) Divide by 32 */
#define   PM_APBCSEL_APBCDIV_DIV64_Val    _U(0x6)   /**< \brief (PM_APBCSEL) Divide by 64 */
#define   PM_APBCSEL_APBCDIV_DIV128_Val   _U(0x7)   /**< \brief (PM_APBCSEL) Divide by 128 */
#define PM_APBCSEL_APBCDIV_DIV1     (PM_APBCSEL_APBCDIV_DIV1_Val   << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV2     (PM_APBCSEL_APBCDIV_DIV2_Val   << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV4     (PM_APBCSEL_APBCDIV_DIV4_Val   << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV8     (PM_APBCSEL_APBCDIV_DIV8_Val   << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV16    (PM_APBCSEL_APBCDIV_DIV16_Val  << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV32    (PM_APBCSEL_APBCDIV_DIV32_Val  << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV64    (PM_APBCSEL_APBCDIV_DIV64_Val  << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_APBCDIV_DIV128   (PM_APBCSEL_APBCDIV_DIV128_Val << PM_APBCSEL_APBCDIV_Pos)
#define PM_APBCSEL_MASK             _U(0x07)     /**< \brief (PM_APBCSEL) MASK Register */

#define PM_AHBMASK_OFFSET           0x14         /**< \brief (PM_AHBMASK offset) AHB Mask */
#define PM_AHBMASK_RESETVALUE       _U(0x0000001F); /**< \brief (PM_AHBMASK reset_value) AHB Mask */

#define PM_AHBMASK_HPB0_Pos         0            /**< \brief (PM_AHBMASK) HPB0 AHB Clock Mask */
#define PM_AHBMASK_HPB0             (_U(0x1) << PM_AHBMASK_HPB0_Pos)
#define PM_AHBMASK_HPB1_Pos         1            /**< \brief (PM_AHBMASK) HPB1 AHB Clock Mask */
#define PM_AHBMASK_HPB1             (_U(0x1) << PM_AHBMASK_HPB1_Pos)
#define PM_AHBMASK_HPB2_Pos         2            /**< \brief (PM_AHBMASK) HPB2 AHB Clock Mask */
#define PM_AHBMASK_HPB2             (_U(0x1) << PM_AHBMASK_HPB2_Pos)
#define PM_AHBMASK_DSU_Pos          3            /**< \brief (PM_AHBMASK) DSU AHB Clock Mask */
#define PM_AHBMASK_DSU              (_U(0x1) << PM_AHBMASK_DSU_Pos)
#define PM_AHBMASK_NVMCTRL_Pos      4            /**< \brief (PM_AHBMASK) NVMCTRL AHB Clock Mask */
#define PM_AHBMASK_NVMCTRL          (_U(0x1) << PM_AHBMASK_NVMCTRL_Pos)
#define PM_AHBMASK_MASK             _U(0x0000001F) /**< \brief (PM_AHBMASK) MASK Register */

#define PM_APBAMASK_OFFSET          0x18         /**< \brief (PM_APBAMASK offset) APBA Mask */
#define PM_APBAMASK_RESETVALUE      _U(0x0000007F); /**< \brief (PM_APBAMASK reset_value) APBA Mask */

#define PM_APBAMASK_PAC0_Pos        0            /**< \brief (PM_APBAMASK) PAC0 APB Clock Enable */
#define PM_APBAMASK_PAC0            (_U(0x1) << PM_APBAMASK_PAC0_Pos)
#define PM_APBAMASK_PM_Pos          1            /**< \brief (PM_APBAMASK) PM APB Clock Enable */
#define PM_APBAMASK_PM              (_U(0x1) << PM_APBAMASK_PM_Pos)
#define PM_APBAMASK_SYSCTRL_Pos     2            /**< \brief (PM_APBAMASK) SYSCTRL APB Clock Enable */
#define PM_APBAMASK_SYSCTRL         (_U(0x1) << PM_APBAMASK_SYSCTRL_Pos)
#define PM_APBAMASK_GCLK_Pos        3            /**< \brief (PM_APBAMASK) GCLK APB Clock Enable */
#define PM_APBAMASK_GCLK            (_U(0x1) << PM_APBAMASK_GCLK_Pos)
#define PM_APBAMASK_WDT_Pos         4            /**< \brief (PM_APBAMASK) WDT APB Clock Enable */
#define PM_APBAMASK_WDT             (_U(0x1) << PM_APBAMASK_WDT_Pos)
#define PM_APBAMASK_RTC_Pos         5            /**< \brief (PM_APBAMASK) RTC APB Clock Enable */
#define PM_APBAMASK_RTC             (_U(0x1) << PM_APBAMASK_RTC_Pos)
#define PM_APBAMASK_EIC_Pos         6            /**< \brief (PM_APBAMASK) EIC APB Clock Enable */
#define PM_APBAMASK_EIC             (_U(0x1) << PM_APBAMASK_EIC_Pos)
#define PM_APBAMASK_MASK            _U(0x0000007F) /**< \brief (PM_APBAMASK) MASK Register */

#define PM_APBBMASK_OFFSET          0x1C         /**< \brief (PM_APBBMASK offset) APBB Mask */
#define PM_APBBMASK_RESETVALUE      _U(0x0000001F); /**< \brief (PM_APBBMASK reset_value) APBB Mask */

#define PM_APBBMASK_PAC1_Pos        0            /**< \brief (PM_APBBMASK) PAC1 APB Clock Enable */
#define PM_APBBMASK_PAC1            (_U(0x1) << PM_APBBMASK_PAC1_Pos)
#define PM_APBBMASK_DSU_Pos         1            /**< \brief (PM_APBBMASK) DSU APB Clock Enable */
#define PM_APBBMASK_DSU             (_U(0x1) << PM_APBBMASK_DSU_Pos)
#define PM_APBBMASK_NVMCTRL_Pos     2            /**< \brief (PM_APBBMASK) NVMCTRL APB Clock Enable */
#define PM_APBBMASK_NVMCTRL         (_U(0x1) << PM_APBBMASK_NVMCTRL_Pos)
#define PM_APBBMASK_PORT_Pos        3            /**< \brief (PM_APBBMASK) PORT APB Clock Enable */
#define PM_APBBMASK_PORT            (_U(0x1) << PM_APBBMASK_PORT_Pos)
#define PM_APBBMASK_MASK            _U(0x0000000F) /**< \brief (PM_APBBMASK) MASK Register */

#define PM_APBCMASK_OFFSET          0x20         /**< \brief (PM_APBCMASK offset) APBC Mask */
#define PM_APBCMASK_RESETVALUE      _U(0x00010000); /**< \brief (PM_APBCMASK reset_value) APBC Mask */

#define PM_APBCMASK_PAC2_Pos        0            /**< \brief (PM_APBCMASK) PAC2 APB Clock Enable */
#define PM_APBCMASK_PAC2            (_U(0x1) << PM_APBCMASK_PAC2_Pos)
#define PM_APBCMASK_EVSYS_Pos       1            /**< \brief (PM_APBCMASK) EVSYS APB Clock Enable */
#define PM_APBCMASK_EVSYS           (_U(0x1) << PM_APBCMASK_EVSYS_Pos)
#define PM_APBCMASK_SERCOM0_Pos     2            /**< \brief (PM_APBCMASK) SERCOM0 APB Clock Enable */
#define PM_APBCMASK_SERCOM0         (_U(0x1) << PM_APBCMASK_SERCOM0_Pos)
#define PM_APBCMASK_SERCOM1_Pos     3            /**< \brief (PM_APBCMASK) SERCOM1 APB Clock Enable */
#define PM_APBCMASK_SERCOM1         (_U(0x1) << PM_APBCMASK_SERCOM1_Pos)
#define PM_APBCMASK_SERCOM2_Pos     4            /**< \brief (PM_APBCMASK) SERCOM2 APB Clock Enable */
#define PM_APBCMASK_SERCOM2         (_U(0x1) << PM_APBCMASK_SERCOM2_Pos)
#define PM_APBCMASK_SERCOM3_Pos     5            /**< \brief (PM_APBCMASK) SERCOM3 APB Clock Enable */
#define PM_APBCMASK_SERCOM3         (_U(0x1) << PM_APBCMASK_SERCOM3_Pos)
#define PM_APBCMASK_SERCOM4_Pos     6            /**< \brief (PM_APBCMASK) SERCOM4 APB Clock Enable */
#define PM_APBCMASK_SERCOM4         (_U(0x1) << PM_APBCMASK_SERCOM4_Pos)
#define PM_APBCMASK_SERCOM5_Pos     7            /**< \brief (PM_APBCMASK) SERCOM5 APB Clock Enable */
#define PM_APBCMASK_SERCOM5         (_U(0x1) << PM_APBCMASK_SERCOM5_Pos)
#define PM_APBCMASK_TC0_Pos         8            /**< \brief (PM_APBCMASK) TC0 APB Clock Enable */
#define PM_APBCMASK_TC0             (_U(0x1) << PM_APBCMASK_TC0_Pos)
#define PM_APBCMASK_TC1_Pos         9            /**< \brief (PM_APBCMASK) TC1 APB Clock Enable */
#define PM_APBCMASK_TC1             (_U(0x1) << PM_APBCMASK_TC1_Pos)
#define PM_APBCMASK_TC2_Pos         10           /**< \brief (PM_APBCMASK) TC2 APB Clock Enable */
#define PM_APBCMASK_TC2             (_U(0x1) << PM_APBCMASK_TC2_Pos)
#define PM_APBCMASK_TC3_Pos         11           /**< \brief (PM_APBCMASK) TC3 APB Clock Enable */
#define PM_APBCMASK_TC3             (_U(0x1) << PM_APBCMASK_TC3_Pos)
#define PM_APBCMASK_TC4_Pos         12           /**< \brief (PM_APBCMASK) TC4 APB Clock Enable */
#define PM_APBCMASK_TC4             (_U(0x1) << PM_APBCMASK_TC4_Pos)
#define PM_APBCMASK_TC5_Pos         13           /**< \brief (PM_APBCMASK) TC5 APB Clock Enable */
#define PM_APBCMASK_TC5             (_U(0x1) << PM_APBCMASK_TC5_Pos)
#define PM_APBCMASK_TC6_Pos         14           /**< \brief (PM_APBCMASK) TC6 APB Clock Enable */
#define PM_APBCMASK_TC6             (_U(0x1) << PM_APBCMASK_TC6_Pos)
#define PM_APBCMASK_TC7_Pos         15           /**< \brief (PM_APBCMASK) TC7 APB Clock Enable */
#define PM_APBCMASK_TC7             (_U(0x1) << PM_APBCMASK_TC7_Pos)
#define PM_APBCMASK_ADC_Pos         16           /**< \brief (PM_APBCMASK) ADC APB Clock Enable */
#define PM_APBCMASK_ADC             (_U(0x1) << PM_APBCMASK_ADC_Pos)
#define PM_APBCMASK_AC_Pos          17           /**< \brief (PM_APBCMASK) AC APB Clock Enable */
#define PM_APBCMASK_AC              (_U(0x1) << PM_APBCMASK_AC_Pos)
#define PM_APBCMASK_DAC_Pos         18           /**< \brief (PM_APBCMASK) DAC APB Clock Enable */
#define PM_APBCMASK_DAC             (_U(0x1) << PM_APBCMASK_DAC_Pos)
#define PM_APBCMASK_PTC_Pos         19           /**< \brief (PM_APBCMASK) PTC APB Clock Enable */
#define PM_APBCMASK_PTC             (_U(0x1) << PM_APBCMASK_PTC_Pos)
#define PM_APBCMASK_MASK            _U(0x000FFFFF) /**< \brief (PM_APBCMASK) MASK Register */

#define PM_INTENCLR_OFFSET          0x34         /**< \brief (PM_INTENCLR offset) Interrupt Enable Clear */
#define PM_INTENCLR_RESETVALUE      _U(0x00);    /**< \brief (PM_INTENCLR reset_value) Interrupt Enable Clear */

#define PM_INTENCLR_CKRDY_Pos       0            /**< \brief (PM_INTENCLR) Clock Ready Interrupt Enable */
#define PM_INTENCLR_CKRDY           (_U(0x1) << PM_INTENCLR_CKRDY_Pos)
#define PM_INTENCLR_MASK            _U(0x01)     /**< \brief (PM_INTENCLR) MASK Register */

#define PM_INTENSET_OFFSET          0x35         /**< \brief (PM_INTENSET offset) Interrupt Enable Set */
#define PM_INTENSET_RESETVALUE      _U(0x00);    /**< \brief (PM_INTENSET reset_value) Interrupt Enable Set */

#define PM_INTENSET_CKRDY_Pos       0            /**< \brief (PM_INTENSET) Clock Ready Interrupt Enable */
#define PM_INTENSET_CKRDY           (_U(0x1) << PM_INTENSET_CKRDY_Pos)
#define PM_INTENSET_MASK            _U(0x01)     /**< \brief (PM_INTENSET) MASK Register */

#define PM_INTFLAG_OFFSET           0x36         /**< \brief (PM_INTFLAG offset) Interrupt Flag Status and Clear */
#define PM_INTFLAG_RESETVALUE       _U(0x00);    /**< \brief (PM_INTFLAG reset_value) Interrupt Flag Status and Clear */

#define PM_INTFLAG_CKRDY_Pos        0            /**< \brief (PM_INTFLAG) Clock Ready */
#define PM_INTFLAG_CKRDY            (_U(0x1) << PM_INTFLAG_CKRDY_Pos)
#define PM_INTFLAG_MASK             _U(0x01)     /**< \brief (PM_INTFLAG) MASK Register */

#define PM_RCAUSE_OFFSET            0x38         /**< \brief (PM_RCAUSE offset) Reset Cause */
#define PM_RCAUSE_RESETVALUE        _U(0x01);    /**< \brief (PM_RCAUSE reset_value) Reset Cause */

#define PM_RCAUSE_POR_Pos           0            /**< \brief (PM_RCAUSE) Power On Reset */
#define PM_RCAUSE_POR               (_U(0x1) << PM_RCAUSE_POR_Pos)
#define PM_RCAUSE_BOD12_Pos         1            /**< \brief (PM_RCAUSE) Brown Out 12 Detector Reset */
#define PM_RCAUSE_BOD12             (_U(0x1) << PM_RCAUSE_BOD12_Pos)
#define PM_RCAUSE_BOD33_Pos         2            /**< \brief (PM_RCAUSE) Brown Out 33 Detector Reset */
#define PM_RCAUSE_BOD33             (_U(0x1) << PM_RCAUSE_BOD33_Pos)
#define PM_RCAUSE_EXT_Pos           4            /**< \brief (PM_RCAUSE) External Reset */
#define PM_RCAUSE_EXT               (_U(0x1) << PM_RCAUSE_EXT_Pos)
#define PM_RCAUSE_WDT_Pos           5            /**< \brief (PM_RCAUSE) Watchdog Reset */
#define PM_RCAUSE_WDT               (_U(0x1) << PM_RCAUSE_WDT_Pos)
#define PM_RCAUSE_SYST_Pos          6            /**< \brief (PM_RCAUSE) System Reset Request */
#define PM_RCAUSE_SYST              (_U(0x1) << PM_RCAUSE_SYST_Pos)
#define PM_RCAUSE_MASK              _U(0x77)     /**< \brief (PM_RCAUSE) MASK Register */

/** \brief PM hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))

typedef struct 
{
	__IO uint8_t	CTRL;			/**< \brief Offset: 0x00 (R/W  8) Control */
	__IO uint8_t	SLEEP;			/**< \brief Offset: 0x01 (R/W  8) Sleep Mode */
		 RoReg8		Reserved1[0x6];
	__IO uint8_t	CPUSEL;			/**< \brief Offset: 0x08 (R/W  8) CPU Clock Select */
	__IO uint8_t	APBASEL;		/**< \brief Offset: 0x09 (R/W  8) APBA Clock Select */
	__IO uint8_t	APBBSEL;		/**< \brief Offset: 0x0A (R/W  8) APBB Clock Select */
	__IO uint8_t	APBCSEL;		/**< \brief Offset: 0x0B (R/W  8) APBC Clock Select */
		 RoReg8		Reserved2[0x8];
	__IO uint32_t	AHBMASK;		/**< \brief Offset: 0x14 (R/W 32) AHB Mask */
	__IO uint32_t	APBAMASK;		/**< \brief Offset: 0x18 (R/W 32) APBA Mask */
	__IO uint32_t	APBBMASK;		/**< \brief Offset: 0x1C (R/W 32) APBB Mask */
	__IO uint32_t	APBCMASK;		/**< \brief Offset: 0x20 (R/W 32) APBC Mask */
		 RoReg8		Reserved3[0x10];
	__IO uint8_t	INTENCLR;		/**< \brief Offset: 0x34 (R/W  8) Interrupt Enable Clear */
	__IO uint8_t	INTENSET;		/**< \brief Offset: 0x35 (R/W  8) Interrupt Enable Set */
	__IO uint8_t	INTFLAG;		/**< \brief Offset: 0x36 (R/W  8) Interrupt Flag Status and Clear */
		 RoReg8		Reserved4[0x1];
	__I  uint8_t	RCAUSE;			/**< \brief Offset: 0x38 (R/   8) Reset Cause */
} Pm;

#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAMD20_PM_COMPONENT_ */
