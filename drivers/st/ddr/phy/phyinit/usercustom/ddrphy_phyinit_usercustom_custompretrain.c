/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <common/debug.h>
#include <lib/mmio.h>

#include <ddrphy_phyinit_usercustom.h>
#include <ddrphy_phyinit.h>

#include <platform_def.h>

/* DDRDBG registers */
#define DDRDBG_DDR34_AC_SWIZZLE_ADD3_0		U(0x100)

/*
 * This function is called before training firmware is executed. Any
 * register override in this function might affect the firmware training
 * results.
 *
 * This function is executed before firmware execution loop. Thus this function
 * should be used only for the following:
 *
 *  - Override PHY register values written by
 *  ddrphy_phyinit_c_initphyconfig.  An example use case is when this
 *  function does not perform the exact programing desired by the user.
 *  - Write custom PHY registers that need to take effect before training
 *  firmware execution.
 *
 * User shall use mmio_write_16 to write PHY registers in order for the register
 * to be tracked by PhyInit for retention restore.
 *
 * To override settings in the message block, users can assign values to the
 * fields in the message block data structure directly
 *
 * \ref examples/simple/ddrphy_phyinit_usercustom_custompretrain.c example of this function
 *
 * @return Void
 */
void ddrphy_phyinit_usercustom_custompretrain(void)
{
	int byte __unused;
	int i = 0;
	int j;
	uintptr_t base;

	VERBOSE("%s Start\n", __func__);

#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
	base = (uintptr_t)(DDRPHYC_BASE + 4 * (TMASTER | CSR_HWTSWIZZLEHWTADDRESS0_ADDR));

	for (i = 0; i < NB_HWT_SWIZZLE; i++) {
		mmio_write_16(base + i * sizeof(uint32_t),
			      (uint16_t)userinputswizzle.swizzle[i]);
	}

	base = (uintptr_t)(stm32_ddrdbg_get_base() + DDRDBG_DDR34_AC_SWIZZLE_ADD3_0);

	for (j = 0; j < NB_AC_SWIZZLE; j++, i++) {
		mmio_write_32(base + j * sizeof(uint32_t), userinputswizzle.swizzle[i]);
	}
#else
	for (byte = 0; byte < userinputbasic.numdbyte; byte++) {
		base = (uintptr_t)(DDRPHYC_BASE + 4 * ((byte << 12) | TDBYTE | CSR_DQ0LNSEL_ADDR));

		for (j = 0; j < NB_DQLNSEL_SWIZZLE_PER_BYTE; j++, i++) {
			mmio_write_16(base + j * sizeof(uint32_t),
				      (uint16_t)userinputswizzle.swizzle[i]);
		}
	}

	base = (uintptr_t)(DDRPHYC_BASE + 4 * (TMASTER | CSR_MAPCAA0TODFI_ADDR));

	for (j = 0; j < NB_MAPCAATODFI_SWIZZLE; j++, i++) {
		mmio_write_16(base + j * sizeof(uint32_t),
			      (uint16_t)userinputswizzle.swizzle[i]);
	}

	base = (uintptr_t)(DDRPHYC_BASE + 4 * (TMASTER | CSR_MAPCAB0TODFI_ADDR));

	for (j = 0; j < NB_MAPCABTODFI_SWIZZLE; j++, i++) {
		mmio_write_16(base + j * sizeof(uint32_t),
			      (uint16_t)userinputswizzle.swizzle[i]);
	}
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */

	VERBOSE("%s End\n", __func__);
}
