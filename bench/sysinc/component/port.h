/**
 * \file
 *
 * \brief Component description for PORT
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

#ifndef _SAMD20_PORT_COMPONENT_
#define _SAMD20_PORT_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR PORT */
/* ========================================================================== */
/** \addtogroup SAMD20_PORT Port Module */
/*@{*/

#define PORT_U2210
#define REV_PORT                    0x100

#define PORT_DIR_OFFSET             0x00         /**< \brief (PORT_DIR offset) Data Direction */
#define PORT_DIR_RESETVALUE         _U(0x00000000); /**< \brief (PORT_DIR reset_value) Data Direction */

#define PORT_DIR_DIR_Pos            0            /**< \brief (PORT_DIR) Port Data Direction */
#define PORT_DIR_DIR_Msk            (_U(0xFFFFFFFF) << PORT_DIR_DIR_Pos)
#define PORT_DIR_DIR(value)         (PORT_DIR_DIR_Msk & ((value) << PORT_DIR_DIR_Pos))
#define PORT_DIR_MASK               _U(0xFFFFFFFF) /**< \brief (PORT_DIR) MASK Register */

#define PORT_DIRCLR_OFFSET          0x04         /**< \brief (PORT_DIRCLR offset) Data Direction Clear */
#define PORT_DIRCLR_RESETVALUE      _U(0x00000000); /**< \brief (PORT_DIRCLR reset_value) Data Direction Clear */

#define PORT_DIRCLR_DIRCLR_Pos      0            /**< \brief (PORT_DIRCLR) Port Data Direction Clear */
#define PORT_DIRCLR_DIRCLR_Msk      (_U(0xFFFFFFFF) << PORT_DIRCLR_DIRCLR_Pos)
#define PORT_DIRCLR_DIRCLR(value)   (PORT_DIRCLR_DIRCLR_Msk & ((value) << PORT_DIRCLR_DIRCLR_Pos))
#define PORT_DIRCLR_MASK            _U(0xFFFFFFFF) /**< \brief (PORT_DIRCLR) MASK Register */

#define PORT_DIRSET_OFFSET          0x08         /**< \brief (PORT_DIRSET offset) Data Direction Set */
#define PORT_DIRSET_RESETVALUE      _U(0x00000000); /**< \brief (PORT_DIRSET reset_value) Data Direction Set */

#define PORT_DIRSET_DIRSET_Pos      0            /**< \brief (PORT_DIRSET) Port Data Direction Set */
#define PORT_DIRSET_DIRSET_Msk      (_U(0xFFFFFFFF) << PORT_DIRSET_DIRSET_Pos)
#define PORT_DIRSET_DIRSET(value)   (PORT_DIRSET_DIRSET_Msk & ((value) << PORT_DIRSET_DIRSET_Pos))
#define PORT_DIRSET_MASK            _U(0xFFFFFFFF) /**< \brief (PORT_DIRSET) MASK Register */

#define PORT_DIRTGL_OFFSET          0x0C         /**< \brief (PORT_DIRTGL offset) Data Direction Toggle */
#define PORT_DIRTGL_RESETVALUE      _U(0x00000000); /**< \brief (PORT_DIRTGL reset_value) Data Direction Toggle */

#define PORT_DIRTGL_DIRTGL_Pos      0            /**< \brief (PORT_DIRTGL) Port Data Direction Toggle */
#define PORT_DIRTGL_DIRTGL_Msk      (_U(0xFFFFFFFF) << PORT_DIRTGL_DIRTGL_Pos)
#define PORT_DIRTGL_DIRTGL(value)   (PORT_DIRTGL_DIRTGL_Msk & ((value) << PORT_DIRTGL_DIRTGL_Pos))
#define PORT_DIRTGL_MASK            _U(0xFFFFFFFF) /**< \brief (PORT_DIRTGL) MASK Register */

#define PORT_OUT_OFFSET             0x10         /**< \brief (PORT_OUT offset) Data Output Value */
#define PORT_OUT_RESETVALUE         _U(0x00000000); /**< \brief (PORT_OUT reset_value) Data Output Value */

#define PORT_OUT_OUT_Pos            0            /**< \brief (PORT_OUT) Port Data Output Value */
#define PORT_OUT_OUT_Msk            (_U(0xFFFFFFFF) << PORT_OUT_OUT_Pos)
#define PORT_OUT_OUT(value)         (PORT_OUT_OUT_Msk & ((value) << PORT_OUT_OUT_Pos))
#define PORT_OUT_MASK               _U(0xFFFFFFFF) /**< \brief (PORT_OUT) MASK Register */

#define PORT_OUTCLR_OFFSET          0x14         /**< \brief (PORT_OUTCLR offset) Data Output Value Clear */
#define PORT_OUTCLR_RESETVALUE      _U(0x00000000); /**< \brief (PORT_OUTCLR reset_value) Data Output Value Clear */

#define PORT_OUTCLR_OUTCLR_Pos      0            /**< \brief (PORT_OUTCLR) Port Data Output Value Clear */
#define PORT_OUTCLR_OUTCLR_Msk      (_U(0xFFFFFFFF) << PORT_OUTCLR_OUTCLR_Pos)
#define PORT_OUTCLR_OUTCLR(value)   (PORT_OUTCLR_OUTCLR_Msk & ((value) << PORT_OUTCLR_OUTCLR_Pos))
#define PORT_OUTCLR_MASK            _U(0xFFFFFFFF) /**< \brief (PORT_OUTCLR) MASK Register */

#define PORT_OUTSET_OFFSET          0x18         /**< \brief (PORT_OUTSET offset) Data Output Value Set */
#define PORT_OUTSET_RESETVALUE      _U(0x00000000); /**< \brief (PORT_OUTSET reset_value) Data Output Value Set */

#define PORT_OUTSET_OUTSET_Pos      0            /**< \brief (PORT_OUTSET) Port Data Output Value Set */
#define PORT_OUTSET_OUTSET_Msk      (_U(0xFFFFFFFF) << PORT_OUTSET_OUTSET_Pos)
#define PORT_OUTSET_OUTSET(value)   (PORT_OUTSET_OUTSET_Msk & ((value) << PORT_OUTSET_OUTSET_Pos))
#define PORT_OUTSET_MASK            _U(0xFFFFFFFF) /**< \brief (PORT_OUTSET) MASK Register */

#define PORT_OUTTGL_OFFSET          0x1C         /**< \brief (PORT_OUTTGL offset) Data Output Value Toggle */
#define PORT_OUTTGL_RESETVALUE      _U(0x00000000); /**< \brief (PORT_OUTTGL reset_value) Data Output Value Toggle */

#define PORT_OUTTGL_OUTTGL_Pos      0            /**< \brief (PORT_OUTTGL) Port Data Output Value Toggle */
#define PORT_OUTTGL_OUTTGL_Msk      (_U(0xFFFFFFFF) << PORT_OUTTGL_OUTTGL_Pos)
#define PORT_OUTTGL_OUTTGL(value)   (PORT_OUTTGL_OUTTGL_Msk & ((value) << PORT_OUTTGL_OUTTGL_Pos))
#define PORT_OUTTGL_MASK            _U(0xFFFFFFFF) /**< \brief (PORT_OUTTGL) MASK Register */

#define PORT_IN_OFFSET              0x20         /**< \brief (PORT_IN offset) Data Input Value */
#define PORT_IN_RESETVALUE          _U(0x00000000); /**< \brief (PORT_IN reset_value) Data Input Value */

#define PORT_IN_IN_Pos              0            /**< \brief (PORT_IN) Port Data Input Value */
#define PORT_IN_IN_Msk              (_U(0xFFFFFFFF) << PORT_IN_IN_Pos)
#define PORT_IN_IN(value)           (PORT_IN_IN_Msk & ((value) << PORT_IN_IN_Pos))
#define PORT_IN_MASK                _U(0xFFFFFFFF) /**< \brief (PORT_IN) MASK Register */

#define PORT_CTRL_OFFSET            0x24         /**< \brief (PORT_CTRL offset) Control */
#define PORT_CTRL_RESETVALUE        _U(0x00000000); /**< \brief (PORT_CTRL reset_value) Control */

#define PORT_CTRL_SAMPLING_Pos      0            /**< \brief (PORT_CTRL) Input Sampling Mode */
#define PORT_CTRL_SAMPLING_Msk      (_U(0xFFFFFFFF) << PORT_CTRL_SAMPLING_Pos)
#define PORT_CTRL_SAMPLING(value)   (PORT_CTRL_SAMPLING_Msk & ((value) << PORT_CTRL_SAMPLING_Pos))
#define PORT_CTRL_MASK              _U(0xFFFFFFFF) /**< \brief (PORT_CTRL) MASK Register */

#define PORT_WRCONFIG_OFFSET        0x28         /**< \brief (PORT_WRCONFIG offset) Write Configuration */
#define PORT_WRCONFIG_RESETVALUE    _U(0x00000000); /**< \brief (PORT_WRCONFIG reset_value) Write Configuration */

#define PORT_WRCONFIG_PINMASK_Pos   0            /**< \brief (PORT_WRCONFIG) Pin Mask for Multiple Pin Configuration */
#define PORT_WRCONFIG_PINMASK_Msk   (_U(0xFFFF) << PORT_WRCONFIG_PINMASK_Pos)
#define PORT_WRCONFIG_PINMASK(value) (PORT_WRCONFIG_PINMASK_Msk & ((value) << PORT_WRCONFIG_PINMASK_Pos))
#define PORT_WRCONFIG_PMUXEN_Pos    16           /**< \brief (PORT_WRCONFIG) Peripheral Multiplexer Enable */
#define PORT_WRCONFIG_PMUXEN        (_U(0x1) << PORT_WRCONFIG_PMUXEN_Pos)
#define PORT_WRCONFIG_INEN_Pos      17           /**< \brief (PORT_WRCONFIG) Input Enable */
#define PORT_WRCONFIG_INEN          (_U(0x1) << PORT_WRCONFIG_INEN_Pos)
#define PORT_WRCONFIG_PULLEN_Pos    18           /**< \brief (PORT_WRCONFIG) Pull Enable */
#define PORT_WRCONFIG_PULLEN        (_U(0x1) << PORT_WRCONFIG_PULLEN_Pos)
#define PORT_WRCONFIG_DRVSTR_Pos    22           /**< \brief (PORT_WRCONFIG) Output Driver Strength Selection */
#define PORT_WRCONFIG_DRVSTR        (_U(0x1) << PORT_WRCONFIG_DRVSTR_Pos)
#define PORT_WRCONFIG_PMUX_Pos      24           /**< \brief (PORT_WRCONFIG) Peripheral Multiplexing */
#define PORT_WRCONFIG_PMUX_Msk      (_U(0xF) << PORT_WRCONFIG_PMUX_Pos)
#define PORT_WRCONFIG_PMUX(value)   (PORT_WRCONFIG_PMUX_Msk & ((value) << PORT_WRCONFIG_PMUX_Pos))
#define PORT_WRCONFIG_WRPMUX_Pos    28           /**< \brief (PORT_WRCONFIG) Write PMUX */
#define PORT_WRCONFIG_WRPMUX        (_U(0x1) << PORT_WRCONFIG_WRPMUX_Pos)
#define PORT_WRCONFIG_WRPINCFG_Pos  30           /**< \brief (PORT_WRCONFIG) Write PINCFG */
#define PORT_WRCONFIG_WRPINCFG      (_U(0x1) << PORT_WRCONFIG_WRPINCFG_Pos)
#define PORT_WRCONFIG_HWSEL_Pos     31           /**< \brief (PORT_WRCONFIG) Half-Word Select */
#define PORT_WRCONFIG_HWSEL         (_U(0x1) << PORT_WRCONFIG_HWSEL_Pos)
#define PORT_WRCONFIG_MASK          _U(0xDF47FFFF) /**< \brief (PORT_WRCONFIG) MASK Register */

#define PORT_PMUX_OFFSET            0x30         /**< \brief (PORT_PMUX offset) Peripheral Multiplexing n */
#define PORT_PMUX_RESETVALUE        _U(0x00);    /**< \brief (PORT_PMUX reset_value) Peripheral Multiplexing n */

#define PORT_PMUX_PMUXE_Pos         0            /**< \brief (PORT_PMUX) Peripheral Multiplexing Even */
#define PORT_PMUX_PMUXE_Msk         (_U(0xF) << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE(value)      (PORT_PMUX_PMUXE_Msk & ((value) << PORT_PMUX_PMUXE_Pos))
#define   PORT_PMUX_PMUXE_A_Val           _U(0x0)   /**< \brief (PORT_PMUX) Peripheral function A selected */
#define   PORT_PMUX_PMUXE_B_Val           _U(0x1)   /**< \brief (PORT_PMUX) Peripheral function B selected */
#define   PORT_PMUX_PMUXE_C_Val           _U(0x2)   /**< \brief (PORT_PMUX) Peripheral function C selected */
#define   PORT_PMUX_PMUXE_D_Val           _U(0x3)   /**< \brief (PORT_PMUX) Peripheral function D selected */
#define   PORT_PMUX_PMUXE_E_Val           _U(0x4)   /**< \brief (PORT_PMUX) Peripheral function E selected */
#define   PORT_PMUX_PMUXE_F_Val           _U(0x5)   /**< \brief (PORT_PMUX) Peripheral function F selected */
#define   PORT_PMUX_PMUXE_G_Val           _U(0x6)   /**< \brief (PORT_PMUX) Peripheral function G selected */
#define   PORT_PMUX_PMUXE_H_Val           _U(0x7)   /**< \brief (PORT_PMUX) Peripheral function H selected */
#define PORT_PMUX_PMUXE_A           (PORT_PMUX_PMUXE_A_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_B           (PORT_PMUX_PMUXE_B_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_C           (PORT_PMUX_PMUXE_C_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_D           (PORT_PMUX_PMUXE_D_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_E           (PORT_PMUX_PMUXE_E_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_F           (PORT_PMUX_PMUXE_F_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_G           (PORT_PMUX_PMUXE_G_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXE_H           (PORT_PMUX_PMUXE_H_Val         << PORT_PMUX_PMUXE_Pos)
#define PORT_PMUX_PMUXO_Pos         4            /**< \brief (PORT_PMUX) Peripheral Multiplexing Odd */
#define PORT_PMUX_PMUXO_Msk         (_U(0xF) << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO(value)      (PORT_PMUX_PMUXO_Msk & ((value) << PORT_PMUX_PMUXO_Pos))
#define   PORT_PMUX_PMUXO_A_Val           _U(0x0)   /**< \brief (PORT_PMUX) Peripheral function A selected */
#define   PORT_PMUX_PMUXO_B_Val           _U(0x1)   /**< \brief (PORT_PMUX) Peripheral function B selected */
#define   PORT_PMUX_PMUXO_C_Val           _U(0x2)   /**< \brief (PORT_PMUX) Peripheral function C selected */
#define   PORT_PMUX_PMUXO_D_Val           _U(0x3)   /**< \brief (PORT_PMUX) Peripheral function D selected */
#define   PORT_PMUX_PMUXO_E_Val           _U(0x4)   /**< \brief (PORT_PMUX) Peripheral function E selected */
#define   PORT_PMUX_PMUXO_F_Val           _U(0x5)   /**< \brief (PORT_PMUX) Peripheral function F selected */
#define   PORT_PMUX_PMUXO_G_Val           _U(0x6)   /**< \brief (PORT_PMUX) Peripheral function G selected */
#define   PORT_PMUX_PMUXO_H_Val           _U(0x7)   /**< \brief (PORT_PMUX) Peripheral function H selected */
#define PORT_PMUX_PMUXO_A           (PORT_PMUX_PMUXO_A_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_B           (PORT_PMUX_PMUXO_B_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_C           (PORT_PMUX_PMUXO_C_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_D           (PORT_PMUX_PMUXO_D_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_E           (PORT_PMUX_PMUXO_E_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_F           (PORT_PMUX_PMUXO_F_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_G           (PORT_PMUX_PMUXO_G_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_PMUXO_H           (PORT_PMUX_PMUXO_H_Val         << PORT_PMUX_PMUXO_Pos)
#define PORT_PMUX_MASK              _U(0xFF)     /**< \brief (PORT_PMUX) MASK Register */

#define PORT_PINCFG_OFFSET          0x40         /**< \brief (PORT_PINCFG offset) Pin Configuration n */
#define PORT_PINCFG_RESETVALUE      _U(0x00);    /**< \brief (PORT_PINCFG reset_value) Pin Configuration n */

#define PORT_PINCFG_PMUXEN_Pos      0            /**< \brief (PORT_PINCFG) Peripheral Multiplexer Enable */
#define PORT_PINCFG_PMUXEN          (_U(0x1) << PORT_PINCFG_PMUXEN_Pos)
#define PORT_PINCFG_INEN_Pos        1            /**< \brief (PORT_PINCFG) Input Enable */
#define PORT_PINCFG_INEN            (_U(0x1) << PORT_PINCFG_INEN_Pos)
#define PORT_PINCFG_PULLEN_Pos      2            /**< \brief (PORT_PINCFG) Pull Enable */
#define PORT_PINCFG_PULLEN          (_U(0x1) << PORT_PINCFG_PULLEN_Pos)
#define PORT_PINCFG_DRVSTR_Pos      6            /**< \brief (PORT_PINCFG) Output Driver Strength Selection */
#define PORT_PINCFG_DRVSTR          (_U(0x1) << PORT_PINCFG_DRVSTR_Pos)
#define PORT_PINCFG_MASK            _U(0x47)     /**< \brief (PORT_PINCFG) MASK Register */

/** \brief PortGroup hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))

typedef struct 
{
	__IO uint32_t	DIR;			/**< \brief Offset: 0x00 (R/W 32) Data Direction */
	__IO uint32_t	DIRCLR;			/**< \brief Offset: 0x04 (R/W 32) Data Direction Clear */
	__IO uint32_t	DIRSET;			/**< \brief Offset: 0x08 (R/W 32) Data Direction Set */
	__IO uint32_t	DIRTGL;			/**< \brief Offset: 0x0C (R/W 32) Data Direction Toggle */
	__IO uint32_t	OUT;			/**< \brief Offset: 0x10 (R/W 32) Data Output Value */
	__IO uint32_t	OUTCLR;			/**< \brief Offset: 0x14 (R/W 32) Data Output Value Clear */
	__IO uint32_t	OUTSET;			/**< \brief Offset: 0x18 (R/W 32) Data Output Value Set */
	__IO uint32_t	OUTTGL;			/**< \brief Offset: 0x1C (R/W 32) Data Output Value Toggle */
	__I  uint32_t	IN;				/**< \brief Offset: 0x20 (R/  32) Data Input Value */
	__IO uint32_t	CTRL;			/**< \brief Offset: 0x24 (R/W 32) Control */
	__O  uint32_t	WRCONFIG;		/**< \brief Offset: 0x28 ( /W 32) Write Configuration */
		 RoReg8		Reserved1[4];
	__IO uint8_t	PMUX[16];		/**< \brief Offset: 0x30 (R/W  8) Peripheral Multiplexing n */
	__IO uint8_t	PINCFG[32];		/**< \brief Offset: 0x40 (R/W  8) Pin Configuration n */
		 RoReg8		Reserved2[32];
} Port;

#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAMD20_PORT_COMPONENT_ */
