#include <stdio.h>
#include <system.h>
#include <inttypes.h>
#include "config.h"
#include "gcc_extensions.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "adc.h"
#include "button-target.h"
#include "button.h"
#include "common.h"
#include "storage.h"
#include "disk.h"
#include "panic.h"
#include "power.h"
#include "string.h"
#include "file.h"
#include "crc32-rkw.h"
#include "rkw-loader.h"

#define DRAM_ORIG 0x60000000
#define LOAD_SIZE 0x700000

extern void show_logo( void );

void main(void) NORETURN_ATTR;
void main(void)
{
    char filename[MAX_PATH];
    unsigned char* loadbuffer;
    void(*kernel_entry)(void);
    int ret;
    enum {rb, of} boot = rb;

    power_init();
    system_init();
    kernel_init();
    enable_irq();

    adc_init();
    lcd_init();
    backlight_init();
    button_init_device();

    font_init();
    lcd_setfont(FONT_SYSFIXED);

    show_logo();

    int btn = button_read_device();

    /* if there is some other button pressed
     * besides POWER/PLAY we boot into OF
     */
    if ((btn & ~POWEROFF_BUTTON))
        boot = of;

    /* if we are woken up by USB insert boot into OF */
    if (DEV_INFO & (1<<20))
        boot = of;

    lcd_clear_display();

    ret = storage_init();
    if(ret < 0)
        error(EATA, ret, true);

    while(!disk_init(IF_MV(0)))
        panicf("disk_init failed!");

    while((ret = disk_mount_all()) <= 0)
        error(EDISK, ret, true);

    loadbuffer = (unsigned char*)DRAM_ORIG; /* DRAM */
   
    if (boot == rb)
        snprintf(filename,sizeof(filename), BOOTDIR "/%s", BOOTFILE);    
    else if (boot == of)
        snprintf(filename,sizeof(filename), BOOTDIR "/%s", "BASE.RKW");

    printf("Loading: %s", filename);

    ret = load_rkw(loadbuffer, filename, LOAD_SIZE);
    if (ret < 0)
    {
        printf(rkw_strerror(ret));
        lcd_update();
        sleep(5*HZ);
        power_off();
    }
    else
    {
        printf(rkw_strerror(0));
        sleep(HZ);
    }

    kernel_entry = (void*) loadbuffer;
    commit_discard_idcache();

    printf("Executing");
    kernel_entry();

    printf("ERR: Failed to boot");
    sleep(5*HZ);
    power_off();

    /* hang */
    while(1);
}
