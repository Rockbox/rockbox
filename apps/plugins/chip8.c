/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Orginal from Vision-8 Emulator / Copyright (C) 1997-1999  Marcel de Kogel
 * Modified for Archos by Blueloop (a.wenger@gmx.de)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

/* Only build for (correct) target */
#ifdef HAVE_LCD_BITMAP 
#ifndef SIMULATOR /* not unless lcd_blit() is implemented and mp3_xx stubbed */

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD /* only 9 out of 16 chip8 buttons */
#define CHIP8_KEY1 BUTTON_F1
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY3 BUTTON_F3
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_PLAY
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY7 BUTTON_F2
#define CHIP8_KEY8 BUTTON_DOWN
#define CHIP8_KEY9 BUTTON_ON

#elif CONFIG_KEYPAD == ONDIO_PAD /* even more limited */
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_MENU
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_DOWN

#endif

static struct plugin_api* rb; /* here is a global api struct pointer */
unsigned char lcd_framebuf[8][64]; /* frame buffer in hardware fomat */

typedef unsigned char byte;                     /* sizeof(byte)==1          */
typedef unsigned short word;                    /* sizeof(word)>=2          */

struct chip8_regs_struct
{
 byte alg[16];                                  /* 16 general registers     */
 byte delay,sound;                              /* delay and sound timer    */
 word i;                                        /* index register           */
 word pc;                                       /* program counter          */
 word sp;                                       /* stack pointer            */
};

static struct chip8_regs_struct chip8_regs;

#define chip8_iperiod 10                        /* number of opcodes per    */
                                                /* timeslice (1/50sec.)     */
static byte chip8_key_pressed;
static byte chip8_keys[16];                     /* if 1, key is held down   */
static byte chip8_display[64*32];               /* 0xff if pixel is set,    */
                                                /* 0x00 otherwise           */
static byte chip8_mem[4096];                    /* machine memory. program  */
                                                /* is loaded at 0x200       */

#define read_mem(a)     (chip8_mem[(a)&4095])
#define write_mem(a,v)  (chip8_mem[(a)&4095]=(v))

static byte chip8_running;                     /* Flag for End-of-Emulation */

#define get_reg_offset(opcode)          (chip8_regs.alg+(opcode>>8))
#define get_reg_value(opcode)           (*get_reg_offset(opcode))
#define get_reg_offset_2(opcode)        (chip8_regs.alg+((opcode>>4)&0x0f))
#define get_reg_value_2(opcode)         (*get_reg_offset_2(opcode))

typedef void (*opcode_fn) (word opcode);
typedef void (*math_fn) (byte *reg1,byte reg2);

static bool is_playing;

/* one frame of bitswapped mp3 data */
static unsigned char beep[]={255,
223, 28, 35,  0,192,210, 35,226, 72,188,242,  1,128,166, 16, 68,146,252,151, 19,
 10,180,245,127, 96,184,  3,184, 30,  0,118, 59,128,121,102,  6,212,  0, 97,  6,
 42, 65, 28,134,192,145, 57, 38,136, 73, 29, 38,132, 15, 21, 70, 91,185, 99,198,
 15,192, 83,  6, 33,129, 20,  6, 97, 33,  4,  6,245,128, 92,  6, 24,  0, 86,  6,
 56,129, 44, 24,224, 25, 13, 48, 50, 82,180, 11,251,106,249, 59, 24, 82,175,223,
252,119, 76,134,120,236,149,250,247,115,254,145,173,174,168,180,255,107,195, 89,
 24, 25, 48,131,192, 61, 48, 64, 10,176, 49, 64,  1,152, 50, 32,  8,140, 48, 16,
  5,129, 51,196,187, 41,177, 23,138, 70, 50,  8, 10,242, 48,192,  3,248,226,  0,
 20,100, 18, 96, 41, 96, 78,102,  7,201,122, 76,119, 20,137, 37,177, 15,132,224,
 20, 17,191, 67,147,187,116,211, 41,169, 63,172,182,186,217,155,111,140,104,254,
111,181,184,144, 17,148, 21,101,166,227,100, 86, 85, 85, 85}; 

/* callback to request more mp3 data */
void callback(unsigned char** start, int* size)
{
    *start = beep; /* give it the same frame again */
    *size = sizeof(beep);
}

/****************************************************************************/
/* Turn sound on                                                            */
/****************************************************************************/
static void chip8_sound_on (void) 
{
    if (!is_playing)
        rb->mp3_play_pause(true); /* kickoff audio */
}

/****************************************************************************/
/* Turn sound off                                                           */
/****************************************************************************/
static void chip8_sound_off (void) 
{ 
    if (!is_playing)
        rb->mp3_play_pause(false); /* pause audio */
}

static void op_call (word opcode)
{
    chip8_regs.sp--;
    write_mem (chip8_regs.sp,chip8_regs.pc&0xff);
    chip8_regs.sp--;
    write_mem (chip8_regs.sp,chip8_regs.pc>>8);
    chip8_regs.pc=opcode;
}

static void op_jmp (word opcode)
{
    chip8_regs.pc=opcode;
}

static void op_key (word opcode)
{
    byte key_value,cp_value;
    if ((opcode&0xff)==0x9e)
        cp_value=1;
    else if ((opcode&0xff)==0xa1)
        cp_value=0;
    else
        return;
    key_value=chip8_keys[get_reg_value(opcode)&0x0f];
    if (cp_value==key_value)
        chip8_regs.pc+=2;
}

static void op_skeq_const (word opcode)
{
    if (get_reg_value(opcode)==(opcode&0xff))
        chip8_regs.pc+=2;
}

static void op_skne_const (word opcode)
{
    if (get_reg_value(opcode)!=(opcode&0xff))
        chip8_regs.pc+=2;
}

static void op_skeq_reg (word opcode)
{
    if (get_reg_value(opcode)==get_reg_value_2(opcode))
        chip8_regs.pc+=2;
}

static void op_skne_reg (word opcode)
{
    if (get_reg_value(opcode)!=get_reg_value_2(opcode))
        chip8_regs.pc+=2;
}

static void op_mov_const (word opcode)
{
    *get_reg_offset(opcode)=opcode&0xff;
}

static void op_add_const (word opcode)
{
    *get_reg_offset(opcode)+=opcode&0xff;
}

static void op_mvi (word opcode)
{
    chip8_regs.i=opcode;
}

static void op_jmi (word opcode)
{
    chip8_regs.pc=opcode+chip8_regs.alg[0];
}

static void op_rand (word opcode)
{
    *get_reg_offset(opcode)=rb->rand()&(opcode&0xff);
}

static void math_or (byte *reg1,byte reg2)
{
    *reg1|=reg2;
}

static void math_mov (byte *reg1,byte reg2)
{
    *reg1=reg2;
}

static void math_nop (byte *reg1,byte reg2)
{
    (void)reg1;
    (void)reg2;
}

static void math_and (byte *reg1,byte reg2)
{
    *reg1&=reg2;
}

static void math_xor (byte *reg1,byte reg2)
{
 *reg1^=reg2;
}

static void math_add (byte *reg1,byte reg2)
{
    word tmp;
    tmp=*reg1+reg2;
    *reg1=(byte)tmp;
    chip8_regs.alg[15]=tmp>>8;
}

static void math_sub (byte *reg1,byte reg2)
{
    word tmp;
    tmp=*reg1-reg2;
    *reg1=(byte)tmp;
    chip8_regs.alg[15]=((byte)(tmp>>8))+1;
}

static void math_shr (byte *reg1,byte reg2)
{
    (void)reg2;
    chip8_regs.alg[15]=*reg1&1;
    *reg1>>=1;
}

static void math_shl (byte *reg1,byte reg2)
{
    (void)reg2;
    chip8_regs.alg[15]=*reg1>>7;
    *reg1<<=1;
}

static void math_rsb (byte *reg1,byte reg2)
{
    word tmp;
    tmp=reg2-*reg1;
    *reg1=(byte)tmp;
    chip8_regs.alg[15]=((byte)(tmp>>8))+1;
}

static void op_system (word opcode)
{
    switch ((byte)opcode)
    {
        case 0xe0:
            rb->memset (chip8_display,0,sizeof(chip8_display));
            break;
        case 0xee:
            chip8_regs.pc=read_mem(chip8_regs.sp)<<8;
            chip8_regs.sp++;
            chip8_regs.pc+=read_mem(chip8_regs.sp);
            chip8_regs.sp++;
            break;
    }
}

static void op_misc (word opcode)
{
    byte *reg,i,j;
    reg=get_reg_offset(opcode);
    switch ((byte)opcode)
    {
        case 0x07:
            *reg=chip8_regs.delay;
            break;
        case 0x0a:
            if (chip8_key_pressed)
                *reg=chip8_key_pressed-1;
            else
                chip8_regs.pc-=2;
            break;
        case 0x15:
            chip8_regs.delay=*reg;
            break;
        case 0x18:
            chip8_regs.sound=*reg;
            if (chip8_regs.sound)
                chip8_sound_on();
            break;
        case 0x1e:
            chip8_regs.i+=(*reg);
            break;
        case 0x29:
            chip8_regs.i=((word)(*reg&0x0f))*5;
            break;
        case 0x33:
            i=*reg;
            for (j=0;i>=100;i-=100)
                j++;
            write_mem (chip8_regs.i,j);
            for (j=0;i>=10;i-=10)
                j++;
            write_mem (chip8_regs.i+1,j);
            write_mem (chip8_regs.i+2,i);
            break;
        case 0x55:
            for (i=0,j=(opcode>>8)&0x0f; i<=j; ++i)
                write_mem(chip8_regs.i+i,chip8_regs.alg[i]);
            break;
        case 0x65:
            for (i=0,j=(opcode>>8)&0x0f; i<=j; ++i)
                chip8_regs.alg[i]=read_mem(chip8_regs.i+i);
            break;
    }
}

static void op_sprite (word opcode)
{
    byte *q;
    byte n,x,x2,y,collision;
    word p;
    x=get_reg_value(opcode)&63;
    y=get_reg_value_2(opcode)&31;
    p=chip8_regs.i;
    q=chip8_display+y*64;
    n=opcode&0x0f;
    if (n+y>32)
        n=32-y;
    for (collision=1;n;--n,q+=64)
    {
        for (y=read_mem(p++),x2=x;y;y<<=1,x2=(x2+1)&63)
            if (y&0x80) collision&=(q[x2]^=0xff);
    }
    chip8_regs.alg[15]=collision^1;
}

static math_fn math_opcodes[16]=
{
    math_mov,
    math_or,
    math_and,
    math_xor,
    math_add,
    math_sub,
    math_shr,
    math_rsb,
    math_nop,
    math_nop,
    math_nop,
    math_nop,
    math_nop,
    math_nop,
    math_shl,
    math_nop
};

static void op_math (word opcode)
{
    (*(math_opcodes[opcode&0x0f]))
        (get_reg_offset(opcode),get_reg_value_2(opcode));
}

static opcode_fn main_opcodes[16]=
{
    op_system,
    op_jmp,
    op_call,
    op_skeq_const,
    op_skne_const,
    op_skeq_reg,
    op_mov_const,
    op_add_const,
    op_math,
    op_skne_reg,
    op_mvi,
    op_jmi,
    op_rand,
    op_sprite,
    op_key,
    op_misc
};

/****************************************************************************/
/* Update the display                                                       */
/****************************************************************************/
static void chip8_update_display(void)
{
    int x,y,i;
    byte w;
    byte* row;

    for (y=0;y<=7;++y)                          /* 32 rows                  */
    {
        row = lcd_framebuf[y];
        for (x=0;x<64;++x)                      /* 64 columns               */
        {
            w = 0;
            for (i=0;i<=3;i++)
            {
                w = w >> 2;
                if (chip8_display[x+(y*4+i)*64] != 0)
                {
                    w += 128+64;
                }
            }
            *row++ = w;
        }
    }
    rb->lcd_blit(lcd_framebuf[0], 24, 0, 64, 8, 64);
}

static void chip8_keyboard(void)
{
  int button = rb->button_get(false);
  switch (button)
  {
    case BUTTON_OFF:                                      /* Abort Emulator */
        chip8_running = 0;
      break;

    case CHIP8_KEY2:                chip8_keys[2] = 1; break;
    case CHIP8_KEY2 | BUTTON_REL:   chip8_keys[2] = 0; break;
    case CHIP8_KEY4:                chip8_keys[4] = 1; break;
    case CHIP8_KEY4 | BUTTON_REL:   chip8_keys[4] = 0; break;
    case CHIP8_KEY6:                chip8_keys[6] = 1; break;
    case CHIP8_KEY6 | BUTTON_REL:   chip8_keys[6] = 0; break;
    case CHIP8_KEY8:                chip8_keys[8] = 1; break;
    case CHIP8_KEY8 | BUTTON_REL:   chip8_keys[8] = 0; break;
    case CHIP8_KEY5:                chip8_keys[5] = 1; break;
    case CHIP8_KEY5 | BUTTON_REL:   chip8_keys[5] = 0; break;
#ifdef CHIP8_KEY1
    case CHIP8_KEY1:                chip8_keys[1] = 1; break;
    case CHIP8_KEY1 | BUTTON_REL:   chip8_keys[1] = 0; break;
#endif
#ifdef CHIP8_KEY3
    case CHIP8_KEY3:                chip8_keys[3] = 1; break;
    case CHIP8_KEY3 | BUTTON_REL:   chip8_keys[3] = 0; break;
#endif
#ifdef CHIP8_KEY7
    case CHIP8_KEY7:                chip8_keys[7] = 1; break;
    case CHIP8_KEY7 | BUTTON_REL:   chip8_keys[7] = 0; break;
#endif
#ifdef CHIP8_KEY9
    case CHIP8_KEY9:                chip8_keys[9] = 1; break;
    case CHIP8_KEY9 | BUTTON_REL:   chip8_keys[9] = 0; break;
#endif

    default:
      if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
        chip8_running = 2; /* indicates stopped because of USB */
      break;
  }
}


/****************************************************************************/
/* Execute chip8_iperiod opcodes                                            */
/****************************************************************************/
static void chip8_execute(void)
{
    byte i;
    byte key_pressed=0;
    word opcode;
    for (i = chip8_iperiod ; i ;--i)
    {                                                 /* Fetch the opcode */
        opcode=(read_mem(chip8_regs.pc)<<8)+read_mem(chip8_regs.pc+1);

        chip8_regs.pc+=2;
        (*(main_opcodes[opcode>>12]))(opcode&0x0fff); /* Emulate this opcode */
    }
                                                      /* Update timers */
    if (chip8_regs.delay)
        --chip8_regs.delay;

    if (chip8_regs.sound)
        if (--chip8_regs.sound == 0)          /* Update the machine status */
            chip8_sound_off();       
                                               
    chip8_update_display();
    chip8_keyboard();
    rb->yield(); /* we should regulate the speed by timer query, sleep/yield */

    for (i=key_pressed=0;i<16;++i)              /* check if a key was first */
        if (chip8_keys[i])
            key_pressed=i+1;         /* pressed                  */
    if (key_pressed && key_pressed!=chip8_key_pressed)
        chip8_key_pressed=key_pressed;
    else
        chip8_key_pressed=0;
}

/****************************************************************************/
/* Reset the virtual chip8 machine                                          */
/****************************************************************************/
static void chip8_reset(void)
{
    static byte chip8_sprites[16*5]=
     {
         0xf9,0x99,0xf2,0x62,0x27,
         0xf1,0xf8,0xff,0x1f,0x1f,
         0x99,0xf1,0x1f,0x8f,0x1f,
         0xf8,0xf9,0xff,0x12,0x44,
         0xf9,0xf9,0xff,0x9f,0x1f,
         0xf9,0xf9,0x9e,0x9e,0x9e,
         0xf8,0x88,0xfe,0x99,0x9e,
         0xf8,0xf8,0xff,0x8f,0x88,
     };
    byte i;
    for (i=0;i<16*5;++i)
    {
        write_mem (i<<1,chip8_sprites[i]&0xf0);
        write_mem ((i<<1)+1,chip8_sprites[i]<<4);
    }
    rb->memset (chip8_regs.alg,0,sizeof(chip8_regs.alg));
    rb->memset (chip8_keys,0,sizeof(chip8_keys));
    chip8_key_pressed=0;
    rb->memset (chip8_display,0,sizeof(chip8_display));
    chip8_regs.delay=chip8_regs.sound=chip8_regs.i=0;
    chip8_regs.sp=0x1e0;
    chip8_regs.pc=0x200;
    chip8_sound_off ();
    chip8_running=1;
}

static bool chip8_init(char* file)
{
    int numread;
    int fd;

    fd = rb->open(file, O_RDONLY);
    if (fd==-1) return false;
    numread = rb->read(fd, chip8_mem+0x200, 4096-0x200);
    if (numread==-1) return false;

    rb->close(fd);
    return true;
}

bool chip8_run(char* file)
{
    int ok;

    ok = chip8_init(file);
    if (!ok) {
        rb->lcd_clear_display();
        rb->lcd_puts(0, 0, "Error");
        rb->lcd_update();
        rb->sleep(HZ);
        return false;
    }
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Chip8 Emulator ");
    rb->lcd_puts(0, 1, "    (c) by     ");
    rb->lcd_puts(0, 2, "Marcel de Kogel");
    rb->lcd_puts(0, 3, "    Archos:    ");
    rb->lcd_puts(0, 4, "   Blueloop    ");
    rb->lcd_puts(0, 5, "---------------");
    rb->lcd_puts(0, 6, "File OK...");
    rb->lcd_update();
    rb->sleep(HZ*1);
    rb->lcd_clear_display();
    rb->lcd_drawrect(23,0,66,64);
    rb->lcd_update();

    /* init sound */
    is_playing = rb->mp3_is_playing(); /* would we disturb playback? */
    if (!is_playing) /* no? then we can make sound */
    {   /* prepare */
        rb->mp3_play_data(beep, sizeof(beep), callback);
    }

    chip8_reset();
    while (chip8_running == 1) chip8_execute();

    if (!is_playing)
    {   /* stop it if we used audio */
        rb->mp3_play_stop(); // stop audio ISR
    }

    return true;
}


/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char* filename;

    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);
    rb = api; /* copy to global api pointer */

    if (parameter == NULL)
    {
        rb->lcd_puts(0, 0, "Play .ch8 file!");
        rb->lcd_update();
        rb->sleep(HZ);
        return PLUGIN_ERROR;
    }
    else
    {
        filename = (char*) parameter; 
    }

    /* now go ahead and have fun! */
    if (chip8_run(filename))
        if (chip8_running == 0)
            return PLUGIN_OK;
        else
            return PLUGIN_USB_CONNECTED;
    else
        return PLUGIN_ERROR;
}

#endif /* #ifndef SIMULATOR */
#endif /* #ifdef HAVE_LCD_BITMAP */
