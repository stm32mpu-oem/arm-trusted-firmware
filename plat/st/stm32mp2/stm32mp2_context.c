/*
 * Copyright (c) 2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/clk.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/el3_runtime/cpu_data.h>

#if defined(SPD_opteed)
#include "../../../services/spd/opteed/opteed_private.h"
#endif

#include <platform_def.h>
#include <stm32mp2_context.h>

#define BACKUP_CTX_ADDR		STM32MP_BACKUP_RAM_BASE
#define BACKUP_CTX_CLK		CK_BUS_BKPSRAM
#define ENC_KEY_SIZE_IN_BYTES	RISAF_KEY_SIZE_IN_BYTES

/* Magic used to indicated valid = ' ' 'M' 'P' '2' */
#define CONTEXT_MAGIC			0x204D5032

struct backup_data_s {
	uint32_t magic;
	uint8_t enc_mkey[ENC_KEY_SIZE_IN_BYTES];
	psci_power_state_t standby_pwr_state;
	cpu_context_t saved_cpu_s_context[PLATFORM_CORE_COUNT];
	cpu_context_t saved_cpu_ns_context[PLATFORM_CORE_COUNT];
#if defined(SPD_opteed)
	uintptr_t optee_vector;
	optee_context_t opteed_sp_context[OPTEED_CORE_COUNT];
#endif
};

void stm32mp_pm_save_enc_mkey_in_context(uint8_t *data)
{
	struct backup_data_s *backup_data;

	backup_data = (struct backup_data_s *)BACKUP_CTX_ADDR;

	clk_enable(BACKUP_CTX_CLK);

	memcpy(backup_data->enc_mkey, data, ENC_KEY_SIZE_IN_BYTES);

	clk_disable(BACKUP_CTX_CLK);
}

void stm32mp_pm_get_enc_mkey_from_context(uint8_t *data)
{
	struct backup_data_s *backup_data;

	backup_data = (struct backup_data_s *)BACKUP_CTX_ADDR;

	clk_enable(BACKUP_CTX_CLK);

	memcpy(data, backup_data->enc_mkey, ENC_KEY_SIZE_IN_BYTES);

	clk_disable(BACKUP_CTX_CLK);
}

bool stm32_pm_context_is_valid(void)
{
	struct backup_data_s *backup_data;
	bool ret;

	backup_data = (struct backup_data_s *)BACKUP_CTX_ADDR;

	clk_enable(BACKUP_CTX_CLK);

	ret = (backup_data->magic == CONTEXT_MAGIC);
	clk_disable(BACKUP_CTX_CLK);

	return ret;
}

void stm32_pm_context_save(const psci_power_state_t *state)
{
	void *cpu_context;
	struct backup_data_s *backup_data;

	clk_enable(BACKUP_CTX_CLK);
	backup_data = (struct backup_data_s *)BACKUP_CTX_ADDR;

	backup_data->magic = CONTEXT_MAGIC;

	/* Retrieve non-secure CPU context struct address */
	cpu_context = cm_get_context(NON_SECURE);

	/* Save context in Backup SRAM */
	memcpy(&backup_data->saved_cpu_ns_context[0], cpu_context,
	       sizeof(cpu_context_t) * PLATFORM_CORE_COUNT);

	/* Retrieve secure CPU context struct address */
	cpu_context = cm_get_context(SECURE);

	/* Save context in Backup SRAM */
	memcpy(&backup_data->saved_cpu_s_context[0], cpu_context,
	       sizeof(cpu_context_t) * PLATFORM_CORE_COUNT);

#if defined(SPD_opteed)
	backup_data->optee_vector = (uintptr_t)optee_vector_table;

	memcpy(&backup_data->opteed_sp_context[0], opteed_sp_context,
	       sizeof(optee_context_t) * OPTEED_CORE_COUNT);
#endif

	/* Save PSCI state in Backup SRAM */
	memcpy(&backup_data->standby_pwr_state, state, sizeof(psci_power_state_t));

	clk_disable(BACKUP_CTX_CLK);
}

void stm32_pm_context_restore(void)
{
	void *cpu_context;
	struct backup_data_s *backup_data;

	clk_enable(BACKUP_CTX_CLK);
	backup_data = (struct backup_data_s *)BACKUP_CTX_ADDR;

	/* Retrieve non-secure CPU context struct address */
	cpu_context = cm_get_context(NON_SECURE);

	/* Restore data from Backup SRAM */
	memcpy(cpu_context, backup_data->saved_cpu_ns_context,
	       sizeof(cpu_context_t) * PLATFORM_CORE_COUNT);

	/* Retrieve non-secure CPU context struct address */
	cpu_context = cm_get_context(SECURE);

	/* Restore data from Backup SRAM */
	memcpy(cpu_context, backup_data->saved_cpu_s_context,
	       sizeof(cpu_context_t) * PLATFORM_CORE_COUNT);

	psci_set_target_local_pwr_states(PLAT_MAX_PWR_LVL,
					 &backup_data->standby_pwr_state);

#if defined(SPD_opteed)
	optee_vector_table = (optee_vectors_t *)backup_data->optee_vector;

	memcpy(opteed_sp_context, backup_data->opteed_sp_context,
	       sizeof(optee_context_t) * OPTEED_CORE_COUNT);

	opteed_restore();
#endif

	clk_disable(BACKUP_CTX_CLK);
}
