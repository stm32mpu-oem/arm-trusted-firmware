/*
 * Copyright (C) 2021-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP1_FIP_DEF_H
#define STM32MP1_FIP_DEF_H

#define STM32MP_DDR_S_SIZE		U(0x02000000)	/* 32 MB */

#if STM32MP_SSP
#if STM32MP13
#define STM32MP_BL2_DTB_BASE		STM32MP_SEC_SYSRAM_BASE
#endif
#if STM32MP15
#define STM32MP_BL2_DTB_BASE		(STM32MP_SYSRAM_BASE + \
					 STM32MP_HEADER_RESERVED_SIZE)
#endif

#define STM32MP_BL2_DTB_SIZE		U(0x00005000)	/* 20 KB for DTB */

#define STM32MP_BL2_RO_SIZE		U(0x0000E000)	/* 56 Ko for BL2 */

#define STM32MP_BL2_RO_BASE		STM32MP_BL2_DTB_BASE + \
					STM32MP_BL2_DTB_SIZE

#define STM32MP_BL2_RW_BASE		(STM32MP_BL2_RO_BASE + \
					 STM32MP_BL2_RO_SIZE)

#define STM32MP_BL2_RW_SIZE		(STM32MP_SYSRAM_BASE + \
					 STM32MP_SYSRAM_SIZE - \
					 STM32MP_BL2_RW_BASE)
#else /* STM32MP_SSP */
#if TRUSTED_BOARD_BOOT && !STM32MP_USE_EXTERNAL_HEAP
#if STM32MP15
#define STM32MP_BL2_RO_SIZE		U(0x00014000)	/* 80 KB */
#define STM32MP_BL2_SIZE		U(0x0001B000)	/* 108 KB for BL2 */
#endif /* STM32MP15 */
#else /* TRUSTED_BOARD_BOOT && !STM32MP_USE_EXTERNAL_HEAP */
#if STM32MP13
#if BL2_IN_XIP_MEM
#define STM32MP_BL2_RO_SIZE		U(0x00015000)	/* 84 KB */
#define STM32MP_BL2_SIZE		U(0x00017000)	/* 92 KB for BL2 */
#else
/* STM32MP_BL2_RO_SIZE not used if !BL2_IN_XIP_MEM */
#define STM32MP_BL2_SIZE		U(0x0001B000)	/* 108KB for BL2 */
					/* with 20KB for DTB, SYSRAM is full */
#endif
#endif /* STM32MP13 */
#if STM32MP15
#define STM32MP_BL2_RO_SIZE		U(0x00011000)	/* 68 KB */
#define STM32MP_BL2_SIZE		U(0x00016000)	/* 88 KB for BL2 */
#endif /* STM32MP15 */
#endif /* TRUSTED_BOARD_BOOT && !STM32MP_USE_EXTERNAL_HEAP */

#if STM32MP13
#if TRUSTED_BOARD_BOOT
#define STM32MP_BL2_DTB_SIZE		U(0x00005000)	/* 20 KB for DTB */
#else /* TRUSTED_BOARD_BOOT */
#define STM32MP_BL2_DTB_SIZE		U(0x00004000)	/* 16 KB for DTB */
#endif /* TRUSTED_BOARD_BOOT */
#endif /* STM32MP13 */
#if STM32MP15
#define STM32MP_BL2_DTB_SIZE		U(0x00007000)	/* 28 KB for DTB */
#endif /* STM32MP15 */
#define STM32MP_BL32_SIZE		U(0x0001B000)	/* 108 KB for BL32 */
#define STM32MP_BL32_DTB_SIZE		U(0x00005000)	/* 20 KB for DTB */
#define STM32MP_FW_CONFIG_MAX_SIZE	PAGE_SIZE	/* 4 KB for FCONF DTB */
#define STM32MP_HW_CONFIG_MAX_SIZE	U(0x40000)	/* 256 KB for HW config DTB */

#if STM32MP13
#define STM32MP_BL2_BASE		(STM32MP_BL2_DTB_BASE + \
					 STM32MP_BL2_DTB_SIZE)
#endif /* STM32MP13 */
#if STM32MP15
#define STM32MP_BL2_BASE		(STM32MP_SEC_SYSRAM_BASE + \
					 STM32MP_SEC_SYSRAM_SIZE - \
					 STM32MP_BL2_SIZE)
#endif /* STM32MP15 */

#define STM32MP_BL2_RO_BASE		STM32MP_BL2_BASE

#define STM32MP_BL2_RW_BASE		(STM32MP_BL2_RO_BASE + \
					 STM32MP_BL2_RO_SIZE)

#if STM32MP13
#define STM32MP_BL2_RW_SIZE		(STM32MP_SYSRAM_BASE + \
					 STM32MP_SYSRAM_SIZE - \
					 STM32MP_BL2_RW_BASE)

#define STM32MP_BL2_DTB_BASE		STM32MP_SEC_SYSRAM_BASE
#endif /* STM32MP13 */
#if STM32MP15
#define STM32MP_BL2_RW_SIZE		(STM32MP_SEC_SYSRAM_BASE + \
					 STM32MP_SEC_SYSRAM_SIZE - \
					 STM32MP_BL2_RW_BASE)

#define STM32MP_BL2_DTB_BASE		(STM32MP_BL2_BASE - \
					 STM32MP_BL2_DTB_SIZE)
#endif /* STM32MP15 */

#define STM32MP_BL32_DTB_BASE		STM32MP_SYSRAM_BASE

#define STM32MP_BL32_BASE		(STM32MP_BL32_DTB_BASE + \
					 STM32MP_BL32_DTB_SIZE)
#endif /* STM32MP_SSP */


#if defined(IMAGE_BL2)
#define STM32MP_DTB_SIZE		STM32MP_BL2_DTB_SIZE
#define STM32MP_DTB_BASE		STM32MP_BL2_DTB_BASE
#endif
#if defined(IMAGE_BL32)
#define STM32MP_DTB_SIZE		STM32MP_BL32_DTB_SIZE
#define STM32MP_DTB_BASE		STM32MP_BL32_DTB_BASE
#endif

#if defined(AARCH32_SP_OPTEE) && STM32MP1_OPTEE_IN_SYSRAM
#define STM32MP_OPTEE_BASE		STM32MP_SEC_SYSRAM_BASE

#define STM32MP_OPTEE_SIZE		(STM32MP_BL2_DTB_BASE -  \
					 STM32MP_OPTEE_BASE)
#endif

#if STM32MP13
#define STM32MP_FW_CONFIG_BASE		SRAM3_BASE
#endif /* STM32MP13 */
#if STM32MP15
#define STM32MP_FW_CONFIG_BASE		(STM32MP_SYSRAM_BASE + \
					 STM32MP_SYSRAM_SIZE - \
					 PAGE_SIZE)
#endif /* STM32MP15 */
#define STM32MP_HW_CONFIG_BASE		(STM32MP_BL33_BASE + \
					STM32MP_BL33_MAX_SIZE)

/*
 * MAX_MMAP_REGIONS is usually:
 * BL stm32mp1_mmap size + mmap regions in *_plat_arch_setup
 */
#if defined(IMAGE_BL32)
#define MAX_MMAP_REGIONS		10
#endif

#endif /* STM32MP1_FIP_DEF_H */
