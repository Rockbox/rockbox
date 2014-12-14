#include "ti/hardware/interrupts.h"
#include "log/log.h"

#include <stdlib.h>

void ti_interrupts_check_state(ti_interrupts_t *interrupts) {
	if (!(
		interrupts->interrupted.on_key ||
		interrupts->interrupted.first_timer ||
		interrupts->interrupted.second_timer ||
		interrupts->interrupted.link_activity ||
		interrupts->interrupted.first_crystal ||
		interrupts->interrupted.second_crystal ||
		interrupts->interrupted.third_crystal)) {
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "disabling interrupt pin");
		interrupts->asic->cpu->interrupt = 0;
	}
}

void ti_interrupts_interrupt(ti_interrupts_t *interrupts, int flag) {
	int should_interrupt = 0;

	if (flag & INTERRUPT_ON_KEY && interrupts->enabled.on_key) {
		interrupts->interrupted.on_key = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "Triggered ON interrupt");
	}

	if (flag & INTERRUPT_FIRST_TIMER && interrupts->enabled.first_timer) {
		interrupts->interrupted.first_timer = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "Triggered first timer interrupt");
	}

	if (flag & INTERRUPT_SECOND_TIMER && interrupts->enabled.second_timer) {
		interrupts->interrupted.second_timer = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "Triggered second timer interrupt");
	}

	if (flag & INTERRUPT_LINK_ACTIVITY && interrupts->enabled.link_activity) {
		interrupts->interrupted.link_activity = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "Triggered link activity interrupt");
	}

	if (flag & INTERRUPT_FIRST_CRYSTAL && interrupts->enabled.first_crystal) {
		interrupts->interrupted.first_crystal = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "Triggered first crystal interrupt");
	}

	if (flag & INTERRUPT_SECOND_CRYSTAL && interrupts->enabled.second_crystal) {
		interrupts->interrupted.second_crystal = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "Triggered second crystal interrupt");
	}

	if (flag & INTERRUPT_THIRD_CRYSTAL && interrupts->enabled.third_crystal) {
		interrupts->interrupted.third_crystal = 1;
		should_interrupt |= 1;
		log_message(interrupts->asic->log,L_DEBUG, "interrupts", "Triggered third crystal interrupt");
	}

	if (should_interrupt) {
		interrupts->asic->cpu->interrupt = 1;
	}
}

void ti_interrupts_set_interrupt_enabled(ti_interrupts_t *interrupts, int flag, int set_to) {
	if (flag & INTERRUPT_ON_KEY) {
		interrupts->enabled.on_key = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "on key interrupt %s", set_to ? "enabled" : "disabled");
	}

	if (flag & INTERRUPT_FIRST_TIMER) {
		interrupts->enabled.first_timer = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "first timer interrupt %s", set_to ? "enabled" : "disabled");
	}

	if (flag & INTERRUPT_SECOND_TIMER) {
		interrupts->enabled.second_timer = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "second timer interrupt %s", set_to ? "enabled" : "disabled");
	}

	if (flag & INTERRUPT_LINK_ACTIVITY) {
		interrupts->enabled.link_activity = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "link activity interrupt %s", set_to ? "enabled" : "disabled");
	}

	if (flag & INTERRUPT_FIRST_CRYSTAL) {
		interrupts->enabled.first_crystal = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "first crystal interrupt %s", set_to ? "enabled" : "disabled");
	}

	if (flag & INTERRUPT_SECOND_CRYSTAL) {
		interrupts->enabled.second_crystal = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "second crystal interrupt %s", set_to ? "enabled" : "disabled");
	}

	if (flag & INTERRUPT_THIRD_CRYSTAL) {
		interrupts->enabled.third_crystal = set_to;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "third crystal interrupt %s", set_to ? "enabled" : "disabled");
	}
}

void ti_interrupts_acknowledge_interrupt(ti_interrupts_t *interrupts, int flag) {
	if (flag & INTERRUPT_ON_KEY) {
		interrupts->interrupted.on_key = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "on key interrupt acknowledged");
	}

	if (flag & INTERRUPT_FIRST_TIMER) {
		interrupts->interrupted.first_timer = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "first timer interrupt acknowledged");
	}

	if (flag & INTERRUPT_SECOND_TIMER) {
		interrupts->interrupted.second_timer = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "second timer interrupt acknowledged");
	}

	if (flag & INTERRUPT_LINK_ACTIVITY) {
		interrupts->interrupted.link_activity = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "link activity interrupt acknowledged");
	}

	if (flag & INTERRUPT_FIRST_CRYSTAL) {
		interrupts->interrupted.first_crystal = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "first crystal interrupt acknowledged");
	}

	if (flag & INTERRUPT_SECOND_CRYSTAL) {
		interrupts->interrupted.second_crystal = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "second crystal interrupt acknowledged");
	}

	if (flag & INTERRUPT_THIRD_CRYSTAL) {
		interrupts->interrupted.third_crystal = 0;
		log_message(interrupts->asic->log, L_DEBUG, "interrupts", "third crystal interrupt acknowledged");
	}

	ti_interrupts_check_state(interrupts);
}

uint8_t read_interrupt_mask(void *device) {
	ti_interrupts_t *interrupts = device;
	return
		(interrupts->enabled.on_key ? INTERRUPT_ON_KEY : 0) |
		(interrupts->enabled.first_timer ? INTERRUPT_FIRST_TIMER : 0) |
		(interrupts->enabled.second_timer ? INTERRUPT_SECOND_TIMER : 0) |
		(interrupts->enabled.link_activity ? INTERRUPT_LINK_ACTIVITY : 0) |
		(interrupts->enabled.first_crystal ? INTERRUPT_FIRST_CRYSTAL : 0) |
		(interrupts->enabled.second_crystal ? INTERRUPT_SECOND_CRYSTAL: 0) |
		(interrupts->enabled.third_crystal ? INTERRUPT_THIRD_CRYSTAL : 0);
}

void write_interrupt_mask(void *device, uint8_t value) {
	ti_interrupts_t *interrupts = device;

	interrupts->interrupted.on_key &= (interrupts->enabled.on_key = !!(value & INTERRUPT_ON_KEY));
	interrupts->interrupted.first_timer &= (interrupts->enabled.first_timer = !!(value & INTERRUPT_FIRST_TIMER));
	interrupts->interrupted.second_timer &= (interrupts->enabled.second_timer = !!(value & INTERRUPT_SECOND_TIMER));
	// TODO: low-power mode
	interrupts->interrupted.link_activity &= (interrupts->enabled.link_activity = !!(value & INTERRUPT_LINK_ACTIVITY));
	interrupts->interrupted.first_crystal &= (interrupts->enabled.first_crystal = !!(value & INTERRUPT_FIRST_CRYSTAL));
	interrupts->interrupted.second_crystal &= (interrupts->enabled.second_crystal = !!(value & INTERRUPT_SECOND_CRYSTAL));
	interrupts->interrupted.third_crystal &= (interrupts->enabled.third_crystal = !!(value & INTERRUPT_THIRD_CRYSTAL));

	ti_interrupts_check_state(interrupts);
}

uint8_t read_interrupting_device(void *device) {
	ti_interrupts_t *interrupts = device;
	return
		(interrupts->interrupted.on_key ? INTERRUPT_ON_KEY : 0) |
		(interrupts->interrupted.first_timer ? INTERRUPT_FIRST_TIMER : 0) |
		(interrupts->interrupted.second_timer ? INTERRUPT_SECOND_TIMER : 0) |
		(interrupts->interrupted.link_activity ? INTERRUPT_LINK_ACTIVITY : 0) |
		(interrupts->interrupted.first_crystal ? INTERRUPT_FIRST_CRYSTAL : 0) |
		(interrupts->interrupted.second_crystal ? INTERRUPT_SECOND_CRYSTAL: 0) |
		(interrupts->interrupted.third_crystal ? INTERRUPT_THIRD_CRYSTAL : 0);
}

void write_acknowledged_interrupts(void *device, uint8_t value) {
	ti_interrupts_t *interrupts = device;

	interrupts->interrupted.on_key &= !!(value & INTERRUPT_ON_KEY);
	interrupts->interrupted.first_timer &= !!(value & INTERRUPT_FIRST_TIMER);
	interrupts->interrupted.second_timer &= !!(value & INTERRUPT_SECOND_TIMER);
	interrupts->interrupted.link_activity &= !!(value & INTERRUPT_LINK_ACTIVITY);
	interrupts->interrupted.first_crystal &= !!(value & INTERRUPT_FIRST_CRYSTAL);
	interrupts->interrupted.second_crystal &= !!(value & INTERRUPT_SECOND_CRYSTAL);
	interrupts->interrupted.third_crystal &= !!(value & INTERRUPT_THIRD_CRYSTAL);
	ti_interrupts_check_state(interrupts);
}

double first_timer_table[2][4] =
	{{ 560, 248, 170, 118 }, { 512, 227.55, 146.29, 107.79 }};

double second_timer_table[2][4] =
	{{ 1120, 497, 344, 236 }, { 1024, 455.11, 292.57, 215.28 }};

void write_timer_speed(void *device, uint8_t value) {
	ti_interrupts_t *interrupts = device;

	value >>= 1;
	uint8_t timer_speed = value & 3;
	set_first_timer_frequency(device, first_timer_table[interrupts->asic->device != TI83p][timer_speed]);
	set_second_timer_frequency(device, second_timer_table[interrupts->asic->device != TI83p][timer_speed]);
}

void first_timer_tick(asic_t *asic, void *device) {
	ti_interrupts_t *interrupts = device;

	ti_interrupts_interrupt(interrupts, INTERRUPT_FIRST_TIMER);
}

void second_timer_tick(asic_t *asic, void *device) {
	ti_interrupts_t *interrupts = device;

	ti_interrupts_interrupt(interrupts, INTERRUPT_SECOND_TIMER);
}

void set_first_timer_frequency(void *device, double speed) {
	ti_interrupts_t *interrupts = device;

	if (interrupts->first_timer_id != -1) {
		asic_remove_timer(interrupts->asic, interrupts->first_timer_id);
		interrupts->first_timer_id = -1;
	}

	if (speed != 0) {
		interrupts->first_timer_id = asic_add_timer(interrupts->asic, 0, speed, first_timer_tick, interrupts);
	}
}

void set_second_timer_frequency(void *device, double speed) {
	ti_interrupts_t *interrupts = device;

	if (interrupts->second_timer_id != -1) {
		asic_remove_timer(interrupts->asic, interrupts->second_timer_id);
		interrupts->second_timer_id = -1;
	}

	if (speed != 0) {
		interrupts->second_timer_id = asic_add_timer(interrupts->asic, 0, speed, second_timer_tick, interrupts);
	}
}

z80iodevice_t init_interrupts(asic_t *asic, ti_interrupts_t **result) {
	*result = calloc(sizeof(ti_interrupts_t), 1);
	(*result)->asic = asic;
	(*result)->first_timer_id = -1;
	(*result)->second_timer_id = -1;
	z80iodevice_t device = { *result, read_interrupt_mask, write_interrupt_mask };
	return device;
}

