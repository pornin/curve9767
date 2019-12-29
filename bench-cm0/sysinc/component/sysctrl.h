/**
 * \file
 *
 * \brief Component description for SYSCTRL
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

#ifndef _SAMD20_SYSCTRL_COMPONENT_
#define _SAMD20_SYSCTRL_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR SYSCTRL */
/* ========================================================================== */
/** \addtogroup SAMD20_SYSCTRL System Control */
/*@{*/

#define SYSCTRL_U2100
#define REV_SYSCTRL                 0x201

#define SYSCTRL_INTENCLR_OFFSET     0x00         /**< \brief (SYSCTRL_INTENCLR offset) Interrupt Enable Clear */
#define SYSCTRL_INTENCLR_RESETVALUE _U(0x00000000); /**< \brief (SYSCTRL_INTENCLR reset_value) Interrupt Enable Clear */

#define SYSCTRL_INTENCLR_XOSCRDY_Pos 0            /**< \brief (SYSCTRL_INTENCLR) XOSC Ready */
#define SYSCTRL_INTENCLR_XOSCRDY    (_U(0x1) << SYSCTRL_INTENCLR_XOSCRDY_Pos)
#define SYSCTRL_INTENCLR_XOSC32KRDY_Pos 1            /**< \brief (SYSCTRL_INTENCLR) XOSC32K Ready */
#define SYSCTRL_INTENCLR_XOSC32KRDY (_U(0x1) << SYSCTRL_INTENCLR_XOSC32KRDY_Pos)
#define SYSCTRL_INTENCLR_OSC32KRDY_Pos 2            /**< \brief (SYSCTRL_INTENCLR) OSC32K Ready */
#define SYSCTRL_INTENCLR_OSC32KRDY  (_U(0x1) << SYSCTRL_INTENCLR_OSC32KRDY_Pos)
#define SYSCTRL_INTENCLR_OSC8MRDY_Pos 3            /**< \brief (SYSCTRL_INTENCLR) OSC8M Ready */
#define SYSCTRL_INTENCLR_OSC8MRDY   (_U(0x1) << SYSCTRL_INTENCLR_OSC8MRDY_Pos)
#define SYSCTRL_INTENCLR_DFLLRDY_Pos 4            /**< \brief (SYSCTRL_INTENCLR) DFLL Ready */
#define SYSCTRL_INTENCLR_DFLLRDY    (_U(0x1) << SYSCTRL_INTENCLR_DFLLRDY_Pos)
#define SYSCTRL_INTENCLR_DFLLOOB_Pos 5            /**< \brief (SYSCTRL_INTENCLR) DFLL Out Of Bounds */
#define SYSCTRL_INTENCLR_DFLLOOB    (_U(0x1) << SYSCTRL_INTENCLR_DFLLOOB_Pos)
#define SYSCTRL_INTENCLR_DFLLLCKF_Pos 6            /**< \brief (SYSCTRL_INTENCLR) DFLL Lock Fine */
#define SYSCTRL_INTENCLR_DFLLLCKF   (_U(0x1) << SYSCTRL_INTENCLR_DFLLLCKF_Pos)
#define SYSCTRL_INTENCLR_DFLLLCKC_Pos 7            /**< \brief (SYSCTRL_INTENCLR) DFLL Lock Coarse */
#define SYSCTRL_INTENCLR_DFLLLCKC   (_U(0x1) << SYSCTRL_INTENCLR_DFLLLCKC_Pos)
#define SYSCTRL_INTENCLR_DFLLRCS_Pos 8            /**< \brief (SYSCTRL_INTENCLR) DFLL Reference Clock Stopped */
#define SYSCTRL_INTENCLR_DFLLRCS    (_U(0x1) << SYSCTRL_INTENCLR_DFLLRCS_Pos)
#define SYSCTRL_INTENCLR_BOD33RDY_Pos 9            /**< \brief (SYSCTRL_INTENCLR) BOD33 Ready */
#define SYSCTRL_INTENCLR_BOD33RDY   (_U(0x1) << SYSCTRL_INTENCLR_BOD33RDY_Pos)
#define SYSCTRL_INTENCLR_BOD33DET_Pos 10           /**< \brief (SYSCTRL_INTENCLR) BOD33 Detection */
#define SYSCTRL_INTENCLR_BOD33DET   (_U(0x1) << SYSCTRL_INTENCLR_BOD33DET_Pos)
#define SYSCTRL_INTENCLR_B33SRDY_Pos 11           /**< \brief (SYSCTRL_INTENCLR) BOD33 Synchronization Ready */
#define SYSCTRL_INTENCLR_B33SRDY    (_U(0x1) << SYSCTRL_INTENCLR_B33SRDY_Pos)
#define SYSCTRL_INTENCLR_MASK       _U(0x00000FFF) /**< \brief (SYSCTRL_INTENCLR) MASK Register */

#define SYSCTRL_INTENSET_OFFSET     0x04         /**< \brief (SYSCTRL_INTENSET offset) Interrupt Enable Set */
#define SYSCTRL_INTENSET_RESETVALUE _U(0x00000000); /**< \brief (SYSCTRL_INTENSET reset_value) Interrupt Enable Set */

#define SYSCTRL_INTENSET_XOSCRDY_Pos 0            /**< \brief (SYSCTRL_INTENSET) XOSC Ready */
#define SYSCTRL_INTENSET_XOSCRDY    (_U(0x1) << SYSCTRL_INTENSET_XOSCRDY_Pos)
#define SYSCTRL_INTENSET_XOSC32KRDY_Pos 1            /**< \brief (SYSCTRL_INTENSET) XOSC32K Ready */
#define SYSCTRL_INTENSET_XOSC32KRDY (_U(0x1) << SYSCTRL_INTENSET_XOSC32KRDY_Pos)
#define SYSCTRL_INTENSET_OSC32KRDY_Pos 2            /**< \brief (SYSCTRL_INTENSET) OSC32K Ready */
#define SYSCTRL_INTENSET_OSC32KRDY  (_U(0x1) << SYSCTRL_INTENSET_OSC32KRDY_Pos)
#define SYSCTRL_INTENSET_OSC8MRDY_Pos 3            /**< \brief (SYSCTRL_INTENSET) OSC8M Ready */
#define SYSCTRL_INTENSET_OSC8MRDY   (_U(0x1) << SYSCTRL_INTENSET_OSC8MRDY_Pos)
#define SYSCTRL_INTENSET_DFLLRDY_Pos 4            /**< \brief (SYSCTRL_INTENSET) DFLL Ready */
#define SYSCTRL_INTENSET_DFLLRDY    (_U(0x1) << SYSCTRL_INTENSET_DFLLRDY_Pos)
#define SYSCTRL_INTENSET_DFLLOOB_Pos 5            /**< \brief (SYSCTRL_INTENSET) DFLL Out Of Bounds */
#define SYSCTRL_INTENSET_DFLLOOB    (_U(0x1) << SYSCTRL_INTENSET_DFLLOOB_Pos)
#define SYSCTRL_INTENSET_DFLLLCKF_Pos 6            /**< \brief (SYSCTRL_INTENSET) DFLL Lock Fine */
#define SYSCTRL_INTENSET_DFLLLCKF   (_U(0x1) << SYSCTRL_INTENSET_DFLLLCKF_Pos)
#define SYSCTRL_INTENSET_DFLLLCKC_Pos 7            /**< \brief (SYSCTRL_INTENSET) DFLL Lock Coarse */
#define SYSCTRL_INTENSET_DFLLLCKC   (_U(0x1) << SYSCTRL_INTENSET_DFLLLCKC_Pos)
#define SYSCTRL_INTENSET_DFLLRCS_Pos 8            /**< \brief (SYSCTRL_INTENSET) DFLL Reference Clock Stopped */
#define SYSCTRL_INTENSET_DFLLRCS    (_U(0x1) << SYSCTRL_INTENSET_DFLLRCS_Pos)
#define SYSCTRL_INTENSET_BOD33RDY_Pos 9            /**< \brief (SYSCTRL_INTENSET) BOD33 Ready */
#define SYSCTRL_INTENSET_BOD33RDY   (_U(0x1) << SYSCTRL_INTENSET_BOD33RDY_Pos)
#define SYSCTRL_INTENSET_BOD33DET_Pos 10           /**< \brief (SYSCTRL_INTENSET) BOD33 Detection */
#define SYSCTRL_INTENSET_BOD33DET   (_U(0x1) << SYSCTRL_INTENSET_BOD33DET_Pos)
#define SYSCTRL_INTENSET_B33SRDY_Pos 11           /**< \brief (SYSCTRL_INTENSET) BOD33 Synchronization Ready */
#define SYSCTRL_INTENSET_B33SRDY    (_U(0x1) << SYSCTRL_INTENSET_B33SRDY_Pos)
#define SYSCTRL_INTENSET_MASK       _U(0x00000FFF) /**< \brief (SYSCTRL_INTENSET) MASK Register */

#define SYSCTRL_INTFLAG_OFFSET      0x08         /**< \brief (SYSCTRL_INTFLAG offset) Interrupt Flag Status and Clear */
#define SYSCTRL_INTFLAG_RESETVALUE  _U(0x00000000); /**< \brief (SYSCTRL_INTFLAG reset_value) Interrupt Flag Status and Clear */

#define SYSCTRL_INTFLAG_XOSCRDY_Pos 0            /**< \brief (SYSCTRL_INTFLAG) XOSC Ready */
#define SYSCTRL_INTFLAG_XOSCRDY     (_U(0x1) << SYSCTRL_INTFLAG_XOSCRDY_Pos)
#define SYSCTRL_INTFLAG_XOSC32KRDY_Pos 1            /**< \brief (SYSCTRL_INTFLAG) XOSC32K Ready */
#define SYSCTRL_INTFLAG_XOSC32KRDY  (_U(0x1) << SYSCTRL_INTFLAG_XOSC32KRDY_Pos)
#define SYSCTRL_INTFLAG_OSC32KRDY_Pos 2            /**< \brief (SYSCTRL_INTFLAG) OSC32K Ready */
#define SYSCTRL_INTFLAG_OSC32KRDY   (_U(0x1) << SYSCTRL_INTFLAG_OSC32KRDY_Pos)
#define SYSCTRL_INTFLAG_OSC8MRDY_Pos 3            /**< \brief (SYSCTRL_INTFLAG) OSC8M Ready */
#define SYSCTRL_INTFLAG_OSC8MRDY    (_U(0x1) << SYSCTRL_INTFLAG_OSC8MRDY_Pos)
#define SYSCTRL_INTFLAG_DFLLRDY_Pos 4            /**< \brief (SYSCTRL_INTFLAG) DFLL Ready */
#define SYSCTRL_INTFLAG_DFLLRDY     (_U(0x1) << SYSCTRL_INTFLAG_DFLLRDY_Pos)
#define SYSCTRL_INTFLAG_DFLLOOB_Pos 5            /**< \brief (SYSCTRL_INTFLAG) DFLL Out Of Bounds */
#define SYSCTRL_INTFLAG_DFLLOOB     (_U(0x1) << SYSCTRL_INTFLAG_DFLLOOB_Pos)
#define SYSCTRL_INTFLAG_DFLLLCKF_Pos 6            /**< \brief (SYSCTRL_INTFLAG) DFLL Lock Fine */
#define SYSCTRL_INTFLAG_DFLLLCKF    (_U(0x1) << SYSCTRL_INTFLAG_DFLLLCKF_Pos)
#define SYSCTRL_INTFLAG_DFLLLCKC_Pos 7            /**< \brief (SYSCTRL_INTFLAG) DFLL Lock Coarse */
#define SYSCTRL_INTFLAG_DFLLLCKC    (_U(0x1) << SYSCTRL_INTFLAG_DFLLLCKC_Pos)
#define SYSCTRL_INTFLAG_DFLLRCS_Pos 8            /**< \brief (SYSCTRL_INTFLAG) DFLL Reference Clock Stopped */
#define SYSCTRL_INTFLAG_DFLLRCS     (_U(0x1) << SYSCTRL_INTFLAG_DFLLRCS_Pos)
#define SYSCTRL_INTFLAG_BOD33RDY_Pos 9            /**< \brief (SYSCTRL_INTFLAG) BOD33 Ready */
#define SYSCTRL_INTFLAG_BOD33RDY    (_U(0x1) << SYSCTRL_INTFLAG_BOD33RDY_Pos)
#define SYSCTRL_INTFLAG_BOD33DET_Pos 10           /**< \brief (SYSCTRL_INTFLAG) BOD33 Detection */
#define SYSCTRL_INTFLAG_BOD33DET    (_U(0x1) << SYSCTRL_INTFLAG_BOD33DET_Pos)
#define SYSCTRL_INTFLAG_B33SRDY_Pos 11           /**< \brief (SYSCTRL_INTFLAG) BOD33 Synchronization Ready */
#define SYSCTRL_INTFLAG_B33SRDY     (_U(0x1) << SYSCTRL_INTFLAG_B33SRDY_Pos)
#define SYSCTRL_INTFLAG_MASK        _U(0x00000FFF) /**< \brief (SYSCTRL_INTFLAG) MASK Register */

#define SYSCTRL_PCLKSR_OFFSET       0x0C         /**< \brief (SYSCTRL_PCLKSR offset) Power and Clocks Status */
#define SYSCTRL_PCLKSR_RESETVALUE   _U(0x00000000); /**< \brief (SYSCTRL_PCLKSR reset_value) Power and Clocks Status */

#define SYSCTRL_PCLKSR_XOSCRDY_Pos  0            /**< \brief (SYSCTRL_PCLKSR) XOSC Ready */
#define SYSCTRL_PCLKSR_XOSCRDY      (_U(0x1) << SYSCTRL_PCLKSR_XOSCRDY_Pos)
#define SYSCTRL_PCLKSR_XOSC32KRDY_Pos 1            /**< \brief (SYSCTRL_PCLKSR) XOSC32K Ready */
#define SYSCTRL_PCLKSR_XOSC32KRDY   (_U(0x1) << SYSCTRL_PCLKSR_XOSC32KRDY_Pos)
#define SYSCTRL_PCLKSR_OSC32KRDY_Pos 2            /**< \brief (SYSCTRL_PCLKSR) OSC32K Ready */
#define SYSCTRL_PCLKSR_OSC32KRDY    (_U(0x1) << SYSCTRL_PCLKSR_OSC32KRDY_Pos)
#define SYSCTRL_PCLKSR_OSC8MRDY_Pos 3            /**< \brief (SYSCTRL_PCLKSR) OSC8M Ready */
#define SYSCTRL_PCLKSR_OSC8MRDY     (_U(0x1) << SYSCTRL_PCLKSR_OSC8MRDY_Pos)
#define SYSCTRL_PCLKSR_DFLLRDY_Pos  4            /**< \brief (SYSCTRL_PCLKSR) DFLL Ready */
#define SYSCTRL_PCLKSR_DFLLRDY      (_U(0x1) << SYSCTRL_PCLKSR_DFLLRDY_Pos)
#define SYSCTRL_PCLKSR_DFLLOOB_Pos  5            /**< \brief (SYSCTRL_PCLKSR) DFLL Out Of Bounds */
#define SYSCTRL_PCLKSR_DFLLOOB      (_U(0x1) << SYSCTRL_PCLKSR_DFLLOOB_Pos)
#define SYSCTRL_PCLKSR_DFLLLCKF_Pos 6            /**< \brief (SYSCTRL_PCLKSR) DFLL Lock Fine */
#define SYSCTRL_PCLKSR_DFLLLCKF     (_U(0x1) << SYSCTRL_PCLKSR_DFLLLCKF_Pos)
#define SYSCTRL_PCLKSR_DFLLLCKC_Pos 7            /**< \brief (SYSCTRL_PCLKSR) DFLL Lock Coarse */
#define SYSCTRL_PCLKSR_DFLLLCKC     (_U(0x1) << SYSCTRL_PCLKSR_DFLLLCKC_Pos)
#define SYSCTRL_PCLKSR_DFLLRCS_Pos  8            /**< \brief (SYSCTRL_PCLKSR) DFLL Reference Clock Stopped */
#define SYSCTRL_PCLKSR_DFLLRCS      (_U(0x1) << SYSCTRL_PCLKSR_DFLLRCS_Pos)
#define SYSCTRL_PCLKSR_BOD33RDY_Pos 9            /**< \brief (SYSCTRL_PCLKSR) BOD33 Ready */
#define SYSCTRL_PCLKSR_BOD33RDY     (_U(0x1) << SYSCTRL_PCLKSR_BOD33RDY_Pos)
#define SYSCTRL_PCLKSR_BOD33DET_Pos 10           /**< \brief (SYSCTRL_PCLKSR) BOD33 Detection */
#define SYSCTRL_PCLKSR_BOD33DET     (_U(0x1) << SYSCTRL_PCLKSR_BOD33DET_Pos)
#define SYSCTRL_PCLKSR_B33SRDY_Pos  11           /**< \brief (SYSCTRL_PCLKSR) BOD33 Synchronization Ready */
#define SYSCTRL_PCLKSR_B33SRDY      (_U(0x1) << SYSCTRL_PCLKSR_B33SRDY_Pos)
#define SYSCTRL_PCLKSR_MASK         _U(0x00000FFF) /**< \brief (SYSCTRL_PCLKSR) MASK Register */

#define SYSCTRL_XOSC_OFFSET         0x10         /**< \brief (SYSCTRL_XOSC offset) XOSC Control */
#define SYSCTRL_XOSC_RESETVALUE     _U(0x0080);  /**< \brief (SYSCTRL_XOSC reset_value) XOSC Control */

#define SYSCTRL_XOSC_ENABLE_Pos     1            /**< \brief (SYSCTRL_XOSC) Enable */
#define SYSCTRL_XOSC_ENABLE         (_U(0x1) << SYSCTRL_XOSC_ENABLE_Pos)
#define SYSCTRL_XOSC_XTALEN_Pos     2            /**< \brief (SYSCTRL_XOSC) Crystal Oscillator Enable */
#define SYSCTRL_XOSC_XTALEN         (_U(0x1) << SYSCTRL_XOSC_XTALEN_Pos)
#define SYSCTRL_XOSC_RUNSTDBY_Pos   6            /**< \brief (SYSCTRL_XOSC) Run during Standby */
#define SYSCTRL_XOSC_RUNSTDBY       (_U(0x1) << SYSCTRL_XOSC_RUNSTDBY_Pos)
#define SYSCTRL_XOSC_ONDEMAND_Pos   7            /**< \brief (SYSCTRL_XOSC) Enable on Demand */
#define SYSCTRL_XOSC_ONDEMAND       (_U(0x1) << SYSCTRL_XOSC_ONDEMAND_Pos)
#define SYSCTRL_XOSC_GAIN_Pos       8            /**< \brief (SYSCTRL_XOSC) Gain Value */
#define SYSCTRL_XOSC_GAIN_Msk       (_U(0x7) << SYSCTRL_XOSC_GAIN_Pos)
#define SYSCTRL_XOSC_GAIN(value)    (SYSCTRL_XOSC_GAIN_Msk & ((value) << SYSCTRL_XOSC_GAIN_Pos))
#define SYSCTRL_XOSC_AMPGC_Pos      11           /**< \brief (SYSCTRL_XOSC) Automatic Amplitude Gain Control */
#define SYSCTRL_XOSC_AMPGC          (_U(0x1) << SYSCTRL_XOSC_AMPGC_Pos)
#define SYSCTRL_XOSC_STARTUP_Pos    12           /**< \brief (SYSCTRL_XOSC) Start-Up Time */
#define SYSCTRL_XOSC_STARTUP_Msk    (_U(0xF) << SYSCTRL_XOSC_STARTUP_Pos)
#define SYSCTRL_XOSC_STARTUP(value) (SYSCTRL_XOSC_STARTUP_Msk & ((value) << SYSCTRL_XOSC_STARTUP_Pos))
#define SYSCTRL_XOSC_MASK           _U(0xFFC6)   /**< \brief (SYSCTRL_XOSC) MASK Register */

#define SYSCTRL_XOSC32K_OFFSET      0x14         /**< \brief (SYSCTRL_XOSC32K offset) XOSC32K Control */
#define SYSCTRL_XOSC32K_RESETVALUE  _U(0x0080);  /**< \brief (SYSCTRL_XOSC32K reset_value) XOSC32K Control */

#define SYSCTRL_XOSC32K_ENABLE_Pos  1            /**< \brief (SYSCTRL_XOSC32K) Enable */
#define SYSCTRL_XOSC32K_ENABLE      (_U(0x1) << SYSCTRL_XOSC32K_ENABLE_Pos)
#define SYSCTRL_XOSC32K_XTALEN_Pos  2            /**< \brief (SYSCTRL_XOSC32K) Crystal Oscillator Enable */
#define SYSCTRL_XOSC32K_XTALEN      (_U(0x1) << SYSCTRL_XOSC32K_XTALEN_Pos)
#define SYSCTRL_XOSC32K_EN32K_Pos   3            /**< \brief (SYSCTRL_XOSC32K) 32kHz Output Enable */
#define SYSCTRL_XOSC32K_EN32K       (_U(0x1) << SYSCTRL_XOSC32K_EN32K_Pos)
#define SYSCTRL_XOSC32K_EN1K_Pos    4            /**< \brief (SYSCTRL_XOSC32K) 1kHz Output Enable */
#define SYSCTRL_XOSC32K_EN1K        (_U(0x1) << SYSCTRL_XOSC32K_EN1K_Pos)
#define SYSCTRL_XOSC32K_AAMPEN_Pos  5            /**< \brief (SYSCTRL_XOSC32K) Automatic Amplitude Control Enable */
#define SYSCTRL_XOSC32K_AAMPEN      (_U(0x1) << SYSCTRL_XOSC32K_AAMPEN_Pos)
#define SYSCTRL_XOSC32K_RUNSTDBY_Pos 6            /**< \brief (SYSCTRL_XOSC32K) Run during Standby */
#define SYSCTRL_XOSC32K_RUNSTDBY    (_U(0x1) << SYSCTRL_XOSC32K_RUNSTDBY_Pos)
#define SYSCTRL_XOSC32K_ONDEMAND_Pos 7            /**< \brief (SYSCTRL_XOSC32K) Enable on Demand */
#define SYSCTRL_XOSC32K_ONDEMAND    (_U(0x1) << SYSCTRL_XOSC32K_ONDEMAND_Pos)
#define SYSCTRL_XOSC32K_STARTUP_Pos 8            /**< \brief (SYSCTRL_XOSC32K) Start-Up Time */
#define SYSCTRL_XOSC32K_STARTUP_Msk (_U(0x7) << SYSCTRL_XOSC32K_STARTUP_Pos)
#define SYSCTRL_XOSC32K_STARTUP(value) (SYSCTRL_XOSC32K_STARTUP_Msk & ((value) << SYSCTRL_XOSC32K_STARTUP_Pos))
#define SYSCTRL_XOSC32K_WRTLOCK_Pos 12           /**< \brief (SYSCTRL_XOSC32K) Write Lock */
#define SYSCTRL_XOSC32K_WRTLOCK     (_U(0x1) << SYSCTRL_XOSC32K_WRTLOCK_Pos)
#define SYSCTRL_XOSC32K_MASK        _U(0x17FE)   /**< \brief (SYSCTRL_XOSC32K) MASK Register */

#define SYSCTRL_OSC32K_OFFSET       0x18         /**< \brief (SYSCTRL_OSC32K offset) OSC32K Control */
#define SYSCTRL_OSC32K_RESETVALUE   _U(0x003F0080); /**< \brief (SYSCTRL_OSC32K reset_value) OSC32K Control */

#define SYSCTRL_OSC32K_ENABLE_Pos   1            /**< \brief (SYSCTRL_OSC32K) Enable */
#define SYSCTRL_OSC32K_ENABLE       (_U(0x1) << SYSCTRL_OSC32K_ENABLE_Pos)
#define SYSCTRL_OSC32K_EN32K_Pos    2            /**< \brief (SYSCTRL_OSC32K) 32kHz Output Enable */
#define SYSCTRL_OSC32K_EN32K        (_U(0x1) << SYSCTRL_OSC32K_EN32K_Pos)
#define SYSCTRL_OSC32K_EN1K_Pos     3            /**< \brief (SYSCTRL_OSC32K) 1kHz Output Enable */
#define SYSCTRL_OSC32K_EN1K         (_U(0x1) << SYSCTRL_OSC32K_EN1K_Pos)
#define SYSCTRL_OSC32K_RUNSTDBY_Pos 6            /**< \brief (SYSCTRL_OSC32K) Run during Standby */
#define SYSCTRL_OSC32K_RUNSTDBY     (_U(0x1) << SYSCTRL_OSC32K_RUNSTDBY_Pos)
#define SYSCTRL_OSC32K_ONDEMAND_Pos 7            /**< \brief (SYSCTRL_OSC32K) Enable on Demand */
#define SYSCTRL_OSC32K_ONDEMAND     (_U(0x1) << SYSCTRL_OSC32K_ONDEMAND_Pos)
#define SYSCTRL_OSC32K_STARTUP_Pos  8            /**< \brief (SYSCTRL_OSC32K) Start-Up Time */
#define SYSCTRL_OSC32K_STARTUP_Msk  (_U(0x7) << SYSCTRL_OSC32K_STARTUP_Pos)
#define SYSCTRL_OSC32K_STARTUP(value) (SYSCTRL_OSC32K_STARTUP_Msk & ((value) << SYSCTRL_OSC32K_STARTUP_Pos))
#define SYSCTRL_OSC32K_WRTLOCK_Pos  12           /**< \brief (SYSCTRL_OSC32K) Write Lock */
#define SYSCTRL_OSC32K_WRTLOCK      (_U(0x1) << SYSCTRL_OSC32K_WRTLOCK_Pos)
#define SYSCTRL_OSC32K_CALIB_Pos    16           /**< \brief (SYSCTRL_OSC32K) Calibration Value */
#define SYSCTRL_OSC32K_CALIB_Msk    (_U(0x7F) << SYSCTRL_OSC32K_CALIB_Pos)
#define SYSCTRL_OSC32K_CALIB(value) (SYSCTRL_OSC32K_CALIB_Msk & ((value) << SYSCTRL_OSC32K_CALIB_Pos))
#define SYSCTRL_OSC32K_MASK         _U(0x007F17CE) /**< \brief (SYSCTRL_OSC32K) MASK Register */

#define SYSCTRL_OSCULP32K_OFFSET    0x1C         /**< \brief (SYSCTRL_OSCULP32K offset) OSCULP32K Control */
#define SYSCTRL_OSCULP32K_RESETVALUE _U(0x0F);    /**< \brief (SYSCTRL_OSCULP32K reset_value) OSCULP32K Control */

#define SYSCTRL_OSCULP32K_CALIB_Pos 0            /**< \brief (SYSCTRL_OSCULP32K) Calibration Value */
#define SYSCTRL_OSCULP32K_CALIB_Msk (_U(0x1F) << SYSCTRL_OSCULP32K_CALIB_Pos)
#define SYSCTRL_OSCULP32K_CALIB(value) (SYSCTRL_OSCULP32K_CALIB_Msk & ((value) << SYSCTRL_OSCULP32K_CALIB_Pos))
#define SYSCTRL_OSCULP32K_WRTLOCK_Pos 7            /**< \brief (SYSCTRL_OSCULP32K) Write Lock */
#define SYSCTRL_OSCULP32K_WRTLOCK   (_U(0x1) << SYSCTRL_OSCULP32K_WRTLOCK_Pos)
#define SYSCTRL_OSCULP32K_MASK      _U(0x9F)     /**< \brief (SYSCTRL_OSCULP32K) MASK Register */

#define SYSCTRL_OSC8M_OFFSET        0x20         /**< \brief (SYSCTRL_OSC8M offset) OSC8M Control A */
#define SYSCTRL_OSC8M_RESETVALUE    _U(0x87070382); /**< \brief (SYSCTRL_OSC8M reset_value) OSC8M Control A */

#define SYSCTRL_OSC8M_ENABLE_Pos    1            /**< \brief (SYSCTRL_OSC8M) Enable */
#define SYSCTRL_OSC8M_ENABLE        (_U(0x1) << SYSCTRL_OSC8M_ENABLE_Pos)
#define SYSCTRL_OSC8M_RUNSTDBY_Pos  6            /**< \brief (SYSCTRL_OSC8M) Run during Standby */
#define SYSCTRL_OSC8M_RUNSTDBY      (_U(0x1) << SYSCTRL_OSC8M_RUNSTDBY_Pos)
#define SYSCTRL_OSC8M_ONDEMAND_Pos  7            /**< \brief (SYSCTRL_OSC8M) Enable on Demand */
#define SYSCTRL_OSC8M_ONDEMAND      (_U(0x1) << SYSCTRL_OSC8M_ONDEMAND_Pos)
#define SYSCTRL_OSC8M_PRESC_Pos     8            /**< \brief (SYSCTRL_OSC8M) Prescaler Select */
#define SYSCTRL_OSC8M_PRESC_Msk     (_U(0x3) << SYSCTRL_OSC8M_PRESC_Pos)
#define SYSCTRL_OSC8M_PRESC(value)  (SYSCTRL_OSC8M_PRESC_Msk & ((value) << SYSCTRL_OSC8M_PRESC_Pos))
#define SYSCTRL_OSC8M_CALIB_Pos     16           /**< \brief (SYSCTRL_OSC8M) Calibration Value */
#define SYSCTRL_OSC8M_CALIB_Msk     (_U(0xFFF) << SYSCTRL_OSC8M_CALIB_Pos)
#define SYSCTRL_OSC8M_CALIB(value)  (SYSCTRL_OSC8M_CALIB_Msk & ((value) << SYSCTRL_OSC8M_CALIB_Pos))
#define SYSCTRL_OSC8M_FRANGE_Pos    30           /**< \brief (SYSCTRL_OSC8M) Frequency Range */
#define SYSCTRL_OSC8M_FRANGE_Msk    (_U(0x3) << SYSCTRL_OSC8M_FRANGE_Pos)
#define SYSCTRL_OSC8M_FRANGE(value) (SYSCTRL_OSC8M_FRANGE_Msk & ((value) << SYSCTRL_OSC8M_FRANGE_Pos))
#define SYSCTRL_OSC8M_MASK          _U(0xCFFF03C2) /**< \brief (SYSCTRL_OSC8M) MASK Register */

#define SYSCTRL_DFLLCTRL_OFFSET     0x24         /**< \brief (SYSCTRL_DFLLCTRL offset) DFLL Config */
#define SYSCTRL_DFLLCTRL_RESETVALUE _U(0x0080);  /**< \brief (SYSCTRL_DFLLCTRL reset_value) DFLL Config */

#define SYSCTRL_DFLLCTRL_ENABLE_Pos 1            /**< \brief (SYSCTRL_DFLLCTRL) Enable */
#define SYSCTRL_DFLLCTRL_ENABLE     (_U(0x1) << SYSCTRL_DFLLCTRL_ENABLE_Pos)
#define SYSCTRL_DFLLCTRL_MODE_Pos   2            /**< \brief (SYSCTRL_DFLLCTRL) Mode Selection */
#define SYSCTRL_DFLLCTRL_MODE       (_U(0x1) << SYSCTRL_DFLLCTRL_MODE_Pos)
#define SYSCTRL_DFLLCTRL_STABLE_Pos 3            /**< \brief (SYSCTRL_DFLLCTRL) Stable Frequency */
#define SYSCTRL_DFLLCTRL_STABLE     (_U(0x1) << SYSCTRL_DFLLCTRL_STABLE_Pos)
#define SYSCTRL_DFLLCTRL_LLAW_Pos   4            /**< \brief (SYSCTRL_DFLLCTRL) Lose Lock After Wake */
#define SYSCTRL_DFLLCTRL_LLAW       (_U(0x1) << SYSCTRL_DFLLCTRL_LLAW_Pos)
#define SYSCTRL_DFLLCTRL_RUNSTDBY_Pos 6            /**< \brief (SYSCTRL_DFLLCTRL) Run during Standby */
#define SYSCTRL_DFLLCTRL_RUNSTDBY   (_U(0x1) << SYSCTRL_DFLLCTRL_RUNSTDBY_Pos)
#define SYSCTRL_DFLLCTRL_ONDEMAND_Pos 7            /**< \brief (SYSCTRL_DFLLCTRL) Enable on Demand */
#define SYSCTRL_DFLLCTRL_ONDEMAND   (_U(0x1) << SYSCTRL_DFLLCTRL_ONDEMAND_Pos)
#define SYSCTRL_DFLLCTRL_CCDIS_Pos  8            /**< \brief (SYSCTRL_DFLLCTRL) Chill Cycle Disable */
#define SYSCTRL_DFLLCTRL_CCDIS      (_U(0x1) << SYSCTRL_DFLLCTRL_CCDIS_Pos)
#define SYSCTRL_DFLLCTRL_QLDIS_Pos  9            /**< \brief (SYSCTRL_DFLLCTRL) Quick Lock Disable */
#define SYSCTRL_DFLLCTRL_QLDIS      (_U(0x1) << SYSCTRL_DFLLCTRL_QLDIS_Pos)
#define SYSCTRL_DFLLCTRL_MASK       _U(0x03DE)   /**< \brief (SYSCTRL_DFLLCTRL) MASK Register */

#define SYSCTRL_DFLLVAL_OFFSET      0x28         /**< \brief (SYSCTRL_DFLLVAL offset) DFLL Calibration Value */
#define SYSCTRL_DFLLVAL_RESETVALUE  _U(0x00000000); /**< \brief (SYSCTRL_DFLLVAL reset_value) DFLL Calibration Value */

#define SYSCTRL_DFLLVAL_FINE_Pos    0            /**< \brief (SYSCTRL_DFLLVAL) Fine Calibration Value */
#define SYSCTRL_DFLLVAL_FINE_Msk    (_U(0x3FF) << SYSCTRL_DFLLVAL_FINE_Pos)
#define SYSCTRL_DFLLVAL_FINE(value) (SYSCTRL_DFLLVAL_FINE_Msk & ((value) << SYSCTRL_DFLLVAL_FINE_Pos))
#define SYSCTRL_DFLLVAL_COARSE_Pos  10           /**< \brief (SYSCTRL_DFLLVAL) Coarse Calibration Value */
#define SYSCTRL_DFLLVAL_COARSE_Msk  (_U(0x3F) << SYSCTRL_DFLLVAL_COARSE_Pos)
#define SYSCTRL_DFLLVAL_COARSE(value) (SYSCTRL_DFLLVAL_COARSE_Msk & ((value) << SYSCTRL_DFLLVAL_COARSE_Pos))
#define SYSCTRL_DFLLVAL_DIFF_Pos    16           /**< \brief (SYSCTRL_DFLLVAL) Multiplication Ratio Difference */
#define SYSCTRL_DFLLVAL_DIFF_Msk    (_U(0xFFFF) << SYSCTRL_DFLLVAL_DIFF_Pos)
#define SYSCTRL_DFLLVAL_DIFF(value) (SYSCTRL_DFLLVAL_DIFF_Msk & ((value) << SYSCTRL_DFLLVAL_DIFF_Pos))
#define SYSCTRL_DFLLVAL_MASK        _U(0xFFFFFFFF) /**< \brief (SYSCTRL_DFLLVAL) MASK Register */

#define SYSCTRL_DFLLMUL_OFFSET      0x2C         /**< \brief (SYSCTRL_DFLLMUL offset) DFLL Multiplier */
#define SYSCTRL_DFLLMUL_RESETVALUE  _U(0x00000000); /**< \brief (SYSCTRL_DFLLMUL reset_value) DFLL Multiplier */

#define SYSCTRL_DFLLMUL_MUL_Pos     0            /**< \brief (SYSCTRL_DFLLMUL) Multiplication Value */
#define SYSCTRL_DFLLMUL_MUL_Msk     (_U(0xFFFF) << SYSCTRL_DFLLMUL_MUL_Pos)
#define SYSCTRL_DFLLMUL_MUL(value)  (SYSCTRL_DFLLMUL_MUL_Msk & ((value) << SYSCTRL_DFLLMUL_MUL_Pos))
#define SYSCTRL_DFLLMUL_FSTEP_Pos   16           /**< \brief (SYSCTRL_DFLLMUL) Maximum Fine Step Size */
#define SYSCTRL_DFLLMUL_FSTEP_Msk   (_U(0x3FF) << SYSCTRL_DFLLMUL_FSTEP_Pos)
#define SYSCTRL_DFLLMUL_FSTEP(value) (SYSCTRL_DFLLMUL_FSTEP_Msk & ((value) << SYSCTRL_DFLLMUL_FSTEP_Pos))
#define SYSCTRL_DFLLMUL_CSTEP_Pos   26           /**< \brief (SYSCTRL_DFLLMUL) Maximum Coarse Step Size */
#define SYSCTRL_DFLLMUL_CSTEP_Msk   (_U(0x3F) << SYSCTRL_DFLLMUL_CSTEP_Pos)
#define SYSCTRL_DFLLMUL_CSTEP(value) (SYSCTRL_DFLLMUL_CSTEP_Msk & ((value) << SYSCTRL_DFLLMUL_CSTEP_Pos))
#define SYSCTRL_DFLLMUL_MASK        _U(0xFFFFFFFF) /**< \brief (SYSCTRL_DFLLMUL) MASK Register */

#define SYSCTRL_DFLLSYNC_OFFSET     0x30         /**< \brief (SYSCTRL_DFLLSYNC offset) DFLL Synchronization */
#define SYSCTRL_DFLLSYNC_RESETVALUE _U(0x00);    /**< \brief (SYSCTRL_DFLLSYNC reset_value) DFLL Synchronization */

#define SYSCTRL_DFLLSYNC_READREQ_Pos 7            /**< \brief (SYSCTRL_DFLLSYNC) Read Request Synchronization */
#define SYSCTRL_DFLLSYNC_READREQ    (_U(0x1) << SYSCTRL_DFLLSYNC_READREQ_Pos)
#define SYSCTRL_DFLLSYNC_MASK       _U(0x80)     /**< \brief (SYSCTRL_DFLLSYNC) MASK Register */

#define SYSCTRL_BOD33_OFFSET        0x34         /**< \brief (SYSCTRL_BOD33 offset) 3.3V Brown-Out Detector (BOD33) Control */
#define SYSCTRL_BOD33_RESETVALUE    _U(0x00000000); /**< \brief (SYSCTRL_BOD33 reset_value) 3.3V Brown-Out Detector (BOD33) Control */

#define SYSCTRL_BOD33_ENABLE_Pos    1            /**< \brief (SYSCTRL_BOD33) Enable */
#define SYSCTRL_BOD33_ENABLE        (_U(0x1) << SYSCTRL_BOD33_ENABLE_Pos)
#define SYSCTRL_BOD33_HYST_Pos      2            /**< \brief (SYSCTRL_BOD33) Hysteresis Enable */
#define SYSCTRL_BOD33_HYST          (_U(0x1) << SYSCTRL_BOD33_HYST_Pos)
#define SYSCTRL_BOD33_ACTION_Pos    3            /**< \brief (SYSCTRL_BOD33) Action when Threshold Crossed */
#define SYSCTRL_BOD33_ACTION_Msk    (_U(0x3) << SYSCTRL_BOD33_ACTION_Pos)
#define SYSCTRL_BOD33_ACTION(value) (SYSCTRL_BOD33_ACTION_Msk & ((value) << SYSCTRL_BOD33_ACTION_Pos))
#define SYSCTRL_BOD33_RUNSTDBY_Pos  6            /**< \brief (SYSCTRL_BOD33) Run during Standby */
#define SYSCTRL_BOD33_RUNSTDBY      (_U(0x1) << SYSCTRL_BOD33_RUNSTDBY_Pos)
#define SYSCTRL_BOD33_MODE_Pos      8            /**< \brief (SYSCTRL_BOD33) Operation Modes */
#define SYSCTRL_BOD33_MODE          (_U(0x1) << SYSCTRL_BOD33_MODE_Pos)
#define SYSCTRL_BOD33_CEN_Pos       9            /**< \brief (SYSCTRL_BOD33) Clock Enable */
#define SYSCTRL_BOD33_CEN           (_U(0x1) << SYSCTRL_BOD33_CEN_Pos)
#define SYSCTRL_BOD33_PSEL_Pos      12           /**< \brief (SYSCTRL_BOD33) Prescaler Select */
#define SYSCTRL_BOD33_PSEL_Msk      (_U(0xF) << SYSCTRL_BOD33_PSEL_Pos)
#define SYSCTRL_BOD33_PSEL(value)   (SYSCTRL_BOD33_PSEL_Msk & ((value) << SYSCTRL_BOD33_PSEL_Pos))
#define SYSCTRL_BOD33_LEVEL_Pos     16           /**< \brief (SYSCTRL_BOD33) Threshold Level */
#define SYSCTRL_BOD33_LEVEL_Msk     (_U(0x3F) << SYSCTRL_BOD33_LEVEL_Pos)
#define SYSCTRL_BOD33_LEVEL(value)  (SYSCTRL_BOD33_LEVEL_Msk & ((value) << SYSCTRL_BOD33_LEVEL_Pos))
#define SYSCTRL_BOD33_MASK          _U(0x003FF35E) /**< \brief (SYSCTRL_BOD33) MASK Register */

#define SYSCTRL_VREG_OFFSET         0x3C         /**< \brief (SYSCTRL_VREG offset) VREG Control */
#define SYSCTRL_VREG_RESETVALUE     _U(0x0402);  /**< \brief (SYSCTRL_VREG reset_value) VREG Control */

#define SYSCTRL_VREG_RUNSTDBY_Pos   6            /**< \brief (SYSCTRL_VREG) Run during Standby */
#define SYSCTRL_VREG_RUNSTDBY       (_U(0x1) << SYSCTRL_VREG_RUNSTDBY_Pos)
#define SYSCTRL_VREG_FORCELDO_Pos   13           /**< \brief (SYSCTRL_VREG) Force LDO Voltage Regulator */
#define SYSCTRL_VREG_FORCELDO       (_U(0x1) << SYSCTRL_VREG_FORCELDO_Pos)
#define SYSCTRL_VREG_MASK           _U(0x2040)   /**< \brief (SYSCTRL_VREG) MASK Register */

#define SYSCTRL_VREF_OFFSET         0x40         /**< \brief (SYSCTRL_VREF offset) VREF Control A */
#define SYSCTRL_VREF_RESETVALUE     _U(0x00000000); /**< \brief (SYSCTRL_VREF reset_value) VREF Control A */

#define SYSCTRL_VREF_TSEN_Pos       1            /**< \brief (SYSCTRL_VREF) Temperature Sensor Output Enable */
#define SYSCTRL_VREF_TSEN           (_U(0x1) << SYSCTRL_VREF_TSEN_Pos)
#define SYSCTRL_VREF_BGOUTEN_Pos    2            /**< \brief (SYSCTRL_VREF) Bandgap Output Enable */
#define SYSCTRL_VREF_BGOUTEN        (_U(0x1) << SYSCTRL_VREF_BGOUTEN_Pos)
#define SYSCTRL_VREF_CALIB_Pos      16           /**< \brief (SYSCTRL_VREF) Voltage Reference Calibration Value */
#define SYSCTRL_VREF_CALIB_Msk      (_U(0x7FF) << SYSCTRL_VREF_CALIB_Pos)
#define SYSCTRL_VREF_CALIB(value)   (SYSCTRL_VREF_CALIB_Msk & ((value) << SYSCTRL_VREF_CALIB_Pos))
#define SYSCTRL_VREF_MASK           _U(0x07FF0006) /**< \brief (SYSCTRL_VREF) MASK Register */

/** \brief SYSCTRL hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))

typedef struct 
{
	__IO uint32_t	INTENCLR;		/**< \brief Offset: 0x00 (R/W 32) Interrupt Enable Clear */
	__IO uint32_t	INTENSET;		/**< \brief Offset: 0x04 (R/W 32) Interrupt Enable Set */
	__IO uint32_t	INTFLAG;		/**< \brief Offset: 0x08 (R/W 32) Interrupt Flag Status and Clear */
	__I  uint32_t	PCLKSR;			/**< \brief Offset: 0x0C (R/  32) Power and Clocks Status */
	__IO uint16_t	XOSC; 			/**< \brief Offset: 0x10 (R/W 16) XOSC Control */
		 RoReg8 	Reserved1[0x2];
	__IO uint16_t	XOSC32K;		/**< \brief Offset: 0x14 (R/W 16) XOSC32K Control */
		 RoReg8 	Reserved2[0x2];
	__IO uint32_t	OSC32K;			/**< \brief Offset: 0x18 (R/W 32) OSC32K Control */
	__IO uint8_t	OSCULP32K;		/**< \brief Offset: 0x1C (R/W  8) OSCULP32K Control */
		 RoReg8 	Reserved3[0x3];
	__IO uint32_t	OSC8M;			/**< \brief Offset: 0x20 (R/W 32) OSC8M Control A */
	__IO uint16_t	DFLLCTRL;		/**< \brief Offset: 0x24 (R/W 16) DFLL Config */
		 RoReg8 	Reserved4[0x2];
	__IO uint32_t	DFLLVAL;		/**< \brief Offset: 0x28 (R/W 32) DFLL Calibration Value */
	__IO uint32_t	DFLLMUL;		/**< \brief Offset: 0x2C (R/W 32) DFLL Multiplier */
	__IO uint8_t	DFLLSYNC;		/**< \brief Offset: 0x30 (R/W  8) DFLL Synchronization */
		 RoReg8 	Reserved5[0x3];
	__IO uint32_t	BOD33;			/**< \brief Offset: 0x34 (R/W 32) 3.3V Brown-Out Detector (BOD33) Control */
		 RoReg8 	Reserved6[0x4];
	__IO uint16_t	VREG;			/**< \brief Offset: 0x3C (R/W 16) VREG Control */
		 RoReg8 	Reserved7[0x2];
	__IO uint32_t	VREF;			/**< \brief Offset: 0x40 (R/W 32) VREF Control A */
} Sysctrl;

#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAMD20_SYSCTRL_COMPONENT_ */
