/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <common/debug.h>
#include <lib/mmio.h>

#include <ddrphy_phyinit.h>

#include <platform_def.h>

/*
 * This function can be used to implement saving of PHY registers to be
 * restored on retention exit.
 *
 * The requirement of this function is to issue register reads and store the
 * value to be recovered on retention exit.  The following is an example
 * implementation and the user may implement alternate methods that suit their
 * specific SoC system needs.
 *
 * In this implementation PhyInit saves register values in an internal C array.
 * During retention exit it restores register values from the array.  The exact
 * list of registers to save and later restore can be seen in the output txt
 * file with an associated calls to ddrphy_phyinit_usercustom_io_read16().
 *
 * PhyInit provides a register interface and a tracking mechanism to minimize
 * the number registers needing restore. Refer to source code for
 * ddrphy_phyinit_reginterface() for detailed implementation of tracking
 * mechanism. Tracking is disabled from step D to Step H as these involve
 * loading, executing and checking the state of training firmware execution
 * which are not required to implement the retention exit sequence. The registers
 * specified in firmware training App Note representing training results are
 * also saved in addition to registers written by PhyInit during PHY
 * initialization.
 *
 * \returns \c void
 */
void ddrphy_phyinit_usercustom_saveretregs(void)
{
	VERBOSE("%s Start\n", __func__);

	/* In short the implementation of this function performs tasks: */

	/*
	 * --------------------------------------------------------------------------
	 * 1. Enable tracking of training firmware result registers\n
	 *    See firmware training App Note section "IO Retention" for reference
	 *    table registers that need to be saved.
	 *
	 *    \note  The tagged registers in this step are in
	 *    addition to what is automatically tagged during Steps C to I.
	 *
	 * --------------------------------------------------------------------------
	 */

	/* 95% of users should not require to change the code below */
	int pstate = 0;
	int anib;
	int byte;
	int nibble;
	int lane;
	int p_addr;
	int c_addr;
	int u_addr;
	int b_addr;
	int r_addr;

	ddrphy_phyinit_trackreg(TMASTER | CSR_PLLCTRL3_ADDR);

	/* Non-PState Dbyte Registers */
	for (byte = 0; byte < userinputbasic.numdbyte; byte++) {
		c_addr = byte << 12;
		for (lane = 0; lane <= R_MAX; lane++) {
			r_addr = lane << 8;
			ddrphy_phyinit_trackreg(TDBYTE | c_addr | r_addr | CSR_RXPBDLYTG0_ADDR);
			ddrphy_phyinit_trackreg(TDBYTE | c_addr | r_addr | CSR_RXPBDLYTG1_ADDR);
#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
			ddrphy_phyinit_trackreg(TDBYTE | c_addr | r_addr | CSR_RXPBDLYTG2_ADDR);
			ddrphy_phyinit_trackreg(TDBYTE | c_addr | r_addr | CSR_RXPBDLYTG3_ADDR);
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */
		} /* r_addr */

#if STM32MP_LPDDR4_TYPE
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_PPTCTLSTATIC_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_TRAININGINCDECDTSMEN_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_TSMBYTE0_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ0LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ1LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ2LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ3LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ4LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ5LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ6LNSEL_ADDR);
		ddrphy_phyinit_trackreg(TDBYTE | c_addr | CSR_DQ7LNSEL_ADDR);
#endif /* STM32MP_LPDDR4_TYPE */
	} /* c_addr */

	/* PState variable registers */
	for (pstate = 0; pstate < userinputbasic.numpstates; pstate++) {
		p_addr = pstate << 20;
		ddrphy_phyinit_trackreg(p_addr | TMASTER | CSR_VREFINGLOBAL_ADDR);

		/* Anig Registers */
		for (anib = 0; anib < userinputbasic.numanib; anib++) {
			c_addr = anib << 12;
			ddrphy_phyinit_trackreg(p_addr | TANIB | c_addr | CSR_ATXDLY_ADDR);
		}

		/* Dbyte Registers */
		for (byte = 0; byte < userinputbasic.numdbyte; byte++) {
			c_addr = byte << 12;
			ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | CSR_DFIMRL_ADDR);
			for (nibble = 0; nibble <= B_MAX; nibble++) {
				b_addr = nibble << 8;
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | b_addr |
							CSR_DQDQSRCVCNTRL_ADDR);
			}

			for (nibble = 0; nibble < 2; nibble++) {
				u_addr = nibble << 8;
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXENDLYTG0_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXENDLYTG1_ADDR);
#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXENDLYTG2_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXENDLYTG3_ADDR);
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_TXDQSDLYTG0_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_TXDQSDLYTG1_ADDR);
#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_TXDQSDLYTG2_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_TXDQSDLYTG3_ADDR);
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXCLKDLYTG0_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXCLKDLYTG1_ADDR);
#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXCLKDLYTG2_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | u_addr |
							CSR_RXCLKDLYTG3_ADDR);
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */
			} /* nibble */

			for (lane = R_MIN; lane <= R_MAX; lane++) {
				r_addr = lane << 8;
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | r_addr |
							CSR_TXDQDLYTG0_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | r_addr |
							CSR_TXDQDLYTG1_ADDR);
#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | r_addr |
							CSR_TXDQDLYTG2_ADDR);
				ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr | r_addr |
							CSR_TXDQDLYTG3_ADDR);
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */
			} /* r_addr */

#if STM32MP_LPDDR4_TYPE
			ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr |
						CSR_PPTDQSCNTINVTRNTG0_ADDR);
			ddrphy_phyinit_trackreg(p_addr | TDBYTE | c_addr |
						CSR_PPTDQSCNTINVTRNTG1_ADDR);
#endif /* STM32MP_LPDDR4_TYPE */
		} /* c_addr */

		/* PIE Registers */
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR1_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR2_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR3_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR4_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR5_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR6_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR7_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TINITENG | CSR_SEQ0BGPR8_ADDR);

		/* Master Registers */
		ddrphy_phyinit_trackreg(p_addr | TMASTER | CSR_DLLGAINCTL_ADDR);
		ddrphy_phyinit_trackreg(p_addr | TMASTER | CSR_DLLLOCKPARAM_ADDR);
#if STM32MP_LPDDR4_TYPE
		ddrphy_phyinit_trackreg(p_addr | TMASTER | CSR_HWTMRL_ADDR);
#endif /* STM32MP_LPDDR4_TYPE */
	} /* p_addr */

	/* Master Registers */
	ddrphy_phyinit_trackreg(TMASTER | CSR_HWTCAMODE_ADDR);
#if STM32MP_LPDDR4_TYPE
	ddrphy_phyinit_trackreg(TMASTER | CSR_HWTLPCSENA_ADDR);
	ddrphy_phyinit_trackreg(TMASTER | CSR_HWTLPCSENB_ADDR);

	/* ACSM registers */
	ddrphy_phyinit_trackreg(TACSM | CSR_ACSMCTRL13_ADDR);
	ddrphy_phyinit_trackreg(TACSM | CSR_ACSMCTRL23_ADDR);
#endif /* STM32MP_LPDDR4_TYPE */

	/*
	 * --------------------------------------------------------------------------
	 * 2. Track any additional registers\n
	 *    Register writes made using the any of the PhyInit functions are
	 *    automatically tracked using the call to ddrphy_phyinit_trackreg() in
	 *    mmio_write_16(). Use this section to track additional registers.
	 * --------------------------------------------------------------------------
	 */

	/*
	 * Example:
	 * ddrphy_phyinit_trackreg(<addr>);
	 */

	/*
	 * --------------------------------------------------------------------------
	 * 3. Prepare for register reads\n
	 *    - Write the MicroContMuxSel CSR to 0x0 to allow access to the internal CSRs
	 *    - Write the UcclkHclkEnables CSR to 0x3 to enable all the clocks so the reads
	 *      can complete.
	 * --------------------------------------------------------------------------
	 */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICROCONTMUXSEL_ADDR)), 0x0U);
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TDRTUB | CSR_UCCLKHCLKENABLES_ADDR)), 0x3U);

	/*
	 * --------------------------------------------------------------------------
	 * / 4. Read and save all the registers
	 * /    - The list of registers differ depending on protocol and 1D training.
	 * --------------------------------------------------------------------------
	 */

	ddrphy_phyinit_reginterface(saveregs, 0, 0);

	/*
	 * --------------------------------------------------------------------------
	 * 5. Prepare for mission mode
	 *  - Write the UcclkHclkEnables CSR to disable the appropriate clocks after all reads done.
	 *  - Write the MicroContMuxSel CSR to 0x1 to isolate the internal CSRs during mission mode
	 * --------------------------------------------------------------------------
	 */

#if STM32MP_DDR4_TYPE
	/* Disabling Ucclk (PMU) and Hclk (training hardware) */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TDRTUB | CSR_UCCLKHCLKENABLES_ADDR)), 0x0U);
#elif STM32MP_LPDDR4_TYPE
	/* Disabling Ucclk (PMU) */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TDRTUB | CSR_UCCLKHCLKENABLES_ADDR)), 0x2U);
#endif /* STM32MP_LPDDR4_TYPE */

	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICROCONTMUXSEL_ADDR)), 0x1U);

	VERBOSE("%s End\n", __func__);

	return;
}