#include "ti/hardware/keyboard.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "ti/asic.h"
#include "core/cpu.h"

typedef struct {
	uint8_t group_mask;
	uint8_t groups[8];
} keyboard_state_t;

/* Key codes in z80e are group << 4 | bit. That is, 0x14 is bit 4 of group 1. */
void depress_key(void *keyboard, uint8_t keycode) {
	keyboard_state_t *state = keyboard;
	uint8_t group = keycode >> 4;
	uint8_t mask = 1 << (keycode & 0xF);
	state->groups[group] &= ~mask;
}

void release_key(void *keyboard, uint8_t keycode) {
	keyboard_state_t *state = keyboard;
	uint8_t group = keycode >> 4;
	uint8_t mask = 1 << (keycode & 0xF);
	state->groups[group] |= mask;
}

uint8_t read_keyboard(void *_state) {
	keyboard_state_t *state = (keyboard_state_t*)_state;
	uint8_t mask = state->group_mask;

	uint8_t value = 0;
	int i;
	for (i = 7; i >= 0; i--) {
		if (mask & 0x80) {
			value |= ~state->groups[i];
		}
		mask <<= 1;
	}
	return ~value;
}

void write_keyboard(void *_state, uint8_t value) {
	keyboard_state_t *state = (keyboard_state_t*)_state;
	if (value == 0xFF) {
		state->group_mask = 0;
		return;
	}
	state->group_mask |= ~value;
}

z80iodevice_t init_keyboard() {
	keyboard_state_t *state = calloc(1, sizeof(keyboard_state_t));
	int i;
	for (i = 0; i < 8; i++) {
		state->groups[i] = 0xFF;
	}
	state->group_mask = 0;
	z80iodevice_t device = { state, read_keyboard, write_keyboard };
	return device;
}

void free_keyboard(void *keyboard) {
	free(keyboard);
}
