


#include "rockmacros.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu-gb.h"
#include "mem.h"
#include "lcd-gb.h"
#include "rc.h"
#include "sound.h"
#include "rtc-gb.h"

static int framelen = 16743;
static int framecount;

rcvar_t emu_exports[] =
{
	RCV_INT("framelen", &framelen),
	RCV_INT("framecount", &framecount),
	RCV_END
};







void emu_init(void)
{
	
}


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





void emu_step(void)
{
	cpu_emulate(cpu.lcdc);
}

struct options options;

/* This mess needs to be moved to another module; it's just here to
 * make things work in the mean time. */
//extern struct plugin_api* rb;
void emu_run(void)
{
//	void *timer = sys_timer();
   int framesin=0,frames=0,timeten=*rb->current_tick, timehun=*rb->current_tick;
//	int delay;

	vid_begin();
	lcd_begin();
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
	rb->cpu_boost(true);
#endif
	while(!shut)
	{
		cpu_emulate(2280);
		while (R_LY > 0 && R_LY < 144)
			emu_step();
		
		rtc_tick();
		sound_mix();
		if (!pcm_submit())
		{
/*			delay = framelen - sys_elapsed(timer);
			sys_sleep(delay);
			sys_elapsed(timer);*/
		}

		doevents();
		vid_begin();
		
		if (!(R_LCDC & 0x80))
			cpu_emulate(32832);
		
		while (R_LY > 0) /* wait for next frame */
			emu_step();
		rb->yield();

      frames++;
      framesin++;

      if(*rb->current_tick-timeten>=20)
      {
         timeten=*rb->current_tick;
         if(framesin<12) options.frameskip++;
         if(framesin>12) options.frameskip--;
         if(options.frameskip>options.maxskip) options.frameskip=options.maxskip;
         if(options.frameskip<0) options.frameskip=0;
         framesin=0;
	}

      if(options.showstats)
      {
         if(*rb->current_tick-timehun>=100) {
            options.fps=frames;
            frames=0;
            timehun=*rb->current_tick;
         }
      }

	}

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
}












