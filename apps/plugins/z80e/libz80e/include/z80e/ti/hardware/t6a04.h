#ifndef TI_DISPLAY_H
#define TI_DISPLAY_H

typedef struct ti_bw_lcd ti_bw_lcd_t;

#include <z80e/ti/asic.h>
#include <z80e/debugger/hooks.h>

struct ti_bw_lcd {
	uint8_t up: 1; // set=up, unset=down
	uint8_t counter: 1; // set=Y, unset=X
	uint8_t word_length: 1; // set=8, unset=6
	uint8_t display_on: 1; // set=on, unset=off
	uint8_t op_amp1: 2; // 0-3
	uint8_t op_amp2: 2; // 0-3

	int X; // which is up-down
	int Y; // which is left-right
	int Z; // which is which y is rendered at top
	uint8_t contrast; // 0-63
	uint8_t *ram; // [X * 64 + Y]

	hook_info_t *hook;
	asic_t *asic;
};

void setup_lcd_display(asic_t *, hook_info_t *);

uint8_t bw_lcd_read_screen(ti_bw_lcd_t *, int, int);
void bw_lcd_write_screen(ti_bw_lcd_t *, int, int, char);

void bw_lcd_reset(ti_bw_lcd_t *);

uint8_t bw_lcd_status_read(void *);
void bw_lcd_status_write(void *, uint8_t);

uint8_t bw_lcd_data_read(void *);
void bw_lcd_data_write(void *, uint8_t);

void bw_lcd_advance_int_pointer(ti_bw_lcd_t *, int *, int *);
void bw_lcd_advance_pointer(ti_bw_lcd_t *);

#endif
