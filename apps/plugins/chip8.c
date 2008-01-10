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

PLUGIN_HEADER

static struct plugin_api* rb; /* here is a global api struct pointer */

#define EXTERN static
#define STATIC static
#define memset rb->memset
#define memcpy rb->memcpy
#define printf DEBUGF
#define rand rb->rand
/* #define CHIP8_DEBUG */

#if (LCD_WIDTH >= 112) && (LCD_HEIGHT >= 64)
#define CHIP8_SUPER /* SCHIP even on the archos recorder (112x64) */
#endif

/****************************************************************************/
/**        (S)CHIP-8 core emulation, from Vision-8 + mods by Fdy           **/
/** The core is completely generic and can be used outside of Rockbox,     **/
/** thus the STATIC, EXTERN etc.  Please do not modify.                    **/
/****************************************************************************/
/** Vision8: CHIP8 emulator *************************************************/
/**                                                                        **/
/**                                CHIP8.h                                 **/
/**                                                                        **/
/** This file contains the portable CHIP8 emulation engine definitions     **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1997                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifndef __CHIP8_H
#define __CHIP8_H

#ifndef EXTERN
#define EXTERN extern
#endif
 
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

EXTERN struct chip8_regs_struct chip8_regs;

#ifdef CHIP8_SUPER
#define CHIP8_WIDTH 128
#define CHIP8_HEIGHT 64
EXTERN byte chip8_super;	/* != 0 if in SCHIP display mode */
#else
#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#endif

EXTERN byte chip8_iperiod;                      /* number of opcodes per    */
                                                /* timeslice (1/50sec.)     */
EXTERN byte chip8_keys[16];                     /* if 1, key is held down   */
EXTERN byte chip8_display[CHIP8_WIDTH*CHIP8_HEIGHT];/* 0xff if pixel is set,    */
                                                /* 0x00 otherwise           */
EXTERN byte chip8_mem[4096];                    /* machine memory. program  */
                                                /* is loaded at 0x200       */
EXTERN byte chip8_running;                      /* if 0, emulation stops    */

EXTERN void chip8_execute (void);                      /* execute chip8_iperiod    */
                                                /* opcodes                  */
EXTERN void chip8_reset (void);                        /* reset virtual machine    */
EXTERN void chip8 (void);                              /* start chip8 emulation    */

EXTERN void chip8_sound_on (void);                     /* turn sound on            */
EXTERN void chip8_sound_off (void);                    /* turn sound off           */
EXTERN void chip8_interrupt (void);                    /* update keyboard,         */
                                                /* display, etc.            */

#ifdef CHIP8_DEBUG
EXTERN byte chip8_trace;                        /* if 1, call debugger      */
                                                /* every opcode             */
EXTERN word chip8_trap;                         /* if pc==trap, set trace   */
                                                /* flag                     */
EXTERN void chip8_debug (word opcode,struct chip8_regs_struct *regs);
#endif

#endif          /* __CHIP8_H */

/** Vision8: CHIP8 emulator *************************************************/
/**                                                                        **/
/**                                CHIP8.c                                 **/
/**                                                                        **/
/** This file contains the portable CHIP8 emulation engine                 **/
/** SCHIP emulation (C) Frederic Devernay 2005                             **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1997                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/* you can
   #define STATIC static
   #define EXTERN static
   and include this file for single-object generation
*/

#ifndef STATIC
#include <stdlib.h>		/* for memset, etc. */
#include <string.h>
#define STATIC
#endif

#ifdef CHIP8_DEBUG
#include <stdio.h>
#define DBG_(_x)       ((void)(_x))
#else
#define DBG_(_x)       ((void)0)
#endif

STATIC struct chip8_regs_struct chip8_regs;

static byte chip8_key_pressed;
STATIC byte chip8_keys[16];                     /* if 1, key is held down   */
STATIC byte chip8_display[CHIP8_WIDTH*CHIP8_HEIGHT]; /* 0xff if pixel is set,    */
                                                /* 0x00 otherwise           */
#ifdef CHIP8_SUPER
STATIC byte chip8_super;	/* != 0 if in SCHIP display mode */
#endif
STATIC byte chip8_mem[4096];                    /* machine memory. program  */
                                                /* is loaded at 0x200       */

#define read_mem(a)     (chip8_mem[(a)&4095])
#define write_mem(a,v)  (chip8_mem[(a)&4095]=(v))

STATIC byte chip8_iperiod;

STATIC byte chip8_running;                     /* Flag for End-of-Emulation */

#define get_reg_offset(opcode)          (chip8_regs.alg+(opcode>>8))
#define get_reg_value(opcode)           (*get_reg_offset(opcode))
#define get_reg_offset_2(opcode)        (chip8_regs.alg+((opcode>>4)&0x0f))
#define get_reg_value_2(opcode)         (*get_reg_offset_2(opcode))

typedef void (*opcode_fn) (word opcode);
typedef void (*math_fn) (byte *reg1,byte reg2);



static void op_call (word opcode)
{
    chip8_regs.sp--;
    write_mem (chip8_regs.sp,chip8_regs.pc&0xff);
    chip8_regs.sp--;
    write_mem (chip8_regs.sp,chip8_regs.pc>>8);
    chip8_regs.pc=opcode;
#ifdef CHIP8_DEBUG
    if(chip8_regs.sp < 0x1c0)
	printf("warning: more than 16 subroutine calls, sp=%x\n", chip8_regs.sp);
#endif
}

static void op_jmp (word opcode)
{
    chip8_regs.pc=opcode;
}

static void op_key (word opcode)
{
#ifdef CHIP8_DEBUG
    static byte tested[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
    byte key, key_value,cp_value;
    if ((opcode&0xff)==0x9e) /* skp */
        cp_value=1;
    else if ((opcode&0xff)==0xa1) /* sknp */
        cp_value=0;
    else {
	DBG_(printf("unhandled key opcode 0x%x\n", opcode));
	return;
    }
    key = get_reg_value(opcode)&0x0f;
#ifdef CHIP8_DEBUG
    if (!tested[key]) {
	tested[key] = 1;
	DBG_(printf("testing key %d\n", key));
    }
#endif
    key_value=chip8_keys[key];
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
    *get_reg_offset(opcode)=rand()&(opcode&0xff);
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
    DBG_(printf("Warning: math nop!\n"));
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

#ifdef CHIP8_SUPER
/* SUPER: scroll down n lines (or half in CHIP8 mode) */
static void scroll_down(word opcode)
{
    int n = opcode & 0xf;
    byte *dst = chip8_display + CHIP8_WIDTH*CHIP8_HEIGHT -1;
    byte *src = dst - n*CHIP8_WIDTH;
    while(src >= chip8_display) {
	*dst-- = *src--;
    }
    while(dst >= chip8_display) {
	*dst-- = 0;
    }
}
/* SUPER: scroll 4 pixels left! */
static void scroll_left(void)
{
    byte *dst = chip8_display;
    byte *src = dst;
    byte *eol = chip8_display + CHIP8_WIDTH;
    byte *eoi = chip8_display + CHIP8_WIDTH*CHIP8_HEIGHT;
    while(eol <= eoi) {
	src+=4;
	while(src < eol) {
	    *dst++ = *src++;
	}
	*dst++ = 0;
	*dst++ = 0;
	*dst++ = 0;
	*dst++ = 0;
	eol += CHIP8_WIDTH;
    }
}
static void scroll_right(void)
{
    byte *dst = chip8_display + CHIP8_WIDTH*CHIP8_HEIGHT -1;
    byte *src = dst;
    byte *bol = chip8_display + CHIP8_WIDTH*(CHIP8_HEIGHT-1);
    while(bol >= chip8_display) {
	src-=4;
	while(src >= bol) {
	    *dst-- = *src--;
	}
	*dst-- = 0;
	*dst-- = 0;
	*dst-- = 0;
	*dst-- = 0;
	bol -= CHIP8_WIDTH;
    }
}
#endif

static void op_system (word opcode)
{
    switch ((byte)opcode)
    {
#ifdef CHIP8_SUPER
    case 0xfb:
	scroll_right();
	break;
    case 0xfc:
	scroll_left();
	break;	
    case 0xfd:
	DBG_(printf("SUPER: quit the emulator\n"));
	chip8_reset();
	break;	
    case 0xfe:
	DBG_(printf("SUPER: set CHIP-8 graphic mode\n"));
	memset (chip8_display,0,sizeof(chip8_display));
	chip8_super = 0;
	break;
    case 0xff:
	DBG_(printf("SUPER: set SCHIP graphic mode\n"));
	memset (chip8_display,0,sizeof(chip8_display));
	chip8_super = 1;
	break;	
#endif
        case 0xe0:
            memset (chip8_display,0,sizeof(chip8_display));
            break;
        case 0xee:
            chip8_regs.pc=read_mem(chip8_regs.sp)<<8;
            chip8_regs.sp++;
            chip8_regs.pc+=read_mem(chip8_regs.sp);
            chip8_regs.sp++;
            break;
    default:
#ifdef CHIP8_SUPER
	if ((opcode & 0xF0) == 0xC0)
	    scroll_down(opcode);
	else
#endif
	{
	    DBG_(printf("unhandled system opcode 0x%x\n", opcode));
	    chip8_running = 3;
	}
	break;
    }
}

static void op_misc (word opcode)
{
    byte *reg,i,j;
#ifdef CHIP8_DEBUG
    static byte firstwait = 1;
#endif
    reg=get_reg_offset(opcode);
    switch ((byte)opcode)
    {
        case 0x07:		/* gdelay */
            *reg=chip8_regs.delay;
            break;
        case 0x0a:		/* key */
#ifdef CHIP8_DEBUG
	    if(firstwait) {
		printf("waiting for key press\n");
		firstwait = 0;
	    }
#endif
	    if (chip8_key_pressed)
                *reg=chip8_key_pressed-1;
            else
                chip8_regs.pc-=2;
            break;
        case 0x15:		/* sdelay */
            chip8_regs.delay=*reg;
            break;
        case 0x18:		/* ssound */
            chip8_regs.sound=*reg;
            if (chip8_regs.sound)
                chip8_sound_on();
            break;
        case 0x1e:		/* adi */
            chip8_regs.i+=(*reg);
            break;
        case 0x29:		/* font */
            chip8_regs.i=((word)(*reg&0x0f))*5;
            break;
#ifdef CHIP8_SUPER
    case 0x30:			/* xfont */
            chip8_regs.i=((word)(*reg&0x0f))*10+0x50;
	    break;
#endif
        case 0x33:		/* bcd */
            i=*reg;
            for (j=0;i>=100;i-=100)
                j++;
            write_mem (chip8_regs.i,j);
            for (j=0;i>=10;i-=10)
                j++;
            write_mem (chip8_regs.i+1,j);
            write_mem (chip8_regs.i+2,i);
            break;
        case 0x55:		/* str */
            for (i=0,j=(opcode>>8)&0x0f; i<=j; ++i)
                write_mem(chip8_regs.i+i,chip8_regs.alg[i]);
            break;
        case 0x65:		/* ldr */
            for (i=0,j=(opcode>>8)&0x0f; i<=j; ++i)
                chip8_regs.alg[i]=read_mem(chip8_regs.i+i);
            break;
#ifdef CHIP8_SUPER
    case 0x75:
	DBG_(printf("SUPER: save V0..V%x (X<8) in the HP48 flags\n", (opcode>>8)&0x0f));
	break;
    case 0x85:
	DBG_(printf("SUPER: load V0..V%x (X<8) from the HP48 flags\n", (opcode>>8)&0x0f));
	break;
#endif
    default:
	DBG_(printf("unhandled misc opcode 0x%x\n", opcode));
	break;
    }
}

static void op_sprite (word opcode)
{
    byte *q;
    byte n,x,x2,y,collision;
    word p;
    x=get_reg_value(opcode);
    y=get_reg_value_2(opcode);
    p=chip8_regs.i;
    n=opcode&0x0f;
#ifdef CHIP8_SUPER
    if (chip8_super) {
	/*printf("SUPER: sprite(%x)\n", opcode);*/
	x &= 128-1;
	y &= 64-1;
	q=chip8_display+y*CHIP8_WIDTH;
	if(n == 0)
	{		/* 16x16 sprite */
	    n = 16;
	    if (n+y>64)
		n=64-y;
	    for (collision=1;n;--n,q+=CHIP8_WIDTH)
	    {
		/* first 8 bits */
		for (y=read_mem(p++),x2=x;y;y<<=1,x2=(x2+1)&(CHIP8_WIDTH-1))
		    if (y&0x80)
			collision&=(q[x2]^=0xff);
		x2=(x+8)&(CHIP8_WIDTH-1);
		/* last 8 bits */
		for (y=read_mem(p++);y;y<<=1,x2=(x2+1)&(CHIP8_WIDTH-1))
		    if (y&0x80)
			collision&=(q[x2]^=0xff);
	    }
	}
	else {
	    /* 8xn sprite */
	    if (n+y>64)
		n=64-y;
	    for (collision=1;n;--n,q+=CHIP8_WIDTH)
	    {
		for (y=read_mem(p++),x2=x;y;y<<=1,x2=(x2+1)&(CHIP8_WIDTH-1))
		    if (y&0x80)
			collision&=(q[x2]^=0xff);
	    }
	}
    }
    else {
	x &= 64-1;
	y &= 32-1;
	q=chip8_display+y*CHIP8_WIDTH*2;
	if(n == 0) 
	    n = 16;
	if (n+y>32)
	    n=32-y;
	for (collision=1;n;--n,q+=CHIP8_WIDTH*2)
	{
	    for (y=read_mem(p++),x2=x*2;y;y<<=1,x2=(x2+2)&(CHIP8_WIDTH-1))
		if (y&0x80) {
		    q[x2]^=0xff;
		    q[x2+1]^=0xff;
		    q[x2+CHIP8_WIDTH]^=0xff;
		    q[x2+CHIP8_WIDTH+1]^=0xff;
		    collision &= q[x2]|q[x2+1]|q[x2+CHIP8_WIDTH]|q[x2+CHIP8_WIDTH+1];
		}
	}
    }
#else
    x &= 64-1;
    y &= 32-1;
    q=chip8_display+y*CHIP8_WIDTH;
    if (n+y>32)
        n=32-y;
    for (collision=1;n;--n,q+=CHIP8_WIDTH)
    {
        for (y=read_mem(p++),x2=x;y;y<<=1,x2=(x2+1)&(CHIP8_WIDTH-1))
            if (y&0x80)
		collision&=(q[x2]^=0xff);
    }
#endif
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

#ifdef CHIP8_DEBUG
STATIC byte chip8_trace;
STATIC word chip8_trap;

/****************************************************************************/
/* This routine is called every opcode when chip8_trace==1. It prints the   */
/* current register contents and the opcode being executed                  */
/****************************************************************************/
STATIC void chip8_debug (word opcode,struct chip8_regs_struct *regs)
{
    int i;
    byte hextable[16] = "0123456789ABCDEF";
    byte v1[3] = "Vx\0";
    byte v2[3] = "Vx\0";
    v1[1] = hextable[(opcode>>8)&0x0f];
    v2[1] = hextable[(opcode>>8)&0x0f];
    printf ("PC=%04X: %04X - ",regs->pc,opcode);
    switch (opcode>>12) {
    case 0:
	if ((opcode&0xff0) == 0xc0) {
	    printf ("SCD  %01X       ; Scroll down n lines",opcode&0xf);
	}
	else switch (opcode&0xfff) {
	case 0xe0:
	    printf ("CLS          ; Clear screen");
	    break;
	case 0xee:
	    printf ("RET          ; Return from subroutine call");
	    break;
	case 0xfb:
	    printf("SCR           ; Scroll right");
	    break;
	case 0xfc:
	    printf("SCL           ; Scroll left");
	    break;
	case 0xfd:
	    printf("EXIT          ; Terminate the interpreter");
	    break;
	case 0xfe:
	    printf("LOW           ; Disable extended screen mode");
	    break;
	case 0xff:
	    printf("HIGH          ; Enable extended screen mode");
	    break;
	default:
	    printf ("SYS  %03X     ; Unknown system call",opcode&0xff);
	}
	break;
    case 1:
	printf ("JP   %03X     ; Jump to address",opcode&0xfff);
	break;
    case 2:
	printf ("CALL %03X     ; Call subroutine",opcode&0xfff);
	break;
    case 3:
	printf ("SE   %s,%02X   ; Skip if register == constant",v1,opcode&0xff);
	break;
    case 4:
	printf ("SNE  %s,%02X   ; Skip if register <> constant",v1,opcode&0xff);
	break;
    case 5:
	printf ("SE   %s,%s   ; Skip if register == register",v1,v2);
	break;
    case 6:
	printf ("LD   %s,%02X   ; Set VX = Byte",v1,opcode&0xff);
	break;
    case 7:
	printf ("ADD  %s,%02X   ; Set VX = VX + Byte",v1,opcode&0xff);
	break;
    case 8:
	switch (opcode&0x0f) {
	case 0:
	    printf ("LD   %s,%s   ; Set VX = VY, VF updates",v1,v2);
	    break;
	case 1:
	    printf ("OR   %s,%s   ; Set VX = VX | VY, VF updates",v1,v2);
	    break;
	case 2:
	    printf ("AND  %s,%s   ; Set VX = VX & VY, VF updates",v1,v2);
	    break;
	case 3:
	    printf ("XOR  %s,%s   ; Set VX = VX ^ VY, VF updates",v1,v2);
	    break;
	case 4:
	    printf ("ADD  %s,%s   ; Set VX = VX + VY, VF = carry",v1,v2);
	    break;
	case 5:
	    printf ("SUB  %s,%s   ; Set VX = VX - VY, VF = !borrow",v1,v2);
	    break;
	case 6:
	    printf ("SHR  %s,%s   ; Set VX = VX >> 1, VF = carry",v1,v2);
	    break;
	case 7:
	    printf ("SUBN %s,%s   ; Set VX = VY - VX, VF = !borrow",v1,v2);
	    break;
	case 14:
	    printf ("SHL  %s,%s   ; Set VX = VX << 1, VF = carry",v1,v2);
	    break;
	default:
	    printf ("Illegal opcode");
	}
	break;
    case 9:
	printf ("SNE  %s,%s   ; Skip next instruction iv VX!=VY",v1,v2);
	break;
    case 10:
	printf ("LD   I,%03X   ; Set I = Addr",opcode&0xfff);
	break;
    case 11:
	printf ("JP   V0,%03X  ; Jump to Addr + V0",opcode&0xfff);
	break;
    case 12:
	printf ("RND  %s,%02X   ; Set VX = random & Byte",v1,opcode&0xff);
	break;
    case 13:
	printf ("DRW  %s,%s,%X ; Draw n byte sprite stored at [i] at VX,VY. Set VF = collision",v1,v2,opcode&0x0f);
	break;
    case 14:
	switch (opcode&0xff) {
	case 0x9e:
	    printf ("SKP  %s      ; Skip next instruction if key VX down",v1);
	    break;
	case 0xa1:
	    printf ("SKNP %s      ; Skip next instruction if key VX up",v1);
	    break;
	default:
	    printf ("%04X        ; Illegal opcode", opcode);
	}
	break;
    case 15:
	switch (opcode&0xff) {
	case 0x07:
	    printf ("LD   %s,DT   ; Set VX = delaytimer",v1);
	    break;
	case 0x0a:
	    printf ("LD   %s,K    ; Set VX = key, wait for keypress",v1);
	    break;
	case 0x15:
	    printf ("LD   DT,%s   ; Set delaytimer = VX",v1);
	    break;
	case 0x18:
	    printf ("LD   ST,%s   ; Set soundtimer = VX",v1);
	    break;
	case 0x1e:
	    printf ("ADD  I,%s    ; Set I = I + VX",v1);
	    break;
	case 0x29:
	    printf ("LD  LF,%s    ; Point I to 5 byte numeric sprite for value in VX",v1);
	    break;
	case 0x30:
	    printf ("LD  HF,%s    ; Point I to 10 byte numeric sprite for value in VX",v1);
	    break;
	case 0x33:
	    printf ("LD   B,%s    ; Store BCD of VX in [I], [I+1], [I+2]",v1);
	    break;
	case 0x55:
	    printf ("LD   [I],%s  ; Store V0..VX in [I]..[I+X]",v1);
	    break;
	case 0x65:
	    printf ("LD   %s,[I]  ; Read V0..VX from [I]..[I+X]",v1);
	    break;
	case 0x75:
	    printf ("LD   R,%s    ; Store V0..VX in RPL user flags (X<=7)",v1);
	    break;
	case 0x85:
	    printf ("LD   %s,R    ; Read V0..VX from RPL user flags (X<=7)",v1);
	    break;
	default:
	    printf ("%04X        ; Illegal opcode", opcode);
	}
	break;
    }
    printf ("\n; Registers: ");
    for (i=0;i<16;++i) printf ("%02x ",(regs->alg[i])&0xff);
    printf ("\n; Index: %03x Stack:%03x Delay:%02x Sound:%02x\n",
	    regs->i&0xfff,regs->sp&0xfff,regs->delay&0xff,regs->sound&0xff);
}
#endif

/****************************************************************************/
/* Execute chip8_iperiod opcodes                                            */
/****************************************************************************/
STATIC void chip8_execute(void)
{
    byte i;
    byte key_pressed=0;
    word opcode;
    for (i = chip8_iperiod ; i ;--i)
    {
	/* Fetch the opcode */
        opcode=(read_mem(chip8_regs.pc)<<8)+read_mem(chip8_regs.pc+1);
#ifdef CHIP8_DEBUG
	/* Check if trap address has been reached */
	if ((chip8_regs.pc&4095)==chip8_trap)
	    chip8_trace=1;
	/* Call the debugger if chip8_trace!=0 */
	if (chip8_trace)
	    chip8_debug (opcode,&chip8_regs);
#endif
        chip8_regs.pc+=2;
        (*(main_opcodes[opcode>>12]))(opcode&0x0fff); /* Emulate this opcode */
    }
    /* Update timers */
    if (chip8_regs.delay)
        --chip8_regs.delay;
    if (chip8_regs.sound)
        if (--chip8_regs.sound == 0)
            chip8_sound_off();       

    /* Update the machine status */
    chip8_interrupt ();

    for (i=key_pressed=0;i<16;++i)              /* check if a key was first */
        if (chip8_keys[i])	                /* pressed                  */
            key_pressed=i+1;
    if (key_pressed && key_pressed!=chip8_key_pressed)
        chip8_key_pressed=key_pressed;
    else
        chip8_key_pressed=0;
}

/****************************************************************************/
/* Reset the virtual chip8 machine                                          */
/****************************************************************************/
STATIC void chip8_reset(void)
{
    static byte chip8_sprites[0x50]=
     {
         0xf9,0x99,0xf2,0x62,0x27,
         0xf1,0xf8,0xff,0x1f,0x1f,
         0x99,0xf1,0x1f,0x8f,0x1f,
         0xf8,0xf9,0xff,0x12,0x44,
         0xf9,0xf9,0xff,0x9f,0x1f,
         0xf9,0xf9,0x9e,0x9e,0x9e,
         0xf8,0x88,0xfe,0x99,0x9e,
         0xf8,0xf8,0xff,0x8f,0x88,
     }; /* 4x5 pixel hexadecimal character font patterns */
#ifdef CHIP8_SUPER
    static byte schip_sprites[10*10]=
     {
	 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, /* 0 */
	 0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, /* 1 */
	 0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, /* 2 */
	 0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, /* 3 */
	 0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, /* 4 */
	 0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, /* 5 */
	 0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, /* 6 */
	 0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, /* 7 */
	 0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, /* 8 */
	 0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, /* 9 */
     }; /* 8x10 pixel font patterns (only 10) */
#endif
    byte i;
    for (i=0;i<16*5;++i)
    {
        write_mem (i<<1,chip8_sprites[i]&0xf0);
        write_mem ((i<<1)+1,chip8_sprites[i]<<4);
    }
#ifdef CHIP8_SUPER
    /*
    for (i=0; i<100; i++)
        write_mem (i+0x50,schip_sprites[i]);
    */
    memcpy(chip8_mem+0x50, schip_sprites, 100);
    chip8_super = 0;
#endif
    memset (chip8_regs.alg,0,sizeof(chip8_regs.alg));
    memset (chip8_keys,0,sizeof(chip8_keys));
    chip8_key_pressed=0;
    memset (chip8_display,0,sizeof(chip8_display));
    chip8_regs.delay=chip8_regs.sound=chip8_regs.i=0;
    chip8_regs.sp=0x1e0;
    chip8_regs.pc=0x200;
    chip8_sound_off ();
    chip8_running=1;
#ifdef CHIP8_DEBUG
    chip8_trace=0;
#endif
}


/****************************************************************************/
/* Start CHIP8 emulation                                                    */
/****************************************************************************/
STATIC void chip8 (void)
{
 chip8_reset ();
 while (chip8_running==1) chip8_execute ();
}

/****************************************************************************/
/** END OF (S)CHIP-8 core emulation, from Vision-8 + mods by Fdy           **/
/****************************************************************************/

/* size of the displayed area */
#if (CHIP8_WIDTH == 128) && (LCD_WIDTH < 128)
#define CHIP8_LCDWIDTH 112
#else
#define CHIP8_LCDWIDTH CHIP8_WIDTH
#endif

#if (LCD_WIDTH > CHIP8_LCDWIDTH)
#define CHIP8_X ((LCD_WIDTH - CHIP8_LCDWIDTH)/2)
#else
#define CHIP8_X 0
#endif
#if (LCD_HEIGHT > CHIP8_HEIGHT)
#define CHIP8_Y (((LCD_HEIGHT - CHIP8_HEIGHT)>>4)<<3)
#else
#define CHIP8_Y 0
#endif

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD /* only 9 out of 16 chip8 buttons */
#define CHIP8_OFF  BUTTON_OFF
#define CHIP8_KEY1 BUTTON_F1
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY3 BUTTON_F3
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_PLAY
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY7 BUTTON_F2
#define CHIP8_KEY8 BUTTON_DOWN
#define CHIP8_KEY9 BUTTON_ON

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD /* only 9 out of 16 chip8 buttons */
#define CHIP8_OFF  BUTTON_OFF
#define CHIP8_KEY1 BUTTON_F1
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY3 BUTTON_F3
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_SELECT
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY7 BUTTON_F2
#define CHIP8_KEY8 BUTTON_DOWN
#define CHIP8_KEY9 BUTTON_ON

#elif CONFIG_KEYPAD == ONDIO_PAD /* even more limited */
#define CHIP8_OFF  BUTTON_OFF
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_MENU
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CHIP8_OFF  BUTTON_OFF
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_SELECT
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_DOWN

#define CHIP8_RC_OFF BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define CHIP8_OFF  BUTTON_MENU
#define CHIP8_KEY2 BUTTON_SCROLL_BACK
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_PLAY
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_SCROLL_FWD

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define CHIP8_OFF  BUTTON_POWER
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_SELECT
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_DOWN

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define CHIP8_OFF  BUTTON_POWER
#define CHIP8_KEY1 BUTTON_MENU
#define CHIP8_KEY2 BUTTON_UP
#define CHIP8_KEY3 BUTTON_VOL_DOWN
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_SELECT
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY7 BUTTON_VOL_UP
#define CHIP8_KEY8 BUTTON_DOWN
#define CHIP8_KEY9 BUTTON_A

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define CHIP8_OFF  BUTTON_POWER
#define CHIP8_KEY2 BUTTON_SCROLL_BACK
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_SELECT
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define CHIP8_OFF  BUTTON_POWER
#define CHIP8_KEY2 BUTTON_VOL_UP
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_SELECT
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define CHIP8_OFF  BUTTON_POWER
#define CHIP8_KEY2 BUTTON_SCROLL_UP
#define CHIP8_KEY4 BUTTON_LEFT
#define CHIP8_KEY5 BUTTON_PLAY
#define CHIP8_KEY6 BUTTON_RIGHT
#define CHIP8_KEY8 BUTTON_SCROLL_DOWN

#endif

static byte chip8_virtual_keys[16];
static byte chip8_keymap[16];

static unsigned long starttimer; /* Timer value at the beginning */
static unsigned long cycles; /* Number of update cycles (50Hz) */

#ifndef SIMULATOR
static bool is_playing;
#endif

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
void callback(unsigned char** start, size_t* size)
{
    *start = beep; /* give it the same frame again */
    *size = sizeof(beep);
}

/****************************************************************************/
/* Turn sound on                                                            */
/****************************************************************************/
static void chip8_sound_on (void) 
{
#ifndef SIMULATOR
    if (!is_playing)
        rb->mp3_play_pause(true); /* kickoff audio */
#endif
}

/****************************************************************************/
/* Turn sound off                                                           */
/****************************************************************************/
static void chip8_sound_off (void) 
{ 
#ifndef SIMULATOR
    if (!is_playing)
        rb->mp3_play_pause(false); /* pause audio */
#endif
}

/****************************************************************************/
/* Update the display                                                       */
/****************************************************************************/
static void chip8_update_display(void)
{
    int x, lcd_x, y, i;
    byte w;
    byte* row;
    /* frame buffer in hardware fomat */
    unsigned char lcd_framebuf[CHIP8_HEIGHT/8][CHIP8_LCDWIDTH];

    for (y=0;y<CHIP8_HEIGHT/8;++y)
    {
        row = lcd_framebuf[y];
        for (x=0, lcd_x=0;x<CHIP8_WIDTH;++x,++lcd_x)
        {
            w = 0;
            for (i=0;i<8;i++)
            {
                w = w >> 1;
                if (chip8_display[x+(y*8+i)*CHIP8_WIDTH] != 0)
                {
                    w += 128;
                }
            }
#if (CHIP8_LCDWIDTH < 128)              /* LCD_WIDTH between 112 and 127 */
            if (x%8 == 1) {
                row[-1] |= w;   /* overlay on */
            }
            else
#endif
                *row++ = w;
        }
    }
#if defined(SIMULATOR) || (LCD_DEPTH > 1)
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_mono_bitmap(lcd_framebuf[0], CHIP8_X, CHIP8_Y, CHIP8_LCDWIDTH, CHIP8_HEIGHT);
    rb->lcd_update();
#else
    rb->lcd_blit(lcd_framebuf[0], CHIP8_X, CHIP8_Y>>3, CHIP8_LCDWIDTH, CHIP8_HEIGHT>>3
                 , CHIP8_LCDWIDTH);
#endif
}

static void chip8_keyboard(void)
{
    int i;
    int button = rb->button_get(false);
    switch (button)
    {
#ifdef CHIP8_RC_OFF
    case CHIP8_RC_OFF:
#endif
    case CHIP8_OFF:                                      /* Abort Emulator */
        chip8_running = 0;
        break;

    case CHIP8_KEY2:                chip8_virtual_keys[2] = 1; break;
    case CHIP8_KEY2 | BUTTON_REL:   chip8_virtual_keys[2] = 0; break;
    case CHIP8_KEY4:                chip8_virtual_keys[4] = 1; break;
    case CHIP8_KEY4 | BUTTON_REL:   chip8_virtual_keys[4] = 0; break;
    case CHIP8_KEY6:                chip8_virtual_keys[6] = 1; break;
    case CHIP8_KEY6 | BUTTON_REL:   chip8_virtual_keys[6] = 0; break;
    case CHIP8_KEY8:                chip8_virtual_keys[8] = 1; break;
    case CHIP8_KEY8 | BUTTON_REL:   chip8_virtual_keys[8] = 0; break;
    case CHIP8_KEY5:                chip8_virtual_keys[5] = 1; break;
    case CHIP8_KEY5 | BUTTON_REL:   chip8_virtual_keys[5] = 0; break;
#ifdef CHIP8_KEY1
    case CHIP8_KEY1:                chip8_virtual_keys[1] = 1; break;
    case CHIP8_KEY1 | BUTTON_REL:   chip8_virtual_keys[1] = 0; break;
#endif
#ifdef CHIP8_KEY3
    case CHIP8_KEY3:                chip8_virtual_keys[3] = 1; break;
    case CHIP8_KEY3 | BUTTON_REL:   chip8_virtual_keys[3] = 0; break;
#endif
#ifdef CHIP8_KEY7
    case CHIP8_KEY7:                chip8_virtual_keys[7] = 1; break;
    case CHIP8_KEY7 | BUTTON_REL:   chip8_virtual_keys[7] = 0; break;
#endif
#ifdef CHIP8_KEY9
    case CHIP8_KEY9:                chip8_virtual_keys[9] = 1; break;
    case CHIP8_KEY9 | BUTTON_REL:   chip8_virtual_keys[9] = 0; break;
#endif

    default:
        if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            chip8_running = 2; /* indicates stopped because of USB */
        break;
    }
    for(i=0; i<16; i++) {
        chip8_keys[i] = chip8_virtual_keys[chip8_keymap[i]];
    }
}

/****************************************************************************/
/* Update keyboard and display, sync emulation with VDP clock               */
/****************************************************************************/
static void chip8_interrupt (void)
{
    unsigned long newtimer;
    unsigned long timer, runtime;

    chip8_update_display();
    chip8_keyboard();
    cycles ++;
    runtime = cycles * HZ / 50;
    timer = starttimer + runtime;
    newtimer = *rb->current_tick;
    if (TIME_AFTER(timer, newtimer))
    {
	rb->sleep(timer - newtimer);
    }
    else
    {
	rb->yield();
    }
    starttimer = newtimer - runtime;
}

static bool chip8_init(char* file)
{
    int numread;
    int fd;
    int len;
    int i;

    fd = rb->open(file, O_RDONLY);
    if (fd==-1) {
	rb->lcd_puts(0, 6, "File Error.");
	return false;
    }
    numread = rb->read(fd, chip8_mem+0x200, 4096-0x200);
    if (numread==-1) {
	rb->lcd_puts(0, 6, "I/O Error.");
	return false;
    }

    rb->close(fd);
    /* is there a c8k file (chip8 keys) ? */
    for(i=0; i<16; i++) {
	chip8_virtual_keys[i] = 0;
	chip8_keymap[i] = i;
    }
    len = rb->strlen(file);
    file[len-2] = '8';
    file[len-1] = 'k';
    fd = rb->open(file, O_RDONLY);
    if (fd!=-1) {
	rb->lcd_puts(0, 6, "File&Keymap OK.");
	numread = rb->read(fd, chip8_keymap, 16);
	rb->close(fd);
    }
    else
    {
	rb->lcd_puts(0, 6, "File OK.");
    }
    for(i=0; i<16; i++) {
	if (chip8_keymap[i] >= '0'
	    && chip8_keymap[i] <= '9')
	    chip8_keymap[i] -= '0';
	else if (chip8_keymap[i] >= 'A'
	    && chip8_keymap[i] <= 'F')
	    chip8_keymap[i] -= 'A' - 10;
	else if (chip8_keymap[i] >= 'a'
	    && chip8_keymap[i] <= 'f')
	    chip8_keymap[i] -= 'a' - 10;
	else
	    chip8_keymap[i] %= 16;
    }
    return true;
}

bool chip8_run(char* file)
{
    int ok;

    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "SChip8 Emulator");
    rb->lcd_puts(0, 1, "    (c) by     ");
    rb->lcd_puts(0, 2, "Marcel de Kogel");
    rb->lcd_puts(0, 3, "    Rockbox:   ");
    rb->lcd_puts(0, 4, "  Blueloop/Fdy ");
    rb->lcd_puts(0, 5, "---------------");
    ok = chip8_init(file);
    rb->lcd_update();
    rb->sleep(HZ*1);
    if (!ok)
        return false;
    rb->lcd_clear_display();
#if (CHIP8_X > 0) && (CHIP8_Y > 0)
    rb->lcd_drawrect(CHIP8_X-1,CHIP8_Y-1,CHIP8_LCDWIDTH+2,CHIP8_HEIGHT+2);
#endif
    rb->lcd_update();

#ifndef SIMULATOR
    /* init sound */
    is_playing = rb->mp3_is_playing(); /* would we disturb playback? */
    if (!is_playing) /* no? then we can make sound */
    {   /* prepare */
        rb->mp3_play_data(beep, sizeof(beep), callback);
    }
#endif
    starttimer = *rb->current_tick;

    chip8_iperiod=15;
    cycles = 0;
    chip8();

    if (!ok) {
        rb->splash(HZ, "Error");
        return false;
    }

#ifndef SIMULATOR
    if (!is_playing)
    {   /* stop it if we used audio */
        rb->mp3_play_stop(); /* Stop audio playback */
    }
#endif

    if (chip8_running == 3) {
	/* unsupported instruction */
        rb->splash(HZ, "Error: Unsupported"
#ifndef CHIP8_SUPER
	" CHIP-8 instruction. (maybe S-CHIP)"
#else
        " (S)CHIP-8 instruction."
#endif
            );
        return false;
    }

    return true;
}


/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char* filename;

    rb = api; /* copy to global api pointer */

    if (parameter == NULL)
    {
        rb->splash(HZ, "Play a .ch8 file!");
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
#endif /* #ifdef HAVE_LCD_BITMAP */
