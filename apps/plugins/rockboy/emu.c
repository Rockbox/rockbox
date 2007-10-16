#include "rockmacros.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu-gb.h"
#include "mem.h"
#include "lcd-gb.h"
#include "sound.h"
#include "rtc-gb.h"
#include "pcm.h"

/*
 * emu_reset is called to initialize the state of the emulated
 * system. It should set cpu registers, hardware registers, etc. to
 * their appropriate values at powerup time.
 */

void emu_reset(void)
{
    hw_reset();
    lcd_reset();
    cpu_reset();
    mbc_reset();
    sound_reset();
}

static void emu_step(void)
{
    cpu_emulate(cpu.lcdc);
}

/* This mess needs to be moved to another module; it's just here to
 * make things work in the mean time. */
void emu_run(void)
{
    int framesin=0,frames=0,timeten=*rb->current_tick, timehun=*rb->current_tick;

    setvidmode();
    vid_begin();
    lcd_begin();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    while(!shut)
    {
        cpu_emulate(2280);
        while (R_LY > 0 && R_LY < 144)
            emu_step();

        rtc_tick();

        if (options.sound || !plugbuf)
		{
            sound_mix();
            pcm_submit();
		}

        doevents();
        vid_begin();

        if (!(R_LCDC & 0x80))
            cpu_emulate(32832);
        
        while (R_LY > 0) /* wait for next frame */
        {
            emu_step();
            rb->yield();
        }

        frames++;
        framesin++;

        if(*rb->current_tick-timeten>=10)
        {
            timeten=*rb->current_tick;
            if(framesin<6) options.frameskip++;
            if(framesin>6) options.frameskip--;
            if(options.frameskip>options.maxskip) options.frameskip=options.maxskip;
            if(options.frameskip<0) options.frameskip=0;
            framesin=0;
        }

        if(options.showstats)
            if(*rb->current_tick-timehun>=100)
            {
                options.fps=frames;
                frames=0;
                timehun=*rb->current_tick;
            }
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}
