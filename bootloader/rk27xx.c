#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "backlight.h"
#include "backlight-target.h"
#include "font.h"
#include "common.h"
#include "version.h"

extern int show_logo( void );
void main(void)
{

    _backlight_init();

    system_init();
    kernel_init();
    enable_irq();

    lcd_init_device();
    _backlight_on();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    show_logo();
    sleep(HZ*2);

    while(1)
    {
        reset_screen();
        printf("GPIOA: 0x%0x", GPIO_PADR);
        printf("GPIOB: 0x%0x", GPIO_PBDR);
        printf("GPIOC: 0x%0x", GPIO_PCDR);
        printf("GPIOD: 0x%0x", GPIO_PDDR);
        sleep(HZ/10);
    }
}
