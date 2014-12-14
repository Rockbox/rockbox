#include "ti/hardware/speed.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "ti/asic.h"
#include "core/cpu.h"
#include "ti/hardware/interrupts.h"

uint8_t read_speed(void *_asic) {
	asic_t *asic = (asic_t*)_asic;
	if (asic->clock_rate == 6000000) {
		return 0;
	} else if (asic->clock_rate == 15000000) {
		return 1;
	}
	// TODO: set overclock
	return 0;
}

void write_speed(void *_asic, uint8_t value) {
	asic_t *asic = (asic_t*)_asic;
	if (value == 0) {
		asic_set_clock_rate(asic, 6000000);
	} else if (value == 1) {
		asic_set_clock_rate(asic, 15000000);
	}
	// TODO: set overclock
}

z80iodevice_t init_speed(asic_t *asic) {
	z80iodevice_t device = { asic, read_speed, write_speed };
	return device;
}
