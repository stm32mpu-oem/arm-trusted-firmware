/*
 * Copyright (c) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <tools_share/firmware_image_package.h>

#include <stm32cubeprogrammer.h>
#include <usb_dfu.h>

/* Undefined download address */
#define UNDEFINED_DOWN_ADDR	0xFFFFFFFF

struct dfu_state {
	uint8_t phase;
	uintptr_t base;
	size_t len;
	uintptr_t address;
#if STM32MP_SSP
	uintptr_t cert_base;
	size_t cert_len;
#endif
	/* working buffer */
	uint8_t buffer[UCHAR_MAX];
};

static struct dfu_state dfu_state;

/* minimal size of Get Pḧase = offset for additionnl information */
#define	GET_PHASE_LEN	9

#define DFU_ERROR(...) \
	{ \
		ERROR(__VA_ARGS__); \
		if (dfu->phase != PHASE_RESET) { \
			snprintf((char *)&dfu->buffer[GET_PHASE_LEN], \
				 sizeof(dfu->buffer) - GET_PHASE_LEN, \
				 __VA_ARGS__); \
			dfu->phase = PHASE_RESET; \
			dfu->address = UNDEFINED_DOWN_ADDR; \
			dfu->len = 0; \
		} \
	}

#if !STM32MP_SSP
static bool is_valid_header(fip_toc_header_t *header)
{
	if ((header->name == TOC_HEADER_NAME) && (header->serial_number != 0U)) {
		return true;
	}

	return false;
}
#endif

static int dfu_callback_upload(uint8_t alt, uintptr_t *buffer, uint32_t *len,
			       void *user_data)
{
	int result = 0;
	uint32_t length = 0;
	struct dfu_state *dfu = (struct dfu_state *)user_data;

	switch (usb_dfu_get_phase(alt)) {
	case PHASE_CMD:
		/* Get Pḧase */
#if STM32MP_SSP && STM32MP15
		if (dfu->phase == PHASE_SSP) {
			dfu->buffer[0] = PHASE_FLASHLAYOUT;
		} else {
			dfu->buffer[0] = dfu->phase;
		}
#else
		dfu->buffer[0] = dfu->phase;
#endif
		dfu->buffer[1] = (uint8_t)(dfu->address);
		dfu->buffer[2] = (uint8_t)(dfu->address >> 8);
		dfu->buffer[3] = (uint8_t)(dfu->address >> 16);
		dfu->buffer[4] = (uint8_t)(dfu->address >> 24);
		dfu->buffer[5] = 0x00;
		dfu->buffer[6] = 0x00;
		dfu->buffer[7] = 0x00;
		dfu->buffer[8] = 0x00;
		length = GET_PHASE_LEN;
		if (dfu->phase == PHASE_FLASHLAYOUT &&
		    dfu->address == UNDEFINED_DOWN_ADDR) {
			INFO("Send detach request\n");
			dfu->buffer[length++] = 0x01;
		}
		if (dfu->phase == PHASE_RESET) {
			/* error information is added by DFU_ERROR macro */
			length += strnlen((char *)&dfu->buffer[GET_PHASE_LEN],
					  sizeof(dfu->buffer) - GET_PHASE_LEN)
				  - 1;
		}
		break;

#if STM32MP_SSP
	case PHASE_SSP:
#if STM32MP13
		dfu->buffer[0] = dfu->phase;
#endif
#if STM32MP15
		/* Fix phase to flashlayout phase */
		dfu->buffer[0] = PHASE_FLASHLAYOUT;
#endif
		dfu->buffer[1] = (uint8_t)(dfu_state.cert_base);
		dfu->buffer[2] = (uint8_t)(dfu_state.cert_base >> 8);
		dfu->buffer[3] = (uint8_t)(dfu_state.cert_base >> 16);
		dfu->buffer[4] = (uint8_t)(dfu_state.cert_base >> 24);
		dfu->buffer[5] = 0x00;
		dfu->buffer[6] = 0x00;
		dfu->buffer[7] = 0x00;
		dfu->buffer[8] = 0x00;
		length = 9U;

		if ((length + dfu_state.cert_len) <= sizeof(dfu->buffer)) {
			memcpy(&dfu->buffer[9], (uint8_t *)dfu_state.cert_base,
			       dfu_state.cert_len);
			length += dfu_state.cert_len;
		}

		break;
#endif
	default:
		DFU_ERROR("phase ID :%i, alternate %i for phase %i\n",
			  dfu->phase, alt, usb_dfu_get_phase(alt));
		result = -EIO;
		break;
	}

	if (result == 0) {
		*len = length;
		*buffer = (uintptr_t)dfu->buffer;
	}

	return result;
}

static int dfu_callback_download(uint8_t alt, uintptr_t *buffer, uint32_t *len,
				 void *user_data)
{
	struct dfu_state *dfu = (struct dfu_state *)user_data;

	if ((dfu->phase != usb_dfu_get_phase(alt)) ||
	    (dfu->address == UNDEFINED_DOWN_ADDR)) {
		DFU_ERROR("phase ID :%i, alternate %i, address %x\n",
			  dfu->phase, alt, (uint32_t)dfu->address);
		return -EIO;
	}

	VERBOSE("Download %d %lx %x\n", alt, dfu->address, *len);
	*buffer = dfu->address;
	dfu->address += *len;

	if (dfu->address - dfu->base > dfu->len) {
		return -EIO;
	}

	return 0;
}

static int dfu_callback_manifestation(uint8_t alt, void *user_data)
{
	struct dfu_state *dfu = (struct dfu_state *)user_data;

	if (dfu->phase != usb_dfu_get_phase(alt)) {
		ERROR("Manifestation phase ID :%i, alternate %i, address %lx\n",
		      dfu->phase, alt, dfu->address);
		return -EIO;
	}

	INFO("phase ID :%i, Manifestation %d at %lx\n",
	     dfu->phase, alt, dfu->address);

	switch (dfu->phase) {
#if STM32MP_SSP
	case PHASE_SSP:
		/* Configure End with request detach */
		dfu->phase = PHASE_FLASHLAYOUT;
		dfu->address = UNDEFINED_DOWN_ADDR;
		dfu->len = 0;
		break;
#else
	case PHASE_DDR_FW:
	case PHASE_SSBL:
		if (!is_valid_header((fip_toc_header_t *)dfu->base)) {
			DFU_ERROR("FIP Header check failed for phase %d\n", alt);
			return -EIO;
		}
		VERBOSE("FIP header looks OK.\n");

		/* Configure End with request detach */
		dfu->phase = PHASE_FLASHLAYOUT;
		dfu->address = UNDEFINED_DOWN_ADDR;
		dfu->len = 0;
		break;
#endif /* STM32MP_SSP */
	default:
		DFU_ERROR("Unknown phase\n");
	}

	return 0;
}

/* Open a connection to the USB device */
static const struct usb_dfu_media usb_dfu_fops = {
	.upload = dfu_callback_upload,
	.download = dfu_callback_download,
	.manifestation = dfu_callback_manifestation,
};

#if STM32MP_SSP
int stm32cubeprog_usb_ssp(struct usb_handle *usb_core_handle,
			  uintptr_t cert_base,
			  size_t cert_len,
			  uintptr_t ssp_base,
			  size_t ssp_len)
{
	int ret;

	usb_core_handle->user_data = (void *)&dfu_state;

	INFO("DFU USB START...\n");
	ret = usb_core_start(usb_core_handle);
	if (ret != USBD_OK) {
		return -EIO;
	}

	if (cert_base == UNDEFINED_DOWN_ADDR) {
		struct dfu_state *dfu = (struct dfu_state *)usb_core_handle->user_data;

		/* Send Provisioning message to programmer for reboot */
		DFU_ERROR("Provisioning\n");
	} else {
		dfu_state.phase = PHASE_SSP;
		dfu_state.address = ssp_base;
		dfu_state.base = ssp_base;
		dfu_state.len = ssp_len;
		dfu_state.cert_base = cert_base;
		dfu_state.cert_len = cert_len;
	}

	ret = usb_dfu_loop(usb_core_handle, &usb_dfu_fops);
	if (ret != USBD_OK) {
		return -EIO;
	}

	INFO("DFU USB STOP...\n");
	ret = usb_core_stop(usb_core_handle);
	if (ret != USBD_OK) {
		return -EIO;
	}

	return 0;
}
#endif

int stm32cubeprog_usb_load(struct usb_handle *usb_core_handle,
			   uint8_t phase,
			   uintptr_t base,
			   size_t len)
{
	int ret;

	usb_core_handle->user_data = (void *)&dfu_state;

	INFO("DFU USB START...\n");
	ret = usb_core_start(usb_core_handle);
	if (ret != USBD_OK) {
		return -EIO;
	}

	dfu_state.phase = phase;
	dfu_state.address = base;
	dfu_state.base = base;
	dfu_state.len = len;

	ret = usb_dfu_loop(usb_core_handle, &usb_dfu_fops);
	if (ret != USBD_OK) {
		return -EIO;
	}

	INFO("DFU USB STOP...\n");
	ret = usb_core_stop(usb_core_handle);
	if (ret != USBD_OK) {
		return -EIO;
	}

	return 0;
}
