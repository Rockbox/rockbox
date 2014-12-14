#ifndef ASIC_H
#define ASIC_H

#include <stdint.h>

typedef struct asic asic_t;

#include <z80e/core/cpu.h>
#include <z80e/log/log.h>
#include <z80e/ti/memory.h>
#include <z80e/ti/ti.h>
#include <z80e/runloop/runloop.h>
#include <z80e/debugger/hooks.h>
#include <z80e/debugger/debugger.h>
#include <z80e/ti/hardware/interrupts.h>

typedef enum {
	BATTERIES_REMOVED,
	BATTERIES_LOW,
	BATTERIES_GOOD
} battery_state;


typedef void (*timer_tick)(asic_t *, void *);
typedef struct z80_hardware_timers z80_hardware_timers_t;
typedef struct z80_hardware_timer z80_hardware_timer_t;

enum {
	TIMER_IN_USE = (1 << 0),
	TIMER_ONE_SHOT = (1 << 1)
};

struct z80_hardware_timer {
	int cycles_until_tick;

	int flags;
	double frequency;
	timer_tick on_tick;
	void *data;
};

struct z80_hardware_timers {
	int max_timers;
	z80_hardware_timer_t *timers;
};

struct asic {
	int stopped;
	ti_device_type device;
	battery_state battery;
	int battery_remove_check;
	int clock_rate;

	z80cpu_t* cpu;
	runloop_state_t *runloop;
	ti_mmu_t* mmu;
	ti_interrupts_t *interrupts;
	z80_hardware_timers_t *timers;
	hook_info_t *hook;
	log_t *log;
	debugger_t *debugger;
};

asic_t* asic_init(ti_device_type, log_t *);
void asic_free(asic_t*);

int asic_set_clock_rate(asic_t *, int);

int asic_add_timer(asic_t *, int, double, timer_tick, void *);
void asic_remove_timer(asic_t *, int);

#endif
