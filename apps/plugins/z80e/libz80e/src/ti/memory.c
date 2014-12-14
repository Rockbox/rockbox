#include "ti/memory.h"
#include "core/cpu.h"
#include "ti/ti.h"
#include "log/log.h"

#include "debugger/debugger.h"
#include "disassembler/disassemble.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ti_mmu_t* ti_mmu_init(ti_device_type device_type, log_t *log) {
	ti_mmu_t *mmu = malloc(sizeof(ti_mmu_t));
	switch (device_type) {
	case TI83p:
	case TI73:
		mmu->settings.ram_pages = 3;
		mmu->settings.flash_pages = 0x20;
		break;
	case TI84p:
		mmu->settings.ram_pages = 8;
		mmu->settings.flash_pages = 0x40;
		break;
	case TI83pSE:
	case TI84pSE:
		mmu->settings.ram_pages = 8;
		mmu->settings.flash_pages = 0x80;
		break;
	case TI84pCSE:
		mmu->settings.ram_pages = 3;
		mmu->settings.flash_pages = 0x100;
		break;
	}
	mmu->ram = malloc(mmu->settings.ram_pages * 0x4000);
	memset(mmu->ram, 0, mmu->settings.ram_pages * 0x4000);
	mmu->flash = malloc(mmu->settings.flash_pages * 0x4000);
	memset(mmu->flash, 0xFF, mmu->settings.flash_pages * 0x4000);
	mmu->flash_unlocked = 0;
	memset(mmu->flash_writes, 0, sizeof(flash_write_t) * 6);
	mmu->flash_write_index = 0;
	mmu->log = log;
	// Default bank mappings
	mmu->banks[0].page = 0; mmu->banks[0].flash = 1;
	mmu->banks[1].page = 0; mmu->banks[1].flash = 1;
	mmu->banks[2].page = 0; mmu->banks[2].flash = 1;
	mmu->banks[3].page = 0; mmu->banks[3].flash = 0;
	return mmu;
}

void ti_mmu_free(ti_mmu_t *mmu) {
	free(mmu->ram);
	free(mmu->flash);
	free(mmu);
}

uint8_t ti_read_byte(void *memory, uint16_t address) {
	ti_mmu_t *mmu = memory;
	ti_mmu_bank_state_t bank = mmu->banks[address / 0x4000];
	uint32_t mapped_address = address;
	mapped_address %= 0x4000;
	mapped_address += bank.page * 0x4000;
	uint8_t byte = 0;
	if (bank.flash) {
		byte = mmu->flash[mapped_address];
	} else {
		byte = mmu->ram[mapped_address];
	}
	byte = hook_on_memory_read(mmu->hook, address, byte);
	return byte;
}

struct flash_pattern {
	const flash_write_t pattern[6];
	void (*handler)(ti_mmu_t *memory, uint32_t address, uint8_t value);
};

void chip_erase(ti_mmu_t *mmu, uint32_t address, uint8_t value) {
	memset(mmu->flash, 0xFF, mmu->settings.flash_pages * 0x4000);
	log_message(mmu->log, L_WARN, "mmu", "Erased entire Flash chip - you probably didn't want to do that.");
}

struct flash_pattern patterns[] = {
	{
		.pattern = {
			{ .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA },
			{ .address = 0x555, .address_mask = 0xFFF, .value = 0x55 },
			{ .address = 0xAAA, .address_mask = 0xFFF, .value = 0xA0 },
		},
		.handler = NULL // Program byte TODO
	},
	{
		.pattern = {
			{ .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA },
			{ .address = 0x555, .address_mask = 0xFFF, .value = 0x55 },
			{ .address = 0xAAA, .address_mask = 0xFFF, .value = 0x80 },
			{ .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA },
			{ .address = 0x555, .address_mask = 0xFFF, .value = 0x55 },
			{ .address = 0xAAA, .address_mask = 0xFFF, .value = 0x10 },
		},
		.handler = chip_erase
	},
	// TODO: More patterns
};

void ti_write_byte(void *memory, uint16_t address, uint8_t value) {
	ti_mmu_t *mmu = memory;
	ti_mmu_bank_state_t bank = mmu->banks[address / 0x4000];
	uint32_t mapped_address = address;
	mapped_address %= 0x4000;
	mapped_address += bank.page * 0x4000;

	value = hook_on_memory_write(mmu->hook, address, value);

	if (!bank.flash)
		mmu->ram[mapped_address] = value;
	else {
		if (mmu->flash_unlocked) {
			flash_write_t *w = &mmu->flash_writes[mmu->flash_write_index++];
			w->address = address;
			w->value = value;
			// Check for Flash command patterns TODO
		}
	}
}

void mmu_force_write(void *memory, uint16_t address, uint8_t value) {
	ti_mmu_t *mmu = memory;
	ti_mmu_bank_state_t bank = mmu->banks[address / 0x4000];
	uint32_t mapped_address = address;
	mapped_address %= 0x4000;
	mapped_address += bank.page * 0x4000;

	value = hook_on_memory_write(mmu->hook, address, value);

	if (!bank.flash)
		mmu->ram[mapped_address] = value;
	else {
		mmu->flash[mapped_address] = value;
	}
}
