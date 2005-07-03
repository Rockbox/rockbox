


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



/* This mess needs to be moved to another module; it's just here to
 * make things work in the mean time. */

void emu_run(void)
{
	void *timer = sys_timer();
	int delay;

	vid_begin();
	lcd_begin();
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
	rb->cpu_boost(true);
#endif
	while(shut==0)
	{
		cpu_emulate(2280);
		while (R_LY > 0 && R_LY < 144)
			emu_step();
		
		vid_end();
		rtc_tick();
		sound_mix();
		if (!pcm_submit())
		{
			delay = framelen - sys_elapsed(timer);
			sys_sleep(delay);
			sys_elapsed(timer);
		}
		doevents();
		vid_begin();
//		if (framecount) { if (!--framecount) die("finished\n"); }
		
		if (!(R_LCDC & 0x80))
			cpu_emulate(32832);
		
		while (R_LY > 0) /* wait for next frame */
			emu_step();
		rb->yield();
	}
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
		
}












