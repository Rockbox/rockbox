#ifndef DEBUGGER_HOOKS_H
#define DEBUGGER_HOOKS_H

#include <stdint.h>

typedef struct hook_info hook_info_t;

#include <z80e/ti/asic.h>
#include <z80e/core/registers.h>
#include <z80e/ti/hardware/t6a04.h>

hook_info_t *create_hook_set(asic_t *asic);

// Memory hooks

uint8_t hook_on_memory_read(hook_info_t *, uint16_t address, uint8_t value);
uint8_t hook_on_memory_write(hook_info_t *, uint16_t address, uint8_t value);

typedef uint8_t (*hook_memory_callback)(void *data, uint16_t address, uint8_t value);

void hook_remove_memory_read(hook_info_t *, int);
int hook_add_memory_read(hook_info_t *, uint16_t address_start, uint16_t address_end, void *data, hook_memory_callback);
void hook_remove_register_write(hook_info_t *, int);
int hook_add_memory_write(hook_info_t *, uint16_t address_start, uint16_t address_end, void *data, hook_memory_callback);

// Register hooks

uint16_t hook_on_register_read(hook_info_t *, registers flags, uint16_t value);
uint16_t hook_on_register_write(hook_info_t *, registers flags, uint16_t value);

typedef uint16_t (*hook_register_callback)(void *data, registers reg, uint16_t value);

void hook_remove_register_read(hook_info_t *, int);
int hook_add_register_read(hook_info_t *, registers flags, void *data, hook_register_callback);
void hook_remove_register_write(hook_info_t *, int);
int hook_add_register_write(hook_info_t *, registers flags, void *data, hook_register_callback);

// Execution hooks

void hook_on_before_execution(hook_info_t *, uint16_t address);
void hook_on_after_execution(hook_info_t *, uint16_t address);

typedef void (*hook_execution_callback)(void *data, uint16_t address);

void hook_remove_before_execution(hook_info_t *, int);
int hook_add_before_execution(hook_info_t *, void *data, hook_execution_callback);
void hook_remove_after_execution(hook_info_t *, int);
int hook_add_after_execution(hook_info_t *, void *data, hook_execution_callback);

// LCD hook

void hook_on_lcd_update(hook_info_t *, ti_bw_lcd_t *);

typedef void (*hook_lcd_update_callback)(void *data, ti_bw_lcd_t *lcd);

void hook_remove_lcd_update(hook_info_t *, int);
int hook_add_lcd_update(hook_info_t *, void *data, hook_lcd_update_callback);

#endif
