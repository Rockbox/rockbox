#include <stdio.h>
#include <inttypes.h>
#include "config.h"
#include "system.h"
#include "../kernel-internal.h"
#include "gcc_extensions.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "adc.h"
#include "button-target.h"
#include "button.h"
#include "common.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "panic.h"
#include "power.h"
#include "string.h"
#include "file.h"
#include "crc32-rkw.h"
#include "rkw-loader.h"
#include "version.h"
#include "i2c-rk27xx.h"
#include "loader_strerror.h"

/* beginning of DRAM */
#define DRAM_ORIG 0x60000000

/* bootloader code runs from 0x60700000
 * so we cannot load more code to not overwrite ourself
 */
#define LOAD_SIZE 0x700000

extern void show_logo( void );

/* This function setup bare minimum
 * and jumps to rom in order to activate
 * hardcoded rkusb mode
 */
static void enter_rkusb(void)
{
    asm volatile (
        /* Turn off cache */
        "ldr     r0, =0xefff0000 \n"
        "ldrh    r1, [r0] \n"
        "strh    r1, [r0] \n"

        /* Turn off interrupts */
        "mrs     r0, cpsr \n"
        "bic     r0, r0, #0x1f \n"
        "orr     r0, r0, #0xd3 \n"
        "msr     cpsr, r0 \n"

        /* Disable iram remap */
        "mov     r0, #0x18000000 \n"
        "add     r0, r0, #0x1c000 \n"
        "mov     r1, #0 \n"
        "str     r1, [r0, #4] \n"

        /* Ungate all clocks */
        "str     r1, [r0, #0x18] \n"

        /* Read SCU_ID to determine
         * which version of bootrom we have
         * 2706A has ID 0xa1000604
         * 2706B and 2705 have ID 0xa100027b
         */
        "ldr     r1, [r0] \n"
        "ldr     r2, =0xa1000604 \n"
        "cmp     r1, r2 \n"
        "bne     rk27xx_new \n"

        /* Setup stacks in unmapped
         * iram just as rom will do.
         *
         * We know about two versions
         * of bootrom which are very similar
         * but memory addresses are slightly
         * different.
         */
        "rk27xx_old: \n"
        "ldr     r1, =0x18200258 \n"
        "ldr     r0, =0xaf0 \n"
        "b       jump_to_rom \n"

        "rk27xx_new: \n"
        "ldr     r1, =0x18200274 \n"
        "ldr     r0, =0xec0 \n"

        "jump_to_rom: \n"
        "msr     cpsr, #0xd2 \n"
        "add     r1, r1, #0x200 \n"
        "mov     sp, r1 \n"
        "msr     cpsr, #0xd3 \n"
        "add     r1, r1, #0x400 \n"
        "mov     sp, r1 \n"

        /* Finaly jump to rkusb handler
         * in bootrom.
         */
        "bx      r0 \n"
    );
}


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
    i2c_init();
    enable_irq();

    adc_init();
    lcd_init();
    backlight_init();
    button_init_device();

    font_init();
    lcd_setfont(FONT_SYSFIXED);

    show_logo();

    sleep(HZ);

    lcd_clear_display();

    ret = storage_init();
    if(ret < 0)
        error(EATA, ret, true);

    filesystem_init();

    while((ret = disk_mount_all()) <= 0)
        error(EDISK, ret, true);

    loadbuffer = (unsigned char*)DRAM_ORIG; /* DRAM */

    int btn = button_read_device();

    /* if there is some other button pressed
     * besides POWER/PLAY we boot into OF
     */
    if ((btn & ~POWEROFF_BUTTON))
        boot = of;

    /* if we are woken up by USB insert boot into OF */
    if (DEV_INFO & (1<<20))
        boot = of;

    if (boot == rb)
        snprintf(filename,sizeof(filename), BOOTDIR "/%s", BOOTFILE);    
    else if (boot == of)
        snprintf(filename,sizeof(filename), BOOTDIR "/%s", "BASE.RKW");

    printf("Bootloader version: %s", rbversion);
    printf("Loading: %s", filename);

    ret = load_rkw(loadbuffer, filename, LOAD_SIZE);

    if (ret <= EFILE_EMPTY)
    {
        error(EBOOTFILE, ret, false);

        /* if we boot rockbox we shutdown on error
         * if we boot OF we fall back to rkusb mode on error
         */        
        if (boot == rb)
        {
            power_off();
        }
        else
        {
            /* give visual feedback what we are doing */
            printf("Entering rockchip USB mode...");
            lcd_update();

            enter_rkusb();
        }
    }
    else
    {
        /* print 'Loading OK' */
        printf("Loading OK");
    }

    /* jump to entrypoint */
    kernel_entry = (void*) loadbuffer;
    commit_discard_idcache();

    printf("Executing");
    kernel_entry();

    /* this should never be reached actually */
    printf("ERR: Failed to boot");
    sleep(5*HZ);

    if (boot == rb)
    {
        power_off();
    }
    else
    {
        /* give visual feedback what we are doing */
        printf("Entering rockchip USB mode...");
        lcd_update();

        enter_rkusb();
    }   

    /* hang */
    while(1);
}
