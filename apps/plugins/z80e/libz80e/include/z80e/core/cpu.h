#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include <z80e/core/registers.h>
#include <z80e/log/log.h>

typedef struct z80cpu z80cpu_t;
typedef struct z80iodevice z80iodevice_t;

#include <z80e/debugger/hooks.h>

struct z80iodevice {
	void *device;
	uint8_t (*read_in)(void *);
	void (*write_out)(void *, uint8_t);
};

struct z80cpu {
	z80iodevice_t devices[0x100];
	z80registers_t registers;
	struct {
		uint8_t IFF1 : 1;
		uint8_t IFF2 : 1;
		uint8_t int_mode : 2;
		// Internal use:
		uint8_t IFF_wait : 1;
		uint8_t halted : 1;
	};
	uint8_t bus;
	uint16_t prefix;
	void *memory;
	uint8_t (*read_byte)(void *, uint16_t);
	void (*write_byte)(void *, uint16_t, uint8_t);
	int interrupt;
	hook_info_t *hook;
	log_t *log;
};


uint8_t cpu_read_register_byte(z80cpu_t *, registers);
uint16_t cpu_read_register_word(z80cpu_t *, registers);

uint8_t cpu_write_register_byte(z80cpu_t *, registers, uint8_t);
uint16_t cpu_write_register_word(z80cpu_t *, registers, uint16_t);

z80cpu_t* cpu_init(log_t *log);
void cpu_free(z80cpu_t *cpu);
uint8_t cpu_read_byte(z80cpu_t *cpu, uint16_t address);
void cpu_write_byte(z80cpu_t *cpu, uint16_t address, uint8_t value);
uint16_t cpu_read_word(z80cpu_t *cpu, uint16_t address);
void cpu_write_word(z80cpu_t *cpu, uint16_t address, uint16_t value);
int cpu_execute(z80cpu_t *cpu, int cycles);

#endif
