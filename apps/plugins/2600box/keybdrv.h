#ifndef KEYBDRV_H
#define KEYBDRV_H

void
keybdrv_close (void);

int
keybdrv_init (void);

void
keybdrv_setmap(void);

int
keybdrv_pressed(int key);

void
keybdrv_update (void);

#endif


