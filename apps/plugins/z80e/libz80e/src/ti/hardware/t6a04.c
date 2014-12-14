#include "ti/hardware/t6a04.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "log/log.h"

#ifdef CURSES
#include <curses.h>
#endif

#ifdef CURSES
#define lcd_print(...) wprintw(lcd->lcd_display, __VA_ARGS__)
#else
#define lcd_print(...) printf( __VA_ARGS__)
#endif

void setup_lcd_display(asic_t *asic, hook_info_t *hook) {
	ti_bw_lcd_t *lcd = malloc(sizeof(ti_bw_lcd_t));

	bw_lcd_reset(lcd);

	lcd->ram = malloc((64 * 120));
	int cY;
	int cX;
	for (cX = 0; cX < 64; cX++) {
		for (cY = 0; cY < 120; cY++) {
			bw_lcd_write_screen(lcd, cY, cX, 0);
		}
	}

	lcd->hook = hook;
	lcd->asic = asic;
	asic->cpu->devices[0x10].device = lcd;
	asic->cpu->devices[0x10].read_in = bw_lcd_status_read;
	asic->cpu->devices[0x10].write_out = bw_lcd_status_write;

	asic->cpu->devices[0x11].device = lcd;
	asic->cpu->devices[0x11].read_in = bw_lcd_data_read;
	asic->cpu->devices[0x11].write_out = bw_lcd_data_write;
}

uint8_t bw_lcd_read_screen(ti_bw_lcd_t *lcd, int Y, int X) {
	int location = X * 120 + Y;
	return !!(lcd->ram[location >> 3] & (1 << (location % 8)));
}

void bw_lcd_write_screen(ti_bw_lcd_t *lcd, int Y, int X, char val) {
	val = !!val;
	int location = X * 120 + Y;
	lcd->ram[location >> 3] &= ~(1 << (location % 8));
	lcd->ram[location >> 3] |= (val << (location % 8));
}

void bw_lcd_reset(ti_bw_lcd_t *lcd) {
	lcd->display_on = 0;
	lcd->word_length = 1;
	lcd->up = 1;
	lcd->counter = 0;
	lcd->Y = 0;
	lcd->Z = 0;
	lcd->X = 0;
	lcd->contrast = 0;
	lcd->op_amp1 = 0;
	lcd->op_amp2 = 0;
}

uint8_t bw_lcd_status_read(void *device) {
	ti_bw_lcd_t *lcd = device;

	uint8_t retval = 0;
	retval |= (0) << 7;
	retval |= (lcd->word_length) << 6;
	retval |= (lcd->display_on) << 5;
	retval |= (0) << 4;
	retval |= (lcd->counter) << 1;
	retval |= (lcd->up) << 0;

	return retval;
}

void bw_lcd_status_write(void *device, uint8_t val) {
	ti_bw_lcd_t *lcd = device;

	log_message(lcd->asic->log, L_DEBUG, "lcd", "Wrote 0x%02X (0b%d%d%d%d%d%d%d%d) to status", val,
		!!(val & (1 << 7)),
		!!(val & (1 << 6)),
		!!(val & (1 << 5)),
		!!(val & (1 << 4)),
		!!(val & (1 << 3)),
		!!(val & (1 << 2)),
		!!(val & (1 << 1)),
		!!(val & (1 << 0))
	);
	if (val & 0xC0) { // 0b11XXXXXX
		lcd->contrast = val & 0x3F;
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet contrast to 0x%02X", lcd->contrast);
	} else if (val & 0x80) { // 0b10XXXXXX
		lcd->X = val & 0x3F;
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet X (vertical!) to 0x%02X", lcd->X);
	} else if (val & 0x40) { // 0b01XXXXXX
		lcd->Z = val & 0x3F;
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet Z (vertical scroll) to 0x%02X", lcd->Z);
	} else if (val & 0x20) { // 0b001XXXXX
		lcd->Y = val & 0x1F;
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet Y (horizontal!) to 0x%02X", lcd->Y);
	} else if ((val & 0x18) == 0x18) { // 0b00011***
		// test mode - not emulating yet
	} else if (val & 0x10) { // 0b00010*XX
		lcd->op_amp1 = val & 0x03;
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet Op-Amp 1 to 0x%02X", lcd->op_amp1);
	} else if (val & 0x08) { // 0b00001*XX
		lcd->op_amp2 = val & 0x03;
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet Op-Amp 2 to 0x%02X", lcd->op_amp2);
	} else if (val & 0x04) { // 0b000001XX
		lcd->counter = !!(val & 0x02);
		lcd->up = !!(val & 0x01);
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tSet counter to %s and Up/Down to %s",
			lcd->counter ? "Y" : "X", lcd->up ? "Up" : "Down");
	} else if (val & 0x02) { // 0b0000001X
		lcd->display_on = !!(val & 0x01);
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tDisplay turned %s", lcd->display_on ? "ON" : "OFF");
	} else { // 0b0000000X
		lcd->word_length = !!(val & 0x01);
		log_message(lcd->asic->log, L_DEBUG, "lcd", "\tWord Length set to %d", lcd->word_length ? 8 : 6);
	}
}

void bw_lcd_advance_int_pointer(ti_bw_lcd_t *lcd, int *Y, int *X) {
	if (lcd->counter) { // Y
		(*Y)++;
		*Y = *Y % 120;
	} else { // X
		(*X)++;
		*X = *X % 64;
	}
}

void bw_lcd_advance_pointer(ti_bw_lcd_t *lcd) {
	int maxX = lcd->word_length ? 15 : 20;
	if (lcd->counter) { // Y
		if (!lcd->up) {
			lcd->Y--;
			if (lcd->Y < 0) {
				lcd->Y = maxX - 1;
			}
		} else {
			lcd->Y++;
			lcd->Y = lcd->Y % maxX;
		}
	} else { // X
		if (!lcd->up) {
			lcd->X--;
			if (lcd->X < 0) {
				lcd->X = 63;
			}
		} else {
			lcd->X++;
			lcd->X = lcd->X % 64;
		}
	}
}

uint8_t bw_lcd_data_read(void *device) {
	ti_bw_lcd_t *lcd = device;

	int cY = lcd->Y * (lcd->word_length ? 8 : 6);
	int cX = lcd->X;

	int max = lcd->word_length ? 8 : 6;
	int i = 0;
	uint8_t retval = 0;
	for (i = 0; i < max; i++) {
		retval |= bw_lcd_read_screen(lcd, cY, cX);
		retval <<= 1;
		cY++;
	}

	bw_lcd_advance_pointer(lcd);
	return retval;
}

void bw_lcd_data_write(void *device, uint8_t val) {
	ti_bw_lcd_t *lcd = device;

	int cY = lcd->Y * (lcd->word_length ? 8 : 6);
	int cX = lcd->X;

	int max = lcd->word_length ? 8 : 6;
	int i = 0;
	cY += max - 1;
	for (i = 0; i < max; i++) {
		bw_lcd_write_screen(lcd, cY, cX, val & (1 << i));
		cY--;
	}

	log_message(lcd->asic->log, L_DEBUG, "lcd", "Wrote %02X (0b%d%d%d%d%d%d%d%d) to %d (Y), %d (X)",
		val,
		!!(val & (1 << 7)), !!(val & (1 << 6)),
		!!(val & (1 << 5)), !!(val & (1 << 4)),
		!!(val & (1 << 3)), !!(val & (1 << 2)),
		!!(val & (1 << 1)), !!(val & (1 << 0)),
		lcd->Y, lcd->X);

	bw_lcd_advance_pointer(lcd);
	hook_on_lcd_update(lcd->hook, lcd);
}
