#include <stdio.h>
#include <string.h>

#include "rockmacros.h"
#include "input.h"
#include "emu.h"
#include "loader.h"
#include "hw.h"

void doevents()
{
    event_t ev;
    int st;

    ev_poll();
    while (ev_getevent(&ev))
    {
        if (ev.type != EV_PRESS && ev.type != EV_RELEASE)
            continue;
        st = (ev.type != EV_RELEASE);
        pad_set(ev.code, st);
    }
}

int gnuboy_main(char *rom)
{
    rb->lcd_puts(0,0,"Init video");
    vid_init();
    rb->lcd_puts(0,1,"Init sound");
    pcm_init();
    rb->lcd_puts(0,2,"Loading rom");
    loader_init(rom);
    if(shut)
        return PLUGIN_ERROR;
    rb->lcd_puts(0,3,"Emu reset");
    emu_reset();
    rb->lcd_puts(0,4,"Emu run");
    rb->lcd_clear_display();
    rb->lcd_update();
    emu_run();

    /* never reached */
    return PLUGIN_OK;
}
