#include "ti/hardware/status.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "ti/asic.h"
#include "core/cpu.h"
#include "ti/hardware/interrupts.h"

typedef struct {
	asic_t *asic;
} status_t;

uint8_t read_status(void *_status) {
	status_t *status = (status_t*)_status;
	uint8_t value = 0x00;
	if (status->asic->battery_remove_check) {
		if (status->asic->battery != BATTERIES_REMOVED) {
			value |= 0x01;
		}
	} else {
		if (status->asic->battery == BATTERIES_GOOD) {
			value |= 0x01;
		}
	}

	value |= status->asic->device != TI73 ? 0x02 : 0x00;

	if (status->asic->mmu->flash_unlocked) {
		value |= 0x04;
	}

	// Note: Bits 3-6 unimplemented on the TI-83+ and TI-73 (link assist)
	if (status->asic->device != TI73 && status->asic->device != TI83p) {
		value |= 0x80;
	}
	if (status->asic->device == TI84p || status->asic->device == TI84pSE || status->asic->device == TI84pCSE) {
		value |= 0x20;
	}
	return value;
}

void write_status(void *_status, uint8_t value) {
	status_t *status = (status_t*)_status;
	if (status->asic->device == TI83p || status->asic->device == TI73) {
		return;
	}

	write_acknowledged_interrupts(status->asic->interrupts, value);
}

z80iodevice_t init_status(asic_t *asic) {
	status_t *state = malloc(sizeof(status_t));
	state->asic = asic;
	z80iodevice_t device = { state, read_status, write_status };
	return device;
}

void free_status(z80iodevice_t status) {
	free(status.device);
}
