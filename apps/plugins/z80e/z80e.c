#include "plugin.h"

#include "frontend.h"
#include <z80e/ti/asic.h>
#include <z80e/debugger/debugger.h>
#include <z80e/disassembler/disassemble.h>
#include <z80e/runloop/runloop.h>
#include "tui.h"
#include <z80e/debugger/commands.h>
#include <z80e/log/log.h>
#include <z80e/ti/hardware/t6a04.h>
#include <z80e/ti/hardware/keyboard.h>
#include <z80e/ti/hardware/interrupts.h>

typedef struct {
    char *key;
    loglevel_t level;
} loglevel_options_t;

const loglevel_options_t log_options[] = {
    { "DEBUG", L_DEBUG },
    { "WARN", L_WARN },
    { "ERROR", L_ERROR },
    { "INFO", L_INFO }
};

typedef struct {
    ti_device_type device;
    asic_t *device_asic;
    char *rom_file;
    int cycles;
    int print_state;
    int no_rom_check;
    loglevel_t log_level;
    int braille;
    int scale;
    int offset_l;
    int offset_t;
} appContext_t;

appContext_t context;

appContext_t create_context(void) {
    appContext_t context;
    context.device = TI83p;
    context.rom_file = NULL;
    context.cycles = -1;
    context.print_state = 0;
    context.no_rom_check = 0;
    context.log_level = L_WARN;
    context.braille = 0;
    context.scale = 5;
    context.offset_l = 0;
    context.offset_t = 0;
    return context;
}

int lcd_changed = 0;
void lcd_changed_hook(void *data, ti_bw_lcd_t *lcd) {
    lcd_changed = 1;
}

void tui_unicode_to_utf8(char *b, uint32_t c) {
    if (c<0x80) *b++=c;
    else if (c<0x800) *b++=192+c/64, *b++=128+c%64;
    else if (c-0xd800u<0x800) return;
    else if (c<0x10000) *b++=224+c/4096, *b++=128+c/64%64, *b++=128+c%64;
    else if (c<0x110000) *b++=240+c/262144, *b++=128+c/4096%64, *b++=128+c/64%64, *b++=128+c%64;
}

void print_lcd(void *data, ti_bw_lcd_t *lcd) {
    int cY;
    int cX;

    if (context.braille) {
        /* rockbox doesn't do braille :P */
    }

    for (cX = 0; cX < 64; cX += 1) {
        for (cY = 0; cY < 96; cY += 1) {
            int on = bw_lcd_read_screen(lcd, cY, cX);
            int i, j;
            for (i = 0; i < context.scale; ++i) {
                for (j = 0; j < context.scale; ++j) {
                    //uint8_t *p = (uint8_t *)rb->lcd_framebuffer + (context.offset_t + cX * context.scale + i) *
                    //    context.screen->pitch + (context.offset_l + cY * context.scale + j) *
                    //    context.screen->format->BytesPerPixel;
                    //*(uint32_t *)p = on ? 0xFF000000 : 0xFF99b199;
                }
            }
        }
    }

    rb->lcd_update();

    /* Push pixels to display */

    if (context.log_level >= L_INFO) {
        printf("C: 0x%02X X: 0x%02X Y: 0x%02X Z: 0x%02X\n", lcd->contrast, lcd->X, lcd->Y, lcd->Z);
        printf("   %c%c%c%c  O1: 0x%01X 02: 0x%01X\n", lcd->up ? 'V' : '^', lcd->counter ? '-' : '|',
               lcd->word_length ? '8' : '6', lcd->display_on ? 'O' : ' ', lcd->op_amp1, lcd->op_amp2);
    }
}

void lcd_timer_tick(asic_t *asic, void *data) {
    ti_bw_lcd_t *lcd = data;
    if (lcd_changed) {
        print_lcd(asic, lcd);
        lcd_changed = 0;
    }
}

void setDevice(appContext_t *context, char *target) {
    if (strcasecmp(target, "TI73") == 0) {
        context->device = TI73;
    } else if (strcasecmp(target, "TI83p") == 0) {
        context->device = TI83p;
    } else if (strcasecmp(target, "TI83pSE") == 0) {
        context->device = TI83pSE;
    } else if (strcasecmp(target, "TI84p") == 0) {
        context->device = TI84p;
    } else if (strcasecmp(target, "TI84pSE") == 0) {
        context->device = TI84pSE;
    } else if (strcasecmp(target, "TI84pCSE") == 0) {
        context->device = TI84pCSE;
    } else {
        printf("Incorrect usage. See z80e --help.\n");
        exit(1);
    }
}

void print_help(void) {
    printf("z80e - Emulate z80 calculators\n"
           "Usage: z80e [flags] [rom]\n\n"
           "Flags:\n"
           "\t-d <device>: Selects a device to emulate. Available devices:\n"
           "\t\tTI73, TI83p, TI83pSE, TI84p, TI84pSE, TI84pCSE\n"
           "\t-c <cycles>: Emulate this number of cycles, then exit. If omitted, the machine will be emulated indefinitely.\n"
           "\t-s <scale>: Scaleoffset_l, offset_t, 96 * scale, 64 * scale window by <scale> times.\n"
           "\t--braille: Enables braille display in console.\n"
           "\t--print-state: Prints the state of the machine on exit.\n"
           "\t--no-rom-check: Skips the check that ensure the provided ROM file is the correct size.\n"
           "\t--debug: Enable the debugger, which is enabled by interrupting the emulator.\n");
}

void handleFlag(appContext_t *context, char flag, int *i, char **argv) {
    char *next;
    switch (flag) {
    case 'd':
        next = argv[++*i];
        setDevice(context, next);
        break;
    case 'c':
        next = argv[++*i];
        context->cycles = atoi(next);
        break;
    case 's':
        next = argv[++*i];
        context->scale = atoi(next);
        break;
    default:
        printf("Incorrect usage. See z80e --help.\n");
        exit(1);
        break;
    }
}

int enable_debug = 0;

void handleLongFlag(appContext_t *context, char *flag, int *i, char **argv) {
    if (strcasecmp(flag, "device") == 0) {
        char *next = argv[++*i];
        setDevice(context, next);
    } else if (strcasecmp(flag, "print-state") == 0) {
        context->print_state = 1;
    } else if (strcasecmp(flag, "no-rom-check") == 0) {
        context->no_rom_check = 1;
    } else if (strcasecmp(flag, "braille") == 0) {
        context->braille = 1;
    } else if (strcasecmp(flag, "log") == 0) {
        char *level = argv[++*i];
        int j;
        for (j = 0; j < sizeof(log_options) / sizeof(loglevel_options_t); ++j) {
            if (strcasecmp(level, log_options[j].key) == 0) {
                context->log_level = log_options[j].level;
                return;
            }
        }
        printf("%s is not a valid logging level.\n", level);
        exit(1);
    } else if (strcasecmp(flag, "debug") == 0) {
        enable_debug = 1;
    } else if (strcasecmp(flag, "help") == 0) {
        print_help();
        exit(0);
    } else {
        printf("Incorrect usage. See z80e --help.\n");
        exit(1);
    }
}

void frontend_log(void *data, loglevel_t level, const char *part, const char *message, va_list args) {
    static char buf[128];
    vsnprintf(buf, sizeof(buf), message, args);
    printf("\n");
}

void key_tap(asic_t *asic, int scancode, int down) {
    if (down) {
        depress_key(asic->cpu->devices[0x01].device, scancode);
    } else {
        release_key(asic->cpu->devices[0x01].device, scancode);
    }
}

void setup_display(int w, int h) {

    int best_w_scale = w / 96;
    int best_h_scale = h / 64;

    int y, x;

    if (best_w_scale < best_h_scale) {
        context.scale = best_w_scale;
    } else {
        context.scale = best_h_scale;
    }

    context.offset_l = (w - context.scale * 96) / 2;
    context.offset_t = (h - context.scale * 64) / 2;

    rb->lcd_set_background(LCD_RGBPACK(0xc6, 0xe6, 0xc6));
    rb->lcd_clear_display();

    lcd_changed = 1;
}

void sdl_events_hook(asic_t *device, void * unused) {

}

int rockbox_main(int argc, char **argv) {
    context = create_context();

    int i;
    for (i = 1; i < argc; i++) {
        char *a = argv[i];
        if (*a == '-') {
            a++;
            if (*a == '-') {
                handleLongFlag(&context, a + 1, &i, argv);
            } else {
                while (*a) {
                    handleFlag(&context, *a++, &i, argv);
                }
            }
        } else {
            if (context.rom_file == NULL) {
                context.rom_file = a;
            } else {
                printf("Incorrect usage. See z80e --help.\n");
                return 1;
            }
        }
    }

    context.rom_file = "/.rockbox/z80e/rom";
    log_t *log = init_log(frontend_log, 0, context.log_level);
    asic_t *device = asic_init(context.device, log);
    context.device_asic = device;

    if (enable_debug) {
        device->debugger = init_debugger(device);
        device->debugger->state = DEBUGGER_ENABLED;
    }

    if (context.rom_file == NULL && !enable_debug) {
        log_message(device->log, L_WARN, "main", "No ROM file specified, starting debugger");
        device->debugger = init_debugger(device);
        device->debugger->state = DEBUGGER_ENABLED;
    } else {
        int file = rb->open(context.rom_file, O_RDONLY);
        if (file<0) {
            printf("Error opening '%s'.\n", context.rom_file);
            asic_free(device);
            return 1;
        }
        int length;
        length=rb->lseek(file, 0L, SEEK_END);
        rb->lseek(file, 0L, SEEK_SET);
        if (!context.no_rom_check && length != device->mmu->settings.flash_pages * 0x4000) {
            printf("Error: This file does not match the required ROM size of %d bytes, but it is %d bytes (use --no-rom-check to override).\n",
                   device->mmu->settings.flash_pages * 0x4000, length);
            rb->close(file);
            asic_free(device);
            return 1;
        }
        length = rb->read(file, device->mmu->flash, 0x4000*device->mmu->settings.flash_pages);
        rb->close(file);
    }

    setup_display(LCD_WIDTH, LCD_HEIGHT);

    hook_add_lcd_update(device->hook, NULL, lcd_changed_hook);
    asic_add_timer(device, 0, 60, lcd_timer_tick, device->cpu->devices[0x10].device);
    asic_add_timer(device, 0, 100, sdl_events_hook, device->cpu->devices[0x10].device);

    if (device->debugger) {
        tui_state_t state = { device->debugger };
        tui_init(&state);
        tui_tick(&state);
    } else {
        if (context.cycles == -1) { // Run indefinitely
            while (1) {
                runloop_tick(device->runloop);
                if (device->stopped) {
                    break;
                }
                //nanosleep((struct timespec[]){{0, (1.f / 60.f) * 1000000000}}, NULL);
            }
        } else {
            runloop_tick_cycles(device->runloop, context.cycles);
        }
    }

finish:

    if (context.print_state) {
        print_state(&device->cpu->registers);
    }
    asic_free(device);
    return 0;
}

enum plugin_status plugin_start(const void *param)
{
    const char* argv[] = {
        rb->plugin_get_current_filename()
    };
    if(rockbox_main(ARRAYLEN(argv), argv)==0)
        return PLUGIN_OK;
    else
        return PLUGIN_ERROR;
}
