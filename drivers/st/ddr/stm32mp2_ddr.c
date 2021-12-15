/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/st/stm32mp_ddr.h>
#include <drivers/st/stm32mp2_ddr.h>
#include <drivers/st/stm32mp2_ddr_regs.h>
#include <lib/mmio.h>

#include <ddrphy_phyinit.h>

#include <platform_def.h>

#define SKIP_TRAINING_PARAM	false

#define DDRDBG_FRAC_PLL_LOCK	U(0x10)

#define DDRCTL_REG(x, y)					\
	{							\
		.name = #x,					\
		.offset = offsetof(struct stm32mp_ddrctl, x),	\
		.par_offset = offsetof(struct y, x)		\
	}

#define DDRPHY_REG(x, y)					\
	{							\
		.name = #x,					\
		.offset = offsetof(struct stm32mp_ddrphy, x),	\
		.par_offset = offsetof(struct y, x)		\
	}

/*
 * PARAMETERS: value get from device tree :
 *             size / order need to be aligned with binding
 *             modification NOT ALLOWED !!!
 */
#define DDRCTL_REG_REG_SIZE	48	/* st,ctl-reg */
#define DDRCTL_REG_TIMING_SIZE	20	/* st,ctl-timing */
#define DDRCTL_REG_MAP_SIZE	12	/* st,ctl-map */
#if STM32MP_DDR_DUAL_AXI_PORT
#define DDRCTL_REG_PERF_SIZE	21	/* st,ctl-perf */
#else
#define DDRCTL_REG_PERF_SIZE	14	/* st,ctl-perf */
#endif

#define DDRPHY_REG_REG_SIZE	0	/* st,phy-reg */
#define	DDRPHY_REG_TIMING_SIZE	0	/* st,phy-timing */

#define DDRCTL_REG_REG(x)	DDRCTL_REG(x, stm32mp2_ddrctrl_reg)
static const struct stm32mp_ddr_reg_desc ddr_reg[DDRCTL_REG_REG_SIZE] = {
	DDRCTL_REG_REG(mstr),
	DDRCTL_REG_REG(mrctrl0),
	DDRCTL_REG_REG(mrctrl1),
	DDRCTL_REG_REG(mrctrl2),
	DDRCTL_REG_REG(derateen),
	DDRCTL_REG_REG(derateint),
	DDRCTL_REG_REG(deratectl),
	DDRCTL_REG_REG(pwrctl),
	DDRCTL_REG_REG(pwrtmg),
	DDRCTL_REG_REG(hwlpctl),
	DDRCTL_REG_REG(rfshctl0),
	DDRCTL_REG_REG(rfshctl1),
	DDRCTL_REG_REG(rfshctl3),
	DDRCTL_REG_REG(crcparctl0),
	DDRCTL_REG_REG(crcparctl1),
	DDRCTL_REG_REG(init0),
	DDRCTL_REG_REG(init1),
	DDRCTL_REG_REG(init2),
	DDRCTL_REG_REG(init3),
	DDRCTL_REG_REG(init4),
	DDRCTL_REG_REG(init5),
	DDRCTL_REG_REG(init6),
	DDRCTL_REG_REG(init7),
	DDRCTL_REG_REG(dimmctl),
	DDRCTL_REG_REG(rankctl),
	DDRCTL_REG_REG(rankctl1),
	DDRCTL_REG_REG(zqctl0),
	DDRCTL_REG_REG(zqctl1),
	DDRCTL_REG_REG(zqctl2),
	DDRCTL_REG_REG(dfitmg0),
	DDRCTL_REG_REG(dfitmg1),
	DDRCTL_REG_REG(dfilpcfg0),
	DDRCTL_REG_REG(dfilpcfg1),
	DDRCTL_REG_REG(dfiupd0),
	DDRCTL_REG_REG(dfiupd1),
	DDRCTL_REG_REG(dfiupd2),
	DDRCTL_REG_REG(dfimisc),
	DDRCTL_REG_REG(dfitmg2),
	DDRCTL_REG_REG(dfitmg3),
	DDRCTL_REG_REG(dbictl),
	DDRCTL_REG_REG(dfiphymstr),
	DDRCTL_REG_REG(dbg0),
	DDRCTL_REG_REG(dbg1),
	DDRCTL_REG_REG(dbgcmd),
	DDRCTL_REG_REG(swctl),
	DDRCTL_REG_REG(swctlstatic),
	DDRCTL_REG_REG(poisoncfg),
	DDRCTL_REG_REG(pccfg),
};

#define DDRCTL_REG_TIMING(x)	DDRCTL_REG(x, stm32mp2_ddrctrl_timing)
static const struct stm32mp_ddr_reg_desc ddr_timing[DDRCTL_REG_TIMING_SIZE] = {
	DDRCTL_REG_TIMING(rfshtmg),
	DDRCTL_REG_TIMING(rfshtmg1),
	DDRCTL_REG_TIMING(dramtmg0),
	DDRCTL_REG_TIMING(dramtmg1),
	DDRCTL_REG_TIMING(dramtmg2),
	DDRCTL_REG_TIMING(dramtmg3),
	DDRCTL_REG_TIMING(dramtmg4),
	DDRCTL_REG_TIMING(dramtmg5),
	DDRCTL_REG_TIMING(dramtmg6),
	DDRCTL_REG_TIMING(dramtmg7),
	DDRCTL_REG_TIMING(dramtmg8),
	DDRCTL_REG_TIMING(dramtmg9),
	DDRCTL_REG_TIMING(dramtmg10),
	DDRCTL_REG_TIMING(dramtmg11),
	DDRCTL_REG_TIMING(dramtmg12),
	DDRCTL_REG_TIMING(dramtmg13),
	DDRCTL_REG_TIMING(dramtmg14),
	DDRCTL_REG_TIMING(dramtmg15),
	DDRCTL_REG_TIMING(odtcfg),
	DDRCTL_REG_TIMING(odtmap),
};

#define DDRCTL_REG_MAP(x)	DDRCTL_REG(x, stm32mp2_ddrctrl_map)
static const struct stm32mp_ddr_reg_desc ddr_map[DDRCTL_REG_MAP_SIZE] = {
	DDRCTL_REG_MAP(addrmap0),
	DDRCTL_REG_MAP(addrmap1),
	DDRCTL_REG_MAP(addrmap2),
	DDRCTL_REG_MAP(addrmap3),
	DDRCTL_REG_MAP(addrmap4),
	DDRCTL_REG_MAP(addrmap5),
	DDRCTL_REG_MAP(addrmap6),
	DDRCTL_REG_MAP(addrmap7),
	DDRCTL_REG_MAP(addrmap8),
	DDRCTL_REG_MAP(addrmap9),
	DDRCTL_REG_MAP(addrmap10),
	DDRCTL_REG_MAP(addrmap11),
};

#define DDRCTL_REG_PERF(x)	DDRCTL_REG(x, stm32mp2_ddrctrl_perf)
static const struct stm32mp_ddr_reg_desc ddr_perf[DDRCTL_REG_PERF_SIZE] = {
	DDRCTL_REG_PERF(sched),
	DDRCTL_REG_PERF(sched1),
	DDRCTL_REG_PERF(perfhpr1),
	DDRCTL_REG_PERF(perflpr1),
	DDRCTL_REG_PERF(perfwr1),
	DDRCTL_REG_PERF(sched3),
	DDRCTL_REG_PERF(sched4),
	DDRCTL_REG_PERF(pcfgr_0),
	DDRCTL_REG_PERF(pcfgw_0),
	DDRCTL_REG_PERF(pctrl_0),
	DDRCTL_REG_PERF(pcfgqos0_0),
	DDRCTL_REG_PERF(pcfgqos1_0),
	DDRCTL_REG_PERF(pcfgwqos0_0),
	DDRCTL_REG_PERF(pcfgwqos1_0),
#if STM32MP_DDR_DUAL_AXI_PORT
	DDRCTL_REG_PERF(pcfgr_1),
	DDRCTL_REG_PERF(pcfgw_1),
	DDRCTL_REG_PERF(pctrl_1),
	DDRCTL_REG_PERF(pcfgqos0_1),
	DDRCTL_REG_PERF(pcfgqos1_1),
	DDRCTL_REG_PERF(pcfgwqos0_1),
	DDRCTL_REG_PERF(pcfgwqos1_1),
#endif
};

static const struct stm32mp_ddr_reg_desc ddrphy_reg[DDRPHY_REG_REG_SIZE] = {};

static const struct stm32mp_ddr_reg_desc ddrphy_timing[DDRPHY_REG_TIMING_SIZE] = {};

/*
 * REGISTERS ARRAY: used to parse device tree and interactive mode
 */
static const struct stm32mp_ddr_reg_info ddr_registers[REG_TYPE_NB] __unused = {
	[REG_REG] = {
		.name = "static",
		.desc = ddr_reg,
		.size = DDRCTL_REG_REG_SIZE,
		.base = DDR_BASE
	},
	[REG_TIMING] = {
		.name = "timing",
		.desc = ddr_timing,
		.size = DDRCTL_REG_TIMING_SIZE,
		.base = DDR_BASE
	},
	[REG_PERF] = {
		.name = "perf",
		.desc = ddr_perf,
		.size = DDRCTL_REG_PERF_SIZE,
		.base = DDR_BASE
	},
	[REG_MAP] = {
		.name = "map",
		.desc = ddr_map,
		.size = DDRCTL_REG_MAP_SIZE,
		.base = DDR_BASE
	},
	[REGPHY_REG] = {
		.name = "static",
		.desc = ddrphy_reg,
		.size = DDRPHY_REG_REG_SIZE,
		.base = DDRPHY_BASE
	},
	[REGPHY_TIMING] = {
		.name = "timing",
		.desc = ddrphy_timing,
		.size = DDRPHY_REG_TIMING_SIZE,
		.base = DDRPHY_BASE
	},
};

static void ddr_reset(struct stm32mp_ddr_priv *priv)
{
	udelay(DDR_DELAY_1US);

	mmio_write_32(priv->rcc + RCC_DDRITFCFGR, RCC_DDRITFCFGR_DDRRST);
	mmio_write_32(priv->rcc + RCC_DDRPHYCAPBCFGR,
		      RCC_DDRPHYCAPBCFGR_DDRPHYCAPBEN | RCC_DDRPHYCAPBCFGR_DDRPHYCAPBLPEN);
	mmio_write_32(priv->rcc + RCC_DDRCAPBCFGR,
		      RCC_DDRCAPBCFGR_DDRCAPBEN | RCC_DDRCAPBCFGR_DDRCAPBLPEN);
	mmio_write_32(priv->rcc + RCC_DDRCFGR, RCC_DDRCFGR_DDRCFGEN | RCC_DDRCFGR_DDRCFGLPEN);

	udelay(DDR_DELAY_1US);

	mmio_write_32(priv->rcc + RCC_DDRITFCFGR, RCC_DDRITFCFGR_DDRRST);
	mmio_write_32(priv->rcc + RCC_DDRPHYCAPBCFGR, RCC_DDRPHYCAPBCFGR_DDRPHYCAPBRST);
	mmio_write_32(priv->rcc + RCC_DDRCAPBCFGR, RCC_DDRCAPBCFGR_DDRCAPBRST);
	mmio_write_32(priv->rcc + RCC_DDRCFGR, RCC_DDRCFGR_DDRCFGRST);

	udelay(DDR_DELAY_1US);

	mmio_write_32(priv->rcc + RCC_DDRITFCFGR, RCC_DDRITFCFGR_DDRRST);
	mmio_write_32(priv->rcc + RCC_DDRPHYCAPBCFGR,
		      RCC_DDRPHYCAPBCFGR_DDRPHYCAPBEN | RCC_DDRPHYCAPBCFGR_DDRPHYCAPBLPEN);
	mmio_write_32(priv->rcc + RCC_DDRCAPBCFGR,
		      RCC_DDRCAPBCFGR_DDRCAPBEN | RCC_DDRCAPBCFGR_DDRCAPBLPEN);
	mmio_write_32(priv->rcc + RCC_DDRCFGR, RCC_DDRCFGR_DDRCFGEN | RCC_DDRCFGR_DDRCFGLPEN);

	udelay(DDR_DELAY_1US);
}

static void ddr_sysconf_configuration(struct stm32mp_ddr_priv *priv)
{
	mmio_write_32(stm32_ddrdbg_get_base() + DDRDBG_LP_DISABLE,
		      DDRDBG_LP_DISABLE_LPI_XPI_DISABLE | DDRDBG_LP_DISABLE_LPI_DDRC_DISABLE);
	mmio_write_32(stm32_ddrdbg_get_base() + DDRDBG_BYPASS_PCLKEN, 0U);

	mmio_write_32(priv->rcc + RCC_DDRPHYCCFGR, RCC_DDRPHYCCFGR_DDRPHYCEN);
	mmio_write_32(priv->rcc + RCC_DDRCPCFGR, RCC_DDRCPCFGR_DDRCPEN | RCC_DDRCPCFGR_DDRCPLPEN);
	mmio_write_32(priv->rcc + RCC_DDRITFCFGR, RCC_DDRITFCFGR_DDRRST);

	udelay(DDR_DELAY_1US);
}

static void disable_refresh(struct stm32mp_ddrctl *ctl)
{
	mmio_setbits_32((uintptr_t)&ctl->rfshctl3, DDRCTRL_RFSHCTL3_DIS_AUTO_REFRESH);

	stm32mp_ddr_wait_refresh_update_done_ack(ctl);

	udelay(DDR_DELAY_1US);

	mmio_clrbits_32((uintptr_t)&ctl->pwrctl,
			DDRCTRL_PWRCTL_POWERDOWN_EN | DDRCTRL_PWRCTL_SELFREF_EN);

	udelay(DDR_DELAY_1US);

	stm32mp_ddr_start_sw_done(ctl);

	udelay(DDR_DELAY_1US);

	mmio_clrbits_32((uintptr_t)&ctl->dfimisc, DDRCTRL_DFIMISC_DFI_INIT_COMPLETE_EN);

	udelay(DDR_DELAY_1US);

	stm32mp_ddr_wait_sw_done_ack(ctl);
}

static void enable_refresh(struct stm32mp_ddrctl *ctl)
{
	mmio_clrbits_32((uintptr_t)&ctl->pwrctl, DDRCTRL_PWRCTL_SELFREF_SW);

	udelay(DDR_DELAY_1US);

	mmio_clrbits_32((uintptr_t)&ctl->rfshctl3, DDRCTRL_RFSHCTL3_DIS_AUTO_REFRESH);

	stm32mp_ddr_wait_refresh_update_done_ack(ctl);
}

static void wait_dfi_init_complete(struct stm32mp_ddrctl *ctl)
{
	uint64_t timeout;
	uint32_t dfistat;

	timeout = timeout_init_us(DDR_TIMEOUT_US_1S);
	do {
		dfistat = mmio_read_32((uintptr_t)&ctl->dfistat);
		VERBOSE("[0x%lx] dfistat = 0x%x ", (uintptr_t)&ctl->dfistat, dfistat);
		if (timeout_elapsed(timeout)) {
			panic();
		}
	} while ((dfistat & DDRCTRL_DFISTAT_DFI_INIT_COMPLETE) == 0U);

	VERBOSE("[0x%lx] dfistat = 0x%x\n", (uintptr_t)&ctl->dfistat, dfistat);
}

static void activate_controller(struct stm32mp_ddrctl *ctl)
{
	stm32mp_ddr_start_sw_done(ctl);

	mmio_setbits_32((uintptr_t)&ctl->dfimisc, DDRCTRL_DFIMISC_DFI_INIT_START);

	stm32mp_ddr_wait_sw_done_ack(ctl);

	wait_dfi_init_complete(ctl);

	udelay(DDR_DELAY_1US);

	stm32mp_ddr_start_sw_done(ctl);

	mmio_clrsetbits_32((uintptr_t)&ctl->dfimisc, DDRCTRL_DFIMISC_DFI_INIT_START,
			   DDRCTRL_DFIMISC_DFI_INIT_COMPLETE_EN);

	udelay(DDR_DELAY_1US);

	stm32mp_ddr_wait_sw_done_ack(ctl);
}

void stm32mp2_ddr_init(struct stm32mp_ddr_priv *priv,
		       struct stm32mp_ddr_config *config)
{
	int ret = -EINVAL;
	uint32_t ddr_retdis;

	if ((config->c_reg.mstr & DDRCTRL_MSTR_DDR3) != 0U) {
		ret = stm32mp_board_ddr_power_init(STM32MP_DDR3);
	} else if ((config->c_reg.mstr & DDRCTRL_MSTR_DDR4) != 0U) {
		ret = stm32mp_board_ddr_power_init(STM32MP_DDR4);
	} else if ((config->c_reg.mstr & DDRCTRL_MSTR_LPDDR4) != 0U) {
		ret = stm32mp_board_ddr_power_init(STM32MP_LPDDR4);
	} else {
		ERROR("DDR type not supported\n");
	}

	if (ret != 0) {
		panic();
	}

	VERBOSE("name = %s\n", config->info.name);
	VERBOSE("speed = %u kHz\n", config->info.speed);
	VERBOSE("size  = 0x%x\n", config->info.size);
	if (config->self_refresh) {
		VERBOSE("sel-refresh exit (zdata = 0x%x)\n", config->zdata);
	}

	/* Check DDR PHY pads retention */
	ddr_retdis = mmio_read_32((uint32_t)(priv->pwr) + PWR_CR11) &
		     PWR_CR11_DDRRETDIS;
	if (config->self_refresh) {
		if (ddr_retdis == PWR_CR11_DDRRETDIS) {
			VERBOSE("self-refresh aborted: no retention\n");
			config->self_refresh = false;
		}
	}

	if (!config->self_refresh) {
		VERBOSE("disable DDR PHY retention\n");
		mmio_setbits_32((uint32_t)(priv->pwr) + PWR_CR11, PWR_CR11_DDRRETDIS);
	}

	ddr_reset(priv);

	ddr_sysconf_configuration(priv);

	stm32mp_ddr_set_reg(priv, REG_REG, &config->c_reg, ddr_registers);
	stm32mp_ddr_set_reg(priv, REG_TIMING, &config->c_timing, ddr_registers);
	stm32mp_ddr_set_reg(priv, REG_MAP, &config->c_map, ddr_registers);
	stm32mp_ddr_set_reg(priv, REG_PERF, &config->c_perf, ddr_registers);

	/*  DDR core and PHY reset de-assert */
	mmio_clrbits_32(priv->rcc + RCC_DDRITFCFGR, RCC_DDRITFCFGR_DDRRST);

	disable_refresh(priv->ctl);

	ret = ddrphy_phyinit_sequence(SKIP_TRAINING_PARAM);
	if (ret != 0) {
		ERROR("DDR PHY init: Error %d\n", ret);
		panic();
	}

	activate_controller(priv->ctl);

	enable_refresh(priv->ctl);

	stm32mp_ddr_enable_axi_port(priv->ctl);
}