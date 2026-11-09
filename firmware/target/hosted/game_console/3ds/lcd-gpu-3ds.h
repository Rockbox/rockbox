/* GPU presenting for the bottom screen, see lcd-gpu-3ds.c */
#ifndef LCD_GPU_3DS_H
#define LCD_GPU_3DS_H

#include <stdbool.h>
#include <stdint.h>

bool lcd_gpu_init(int width, int height);
void lcd_gpu_exit(void);
bool lcd_gpu_ready(void);

/* false while the GPU is busy with the previous frame */
bool lcd_gpu_try_begin(void);
/* blocking: for interrupts off (early boot, panic); false if a frame is
 * already open */
bool lcd_gpu_begin(void);
/* the last frame is presented at the next VBlank; until then
 * lcd_gpu_end() can't render (the back buffer is still on screen) */
bool lcd_gpu_present_pending(void);
void lcd_gpu_copy_rows(const uint16_t *src, int src_stride,
                       int x, int y, int w, int h);
void lcd_gpu_end(void);

#endif
