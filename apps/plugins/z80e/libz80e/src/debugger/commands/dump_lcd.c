#include "ti/hardware/t6a04.h"
#include "debugger/debugger.h"
#include "debugger/commands.h"

void dump_lcd_unicode_to_utf8(char *b, uint32_t c) {
	if (c<0x80) *b++=c;
	else if (c<0x800) *b++=192+c/64, *b++=128+c%64;
	else if (c-0xd800u<0x800) return;
	else if (c<0x10000) *b++=224+c/4096, *b++=128+c/64%64, *b++=128+c%64;
	else if (c<0x110000) *b++=240+c/262144, *b++=128+c/4096%64, *b++=128+c/64%64, *b++=128+c%64;
}

int command_dump_lcd(debugger_state_t *state, int argc, char **argv) {
	ti_bw_lcd_t *lcd = state->asic->cpu->devices[0x10].device;
	int cY;
	int cX;

	#define LCD_BRAILLE
	#ifndef LCD_BRAILLE
		for (cX = 0; cX < 64; cX++) {
			for (cY = 0; cY < 120; cY++) {
				state->print(state, "%c", bw_lcd_read_screen(lcd, cY, cX) ? 'O' : ' ');
			}
			state->print(state, "\n");
		}
	#else
		for (cX = 0; cX < 64; cX += 4) {
			for (cY = 0; cY < 120; cY += 2) {
				int a = bw_lcd_read_screen(lcd, cY + 0, cX + 0);
				int b = bw_lcd_read_screen(lcd, cY + 0, cX + 1);
				int c = bw_lcd_read_screen(lcd, cY + 0, cX + 2);
				int d = bw_lcd_read_screen(lcd, cY + 1, cX + 0);
				int e = bw_lcd_read_screen(lcd, cY + 1, cX + 1);
				int f = bw_lcd_read_screen(lcd, cY + 1, cX + 2);
				int g = bw_lcd_read_screen(lcd, cY + 0, cX + 3);
				int h = bw_lcd_read_screen(lcd, cY + 1, cX + 3);
				uint32_t byte_value = 0x2800;
				byte_value += (
					(a << 0) |
					(b << 1) |
					(c << 2) |
					(d << 3) |
					(e << 4) |
					(f << 5) |
					(g << 6) |
					(h << 7));
				char buff[5] = {0};
				dump_lcd_unicode_to_utf8(buff, byte_value);
				state->print(state, "%s", buff);
			}
			state->print(state, "\n");
		}
	#endif
	state->print(state, "C: 0x%02X X: 0x%02X Y: 0x%02X Z: 0x%02X\n", lcd->contrast, lcd->X, lcd->Y, lcd->Z);
	state->print(state, "   %c%c%c%c  O1: 0x%01X 02: 0x%01X\n", lcd->up ? 'V' : '^', lcd->counter ? '-' : '|',
		lcd->word_length ? '8' : '6', lcd->display_on ? 'O' : ' ', lcd->op_amp1, lcd->op_amp2);
	return 0;
}
