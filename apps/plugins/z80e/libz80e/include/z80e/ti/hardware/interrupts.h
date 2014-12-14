#ifndef TI_HARDWARE_INTERRUPTS_H
#define TI_HARDWARE_INTERRUPTS_H

typedef struct ti_interrupts ti_interrupts_t;

#include <z80e/core/cpu.h>
#include <z80e/ti/asic.h>

struct ti_interrupts {
	asic_t *asic;
	int first_timer_id;
	int second_timer_id;

	double first_timer_frequency;
	double second_timer_frequency;

	struct {
		int on_key : 1;
		int first_timer : 1;
		int second_timer : 1;
		int link_activity : 1;
		int first_crystal : 1;
		int second_crystal : 1;
		int third_crystal : 1;
	} interrupted;

	struct {
		int on_key : 1;
		int first_timer : 1;
		int second_timer : 1;
		int link_activity : 1;
		int first_crystal : 1;
		int second_crystal : 1;
		int third_crystal : 1;
	} enabled;
};

enum {
	INTERRUPT_ON_KEY = (1 << 0),
	INTERRUPT_FIRST_TIMER = (1 << 1),
	INTERRUPT_SECOND_TIMER = (1 << 2),
	INTERRUPT_LINK_ACTIVITY = (1 << 4),
	INTERRUPT_FIRST_CRYSTAL = (1 << 5),
	INTERRUPT_SECOND_CRYSTAL = (1 << 6),
	INTERRUPT_THIRD_CRYSTAL = (1 << 7),
};

z80iodevice_t init_interrupts(asic_t *, ti_interrupts_t **result);

void ti_interrupts_interrupt(ti_interrupts_t *, int);
void ti_interrupts_set_interrupt_enabled(ti_interrupts_t *, int, int);
void ti_interrupts_acknowledge_interrupt(ti_interrupts_t *, int);

uint8_t read_interrupt_mask(void *); // port 03
void write_interrupt_mask(void *, uint8_t); // port 03

uint8_t read_interrupting_device(void *); // port 04
void write_acknowledged_interrupts(void *, uint8_t); // port 02
void write_timer_speed(void *, uint8_t);

void set_first_timer_frequency(void *, double);
void set_second_timer_frequency(void *, double);
#endif
