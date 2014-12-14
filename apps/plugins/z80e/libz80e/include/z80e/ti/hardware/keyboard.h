#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <z80e/core/cpu.h>

z80iodevice_t init_keyboard();
void free_keyboard(void *keyboard);
void release_key(void *keyboard, uint8_t keycode);
void depress_key(void *keyboard, uint8_t keycode);

#endif
