#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>

typedef struct ti_mmu ti_mmu_t;

#include <z80e/core/cpu.h>
#include <z80e/debugger/hooks.h>
#include <z80e/ti/ti.h>
#include <z80e/log/log.h>
#include <stdint.h>

typedef struct {
	uint16_t ram_pages;
	uint16_t flash_pages;
} ti_mmu_settings_t;

typedef struct {
	uint8_t page;
	int flash;
} ti_mmu_bank_state_t;

typedef struct {
	uint32_t address;
	uint32_t address_mask;
	uint8_t value;
} flash_write_t;

struct ti_mmu {
	ti_mmu_settings_t settings;
	ti_mmu_bank_state_t banks[4];
	uint8_t *ram;
	uint8_t *flash;
	int flash_unlocked;
	int flash_write_index;
	flash_write_t flash_writes[6];
	hook_info_t *hook;
	log_t *log;
};

ti_mmu_t *ti_mmu_init(ti_device_type, log_t *);
void ti_mmu_free(ti_mmu_t *mmu);
uint8_t ti_read_byte(void *memory, uint16_t address);
void ti_write_byte(void *memory, uint16_t address, uint8_t value);
void mmu_force_write(void *memory, uint16_t address, uint8_t value);

#endif
