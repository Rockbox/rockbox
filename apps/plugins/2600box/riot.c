
/*
 * based on x2600, the Atari Emulator, copyright 1996 by Alex Hornby
 * most of this code was located in vmachine.c
 */

#include "rbconfig.h"
#include "rb_test.h"

#include "vmachine.h"
#include "keyboard.h"
#include "options.h"
#include "memory.h"
#include "cpu.h"

#include "riot.h"

/* RIOT Addresses (minus RIOT base address (0x280)) */
#include "address.h"

/* =========================================================================
 * This file handles the riot I/O and timer functions.
 * RIOT ram is handled in memory.c
 * ========================================================================= */

struct riot riot;

/* Initialise the RIOT (also known as PIA) */
void init_riot (void) COLD_ATTR;
void init_riot (void)
{
  /* Reset the arrays */
  for(unsigned i=0; i< ARRAYLEN(riot.read);i++)
    {
      riot.read[i]=0;
      riot.write[i]=0;
    }

  /* Set the timer to zero */
  riot.read[INTIM] = 0;

  /* Set the joysticks and switches to input */
  riot.write[SWACNT] = 0;
  riot.write[SWBCNT] = 0;

  /* Centre the joysticks */
  riot.read[SWCHA] = 0xff;
  riot.read[SWCHB] = 0x0b;

  /* Set the counter resolution */
  riot.timer_res = 32;
  riot.timer_count = 0;
  riot.timer_clks = 0;
}


/* Calculate the keypad rows */
/* i.e. when reading from INPTx we don't know the row */
extern inline BYTE do_keypad (int pad, int col)
{
  BYTE res= 0x80;

  (void) pad;
  (void) col;
#if 0
  read_keypad(pad);

  /* Bottom row */
  if(pad==0) {
  if( (riot.write[SWCHA] & 0x80) && keypad[pad][col]==3)
    res=0x00;
  /* Third row */
  if( (riot.write[SWCHA] & 0x40) && keypad[pad][col]==2)
    res=0x00;
  if( (riot.write[SWCHA] & 0x20) && keypad[pad][col]==1)
    res=0x00;
  if( (riot.write[SWCHA] & 0x10) && keypad[pad][col]==0)
    res=0x00;
  }
  else {
  /* Bottom row */
  if( (riot.write[SWCHA] & 0x80) && keypad[pad][col]==3)
    res=0x00;
  /* Third row */
  if( (riot.write[SWCHA] & 0x40) && keypad[pad][col]==2)
    res=0x00;
  if( (riot.write[SWCHA] & 0x20) && keypad[pad][col]==1)
    res=0x00;
  if( (riot.write[SWCHA] & 0x10) && keypad[pad][col]==0)
    res=0x00;
  }
#endif // 0
  return res;
}


/*
   Called when the timer is set .
   Note that res is the bit shift, not absolute value.
   Assumes that any timer interval set will last longer than the instruction
   setting it.
 */
/* res: timer interval resolution as a bit shift value */
/* count: the number of intervals to set */
/* clkadj: the number of CPU cycles into the current instruction  */
static inline void set_timer (int res, int count)
{
    TST_PROFILE(PRF_SETTIMER, "set_timer()");
    riot.timer_res = res;
    riot.timer_end = clk + (count << res);

    riot.timer_clks = clk;
}


/* New timer code, now only called on a read of INTIM */
/* clkadj: the number of CPU cycles into the current instruction  */
/* returns: the current timer value */
static inline unsigned do_timer (void)
{
  unsigned result;
  int value;

  TST_PROFILE(PRF_DOTIMER, "do_timer()");
  value = (riot.timer_end - clk) >> riot.timer_res;
  if (clk <= riot.timer_end)
    {                           /* Timer is still going down in res intervals */
      result = value;
    }
  else
    {
      /* Timer is descending from 0xff in clock intervals */
      set_timer (0, 0xff);
      result = 0;
    }

  return result;
}


/*
 * memory mapped riot access
 */

unsigned read_riot(unsigned a)
{
    a &= 0x1f;
    switch (a) {
    /* Timer output */
    case INTIM:
    case 0x05:
    case 0x06:
    case TIM1T:
    case TIM8T:
    case TIM64T:
    case T1024T:
        return do_timer();
        break;
    case SWCHA:
        switch (base_opts.lcon) {
        case PADDLE:
#if 0
            if (base_opts.lcon == PADDLE) {
                if (mouse_button ())
                    riot.read[SWCHA] &= 0x7f;
                else
                    riot.read[SWCHA] |= 0x80;
            }
            else if (base_opts.rcon == PADDLE) {
                if (mouse_button ())
                    riot.read[SWCHA] &= 0xbf;
                else
                    riot.read[SWCHA] |= 0x40;
            }
#endif
            break;
        case STICK:
            read_stick ();
            break;
        }
        return riot.read[SWCHA];
        break;
        /* Switch B is hardwired to input */
    case SWCHB:
        read_console ();
        return riot.read[SWCHB];
        break;
    default:
        DEBUGF("Unknown riot read 0x%x\n", a);
        return 65; // ???
        break;
    }
}

void write_riot(unsigned a, unsigned b)
{
    a &= 0x1f;

    TST_PRF_RIOT_W((unsigned)(a));
    switch (a) {
        /* RIOT I/O ports */
    case SWCHA:
        riot.write[SWCHA] = b;
        break;
    case SWACNT:
        riot.write[SWACNT] = b;
        break;
    case SWCHB:
    case SWBCNT:
        /* Do nothing */
        break;

        /* Timer ports */
    case TIM1T:
        set_timer (0, b);
        break;
    case TIM8T:
        set_timer (3, b);
        break;
    case TIM64T:
        set_timer (6, b);
        break;
    case T1024T:
        set_timer (10, b);
        break;
    default:
        DEBUGF("Unknown riot write %x\n", a);
        break;
    }
}
