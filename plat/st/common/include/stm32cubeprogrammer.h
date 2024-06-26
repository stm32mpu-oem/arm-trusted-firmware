/*
 * Copyright (c) 2021, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32CUBEPROGRAMMER_H
#define STM32CUBEPROGRAMMER_H

#include <stdint.h>

#include <usb_dfu.h>

/* Phase definition */
#define PHASE_FLASHLAYOUT	0U
#define PHASE_DDR_FW		2U
#define PHASE_SSBL		3U
#define PHASE_CMD		0xF1U
#define PHASE_SSP		0xF3U
#define PHASE_RESET		0xFFU

/* Functions provided by plat */
uint8_t usb_dfu_get_phase(uint8_t alt);

int stm32cubeprog_usb_load(struct usb_handle *usb_core_handle,
			   uint8_t phase,
			   uintptr_t ssbl_base,
			   size_t ssbl_len);

int stm32cubeprog_uart_load(uintptr_t instance, uint8_t phase,
			   uintptr_t base, size_t len);

int stm32cubeprog_usb_ssp(struct usb_handle *usb_core_handle,
			  uintptr_t cert_base,
			  size_t cert_len,
			  uintptr_t ssp_base,
			  size_t ssp_len);

int stm32cubeprog_uart_ssp(uintptr_t instance,
			   uintptr_t cert_base,
			   size_t cert_len,
			   uintptr_t ssp_base,
			   size_t ssp_len);

#endif /* STM32CUBEPROGRAMMER_H */
