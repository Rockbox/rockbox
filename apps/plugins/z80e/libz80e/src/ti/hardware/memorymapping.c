#include "ti/asic.h"
#include "log/log.h"
#include "ti/memory.h"
#include "ti/hardware/memorymapping.h"
#include "ti/hardware/interrupts.h"

#include <stdlib.h>
#include <string.h>

void reload_mapping(memory_mapping_state_t *state) {
	ti_mmu_bank_state_t *banks = state->asic->mmu->banks;

	banks[0].page = 0;
	banks[0].flash = 1;

	switch (state->map_mode) {
	case 0:
		banks[1].page = state->bank_a_page;
		banks[1].flash = state->bank_a_flash;
		banks[2].page = state->bank_b_page;
		banks[2].flash = state->bank_b_flash;
		if (state->asic->device == TI83p) {
			banks[3].page = 0;
			banks[3].flash = 0;
		} else {
			banks[3].page = state->ram_bank_page;
			banks[3].flash = 0;
		}
		break;
	case 1:
		banks[1].page = state->bank_a_page & 0xFE;
		banks[1].flash = state->bank_a_flash;
		if (state->asic->device == TI83p) {
			banks[2].page = state->bank_a_page;
			banks[2].flash = state->bank_a_flash;
		} else {
			banks[2].page = state->bank_a_page | 1;
			banks[2].flash = state->bank_a_flash;
		}
		banks[3].page = state->bank_b_page;
		banks[3].flash = state->bank_b_flash;
		break;
	}

	int i;
	for (i = 0; i < 4; i++) {
		if (banks[i].flash && banks[i].page > state->asic->mmu->settings.flash_pages) {
			log_message(state->asic->log, L_ERROR, "memorymapping", "ERROR: Flash page 0x%02X doesn't exist! (at 0x%04X)", banks[i].page, state->asic->cpu->registers.PC);
			banks[i].page &= state->asic->mmu->settings.flash_pages;
		} else if (!banks[i].flash && banks[i].page > state->asic->mmu->settings.ram_pages) {
			log_message(state->asic->log, L_ERROR, "memorymapping", "ERROR: RAM page 0x%02X doesn't exist! (at 0x%04X)", banks[i].page, state->asic->cpu->registers.PC);
			banks[i].page &= state->asic->mmu->settings.ram_pages;
		}
	}
}

uint8_t read_device_status_port(void *device) {
	memory_mapping_state_t *state = device;

	return read_interrupting_device(state->asic->interrupts);
}

void write_device_status_port(void *device, uint8_t data) {
	memory_mapping_state_t *state = device;

	state->map_mode = data & 1;
	log_message(state->asic->log, L_DEBUG, "memorymapping", "Set mapping mode to %d (at 0x%04X)", state->map_mode, state->asic->cpu->registers.PC);
	reload_mapping(state);

	write_timer_speed(state->asic->interrupts, data);
}

uint8_t read_ram_paging_port(void *device) {
	memory_mapping_state_t *state = device;

	return state->ram_bank_page;
}

void write_ram_paging_port(void *device, uint8_t data) {
	memory_mapping_state_t *state = device;

	state->ram_bank_page = data & 0x7; // 0b111
	log_message(state->asic->log, L_DEBUG, "memorymapping", "Set ram banking page to %d (at 0x%04X)", state->ram_bank_page, state->asic->cpu->registers.PC);
	reload_mapping(state);
}

uint8_t read_bank_a_paging_port(void *device) {
	memory_mapping_state_t *state = device;

	uint8_t return_value = state->bank_a_page;
	if (!state->bank_a_flash) {
		if (state->asic->device == TI83p) {
			return_value |= 1 << 6;
		} else {
			return_value |= 1 << 7;
		}
	}

	return return_value;
}

void write_bank_a_paging_port(void *device, uint8_t data) {
	memory_mapping_state_t *state = device;

	int is_flash = 0;

	if (state->asic->device == TI83p) {
		is_flash = (data & (1 << 6)) == 0;
		data &= 0x1F; // 0b11111
	} else {
		is_flash = (data & (1 << 7)) == 0;
		data &= 0x7F; // 0b111111
	}

	state->bank_a_flash = is_flash;
	state->bank_a_page = data;

	log_message(state->asic->log, L_DEBUG, "memorymapping", "Set bank A page to %c:%02X (at 0x%04X)", state->bank_a_flash ? 'F' : 'R',  state->bank_a_page, state->asic->cpu->registers.PC);
	reload_mapping(state);
}

uint8_t read_bank_b_paging_port(void *device) {
	memory_mapping_state_t *state = device;

	uint8_t return_value = state->bank_b_page;
	if (!state->bank_b_flash) {
		if (state->asic->device == TI83p) {
			return_value |= 1 << 6;
		} else {
			return_value |= 1 << 7;
		}
	}

	return return_value;
}

void write_bank_b_paging_port(void *device, uint8_t data) {
	memory_mapping_state_t *state = device;

	int is_flash = 0;

	if (state->asic->device == TI83p) {
		is_flash = (data & (1 << 6)) == 0;
		data &= 0x1F; // 0b11111
	} else {
		is_flash = (data & (1 << 7)) == 0;
		data &= 0x7F; // 0b111111
	}

	state->bank_b_flash = is_flash;
	state->bank_b_page = data;

	log_message(state->asic->log, L_DEBUG, "memorymapping", "Set bank B page to %c:%02X (at 0x%04X)", state->bank_b_flash ? 'F' : 'R',  state->bank_b_page, state->asic->cpu->registers.PC);
	reload_mapping(state);
}

void init_mapping_ports(asic_t *asic) {
	memory_mapping_state_t *state = malloc(sizeof(memory_mapping_state_t));

	memset(state, 0, sizeof(memory_mapping_state_t));
	state->asic = asic;

	state->bank_a_flash = 1;
	state->bank_b_flash = 1; // horrible, isn't it?

	z80iodevice_t device_status_port = { state, read_device_status_port, write_device_status_port };
	z80iodevice_t ram_paging_port = { state, read_ram_paging_port, write_ram_paging_port };
	z80iodevice_t bank_a_paging_port = { state, read_bank_a_paging_port, write_bank_a_paging_port };
	z80iodevice_t bank_b_paging_port = { state, read_bank_b_paging_port, write_bank_b_paging_port };

	asic->cpu->devices[0x04] = device_status_port;

	if (asic->device != TI83p) {
		asic->cpu->devices[0x05] = ram_paging_port;
	}

	asic->cpu->devices[0x06] = bank_a_paging_port;
	asic->cpu->devices[0x07] = bank_b_paging_port;

	reload_mapping(state);
}

void free_mapping_ports(asic_t *asic) {
	free(asic->cpu->devices[0x06].device);
}
