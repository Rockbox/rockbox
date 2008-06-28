/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/* sh-stub.c -- debugging stub for the Hitachi-SH.

 NOTE!! This code has to be compiled with optimization, otherwise the 
 function inlining which generates the exception handlers won't work.

*/

/*   This is originally based on an m68k software stub written by Glenn
     Engel at HP, but has changed quite a bit. 

     Modifications for the SH by Ben Lee and Steve Chamberlain

     Even more modifications for GCC 3.0 and The Rockbox by Linus
     Nielsen Feltzing
*/

/****************************************************************************

		THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/


/* Remote communication protocol.

   A debug packet whose contents are <data>
   is encapsulated for transmission in the form:

	$ <data> # CSUM1 CSUM2

	<data> must be ASCII alphanumeric and cannot include characters
	'$' or '#'.  If <data> starts with two characters followed by
	':', then the existing stubs interpret this as a sequence number.

	CSUM1 and CSUM2 are ascii hex representation of an 8-bit 
	checksum of <data>, the most significant nibble is sent first.
	the hex digits 0-9,a-f are used.

   Receiver responds with:

	+	- if CSUM is correct and ready for next packet
	-	- if CSUM is incorrect

   <data> is as follows:
   All values are encoded in ascii hex digits.

	Request		Packet

	read registers  g
	reply		XX....X		Each byte of register data
					is described by two hex digits.
					Registers are in the internal order
					for GDB, and the bytes in a register
					are in the same order the machine uses.
			or ENN		for an error.

	write regs	GXX..XX		Each byte of register data
					is described by two hex digits.
	reply		OK		for success
			ENN		for an error

        write reg	Pn...=r...	Write register n... with value r...,
					which contains two hex digits for each
					byte in the register (target byte
					order).
	reply		OK		for success
			ENN		for an error
	(not supported by all stubs).

	read mem	mAA..AA,LLLL	AA..AA is address, LLLL is length.
	reply		XX..XX		XX..XX is mem contents
					Can be fewer bytes than requested
					if able to read only part of the data.
			or ENN		NN is errno

	write mem	MAA..AA,LLLL:XX..XX
					AA..AA is address,
					LLLL is number of bytes,
					XX..XX is data
	reply		OK		for success
			ENN		for an error (this includes the case
					where only part of the data was
					written).

	cont		cAA..AA		AA..AA is address to resume
					If AA..AA is omitted,
					resume at same address.

	step		sAA..AA		AA..AA is address to resume
					If AA..AA is omitted,
					resume at same address.

	last signal     ?               Reply the current reason for stopping.
                                        This is the same reply as is generated
					for step or cont : SAA where AA is the
					signal number.

	There is no immediate reply to step or cont.
	The reply comes when the machine stops.
	It is		SAA		AA is the "signal number"

	or...		TAAn...:r...;n:r...;n...:r...;
					AA = signal number
					n... = register number
					r... = register contents
	or...		WAA		The process exited, and AA is
					the exit status.  This is only
					applicable for certains sorts of
					targets.
	kill request	k

	toggle debug	d		toggle debug flag (see 386 & 68k stubs)
	reset		r		reset -- see sparc stub.
	reserved	<other>		On other requests, the stub should
					ignore the request and send an empty
					response ($#<checksum>).  This way
					we can extend the protocol and GDB
					can tell whether the stub it is
					talking to uses the old or the new.
	search		tAA:PP,MM	Search backwards starting at address
					AA for a match with pattern PP and
					mask MM.  PP and MM are 4 bytes.
					Not supported by all stubs.

	general query	qXXXX		Request info about XXXX.
	general set	QXXXX=yyyy	Set value of XXXX to yyyy.
	query sect offs	qOffsets	Get section offsets.  Reply is
					Text=xxx;Data=yyy;Bss=zzz
	console output	Otext		Send text to stdout.  Only comes from
					remote target.

	Responses can be run-length encoded to save space.  A '*' means that
	the next character is an ASCII encoding giving a repeat count which
	stands for that many repititions of the character preceding the '*'.
	The encoding is n+29, yielding a printable character where n >=3 
	(which is where rle starts to win).  Don't use an n > 126. 

	So 
	"0* " means the same as "0000".  */

#include "sh7034.h"
#include <string.h>

typedef int jmp_buf[20];

void longjmp(jmp_buf __jmpb, int __retval);
int setjmp(jmp_buf __jmpb);

/* We need to undefine this from the sh7034.h file */
#undef GBR

/* Hitachi SH architecture instruction encoding masks */

#define COND_BR_MASK   0xff00
#define UCOND_DBR_MASK 0xe000
#define UCOND_RBR_MASK 0xf0df
#define TRAPA_MASK     0xff00

#define COND_DISP      0x00ff
#define UCOND_DISP     0x0fff
#define UCOND_REG      0x0f00

/* Hitachi SH instruction opcodes */

#define BF_INSTR       0x8b00
#define BT_INSTR       0x8900
#define BRA_INSTR      0xa000
#define BSR_INSTR      0xb000
#define JMP_INSTR      0x402b
#define JSR_INSTR      0x400b
#define RTS_INSTR      0x000b
#define RTE_INSTR      0x002b
#define TRAPA_INSTR    0xc300
#define SSTEP_INSTR    0xc37f

/* Hitachi SH processor register masks */

#define T_BIT_MASK     0x0001

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX 1024

/*
 * Number of bytes for registers
 */
#define NUMREGBYTES 112		/* 92 */

/*
 * Forward declarations
 */

static int hex (char);
static char *mem2hex (char *mem, char *buf, int count);
static char *hex2mem (char *buf, char *mem, int count);
static int hex2int (char **ptr, int *intValue);
static unsigned char *getpacket (void);
static void putpacket (register char *buffer);
static int computeSignal (int exceptionVector);
void handle_buserror (void);
void handle_exception (int exceptionVector);
void init_serial(void);

void serial_putc (char ch);
char serial_getc (void);

/* These are in the file but in asm statements so the compiler can't see them */
void catch_exception_4 (void);
void catch_exception_5 (void);
void catch_exception_6 (void);
void catch_exception_7 (void);
void catch_exception_8 (void);
void catch_exception_9 (void);
void catch_exception_10 (void);
void catch_exception_11 (void);
void catch_exception_12 (void);
void catch_exception_13 (void);
void catch_exception_14 (void);
void catch_exception_15 (void);
void catch_exception_16 (void);
void catch_exception_17 (void);
void catch_exception_18 (void);
void catch_exception_19 (void);
void catch_exception_20 (void);
void catch_exception_21 (void);
void catch_exception_22 (void);
void catch_exception_23 (void);
void catch_exception_24 (void);
void catch_exception_25 (void);
void catch_exception_26 (void);
void catch_exception_27 (void);
void catch_exception_28 (void);
void catch_exception_29 (void);
void catch_exception_30 (void);
void catch_exception_31 (void);
void catch_exception_32 (void);
void catch_exception_33 (void);
void catch_exception_34 (void);
void catch_exception_35 (void);
void catch_exception_36 (void);
void catch_exception_37 (void);
void catch_exception_38 (void);
void catch_exception_39 (void);
void catch_exception_40 (void);
void catch_exception_41 (void);
void catch_exception_42 (void);
void catch_exception_43 (void);
void catch_exception_44 (void);
void catch_exception_45 (void);
void catch_exception_46 (void);
void catch_exception_47 (void);
void catch_exception_48 (void);
void catch_exception_49 (void);
void catch_exception_50 (void);
void catch_exception_51 (void);
void catch_exception_52 (void);
void catch_exception_53 (void);
void catch_exception_54 (void);
void catch_exception_55 (void);
void catch_exception_56 (void);
void catch_exception_57 (void);
void catch_exception_58 (void);
void catch_exception_59 (void);
void catch_exception_60 (void);
void catch_exception_61 (void);
void catch_exception_62 (void);
void catch_exception_63 (void);
void catch_exception_64 (void);
void catch_exception_65 (void);
void catch_exception_66 (void);
void catch_exception_67 (void);
void catch_exception_68 (void);
void catch_exception_69 (void);
void catch_exception_70 (void);
void catch_exception_71 (void);
void catch_exception_72 (void);
void catch_exception_73 (void);
void catch_exception_74 (void);
void catch_exception_75 (void);
void catch_exception_76 (void);
void catch_exception_77 (void);
void catch_exception_78 (void);
void catch_exception_79 (void);
void catch_exception_80 (void);
void catch_exception_81 (void);
void catch_exception_82 (void);
void catch_exception_83 (void);
void catch_exception_84 (void);
void catch_exception_85 (void);
void catch_exception_86 (void);
void catch_exception_87 (void);
void catch_exception_88 (void);
void catch_exception_89 (void);
void catch_exception_90 (void);
void catch_exception_91 (void);
void catch_exception_92 (void);
void catch_exception_93 (void);
void catch_exception_94 (void);
void catch_exception_95 (void);
void catch_exception_96 (void);
void catch_exception_97 (void);
void catch_exception_98 (void);
void catch_exception_99 (void);
void catch_exception_100 (void);
void catch_exception_101 (void);
void catch_exception_102 (void);
void catch_exception_103 (void);
void catch_exception_104 (void);
void catch_exception_105 (void);
void catch_exception_106 (void);
void catch_exception_107 (void);
void catch_exception_108 (void);
void catch_exception_109 (void);
void catch_exception_110 (void);
void catch_exception_111 (void);
void catch_exception_112 (void);
void catch_exception_113 (void);
void catch_exception_114 (void);
void catch_exception_115 (void);
void catch_exception_116 (void);
void catch_exception_117 (void);
void catch_exception_118 (void);
void catch_exception_119 (void);
void catch_exception_120 (void);
void catch_exception_121 (void);
void catch_exception_122 (void);
void catch_exception_123 (void);
void catch_exception_124 (void);
void catch_exception_125 (void);
void catch_exception_126 (void);
void catch_exception_127 (void);

void breakpoint (void);


//#define stub_stack_size 2*1024

//int stub_stack[stub_stack_size] __attribute__ ((section (".stack"))) = {0};

extern int stub_stack[];

void INIT (void);
void start (void);

#define CPU_BUS_ERROR_VEC  9
#define DMA_BUS_ERROR_VEC 10
#define NMI_VEC           11
#define INVALID_INSN_VEC   4
#define INVALID_SLOT_VEC   6
#define TRAP_VEC          32
#define IO_VEC            33
#define USER_VEC         127

char in_nmi;   /* Set when handling an NMI, so we don't reenter */
int dofault;  /* Non zero, bus errors will raise exception */

int *stub_sp;

/* debug > 0 prints ill-formed commands in valid packets & checksum errors */
int remote_debug;

/* jump buffer used for setjmp/longjmp */
jmp_buf remcomEnv;

enum regnames
{
    R0, R1, R2, R3, R4, R5, R6, R7,
    R8, R9, R10, R11, R12, R13, R14,
    R15, PC, PR, GBR, VBR, MACH, MACL, SR,
    TICKS, STALLS, CYCLES, INSTS, PLR
};

typedef struct
{
    short *memAddr;
    short oldInstr;
}
stepData;

int registers[NUMREGBYTES / 4];
stepData instrBuffer;
char stepped;
static const char hexchars[] = "0123456789abcdef";
static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

#define ATA_NSECTOR     (*((volatile unsigned char*)0x06100102))
#define ATA_COMMAND     (*((volatile unsigned char*)0x06100107))

/* You may need to change this depending on your ATA I/O address
** 0x200 - 0x06200206
** 0x300 - 0x06200306
*/
#define ATA_CONTROL     (*((volatile unsigned char*)0x06200206))
#define ATA_ALT_STATUS  ATA_CONTROL

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40

#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2

void ata_wait_for_bsy(void)
{
    while (ATA_ALT_STATUS & STATUS_BSY);
}

int ata_wait_for_rdy(void)
{
    ata_wait_for_bsy();
    return ATA_ALT_STATUS & STATUS_RDY;
}

int ata_spindown(int time)
{
	/* Port A setup */
	PAIOR |= 0x0280; /* output for ATA reset, IDE enable */
	PADR |= 0x0200; /* release ATA reset */
 	PACR2 &= 0xBFFF; /* GPIO function for PA7 (IDE enable) */

    /* activate ATA */
    PADR &= ~0x80;

    if(!ata_wait_for_rdy())
        return -1;

    if ( time == -1 ) {
        ATA_COMMAND = CMD_STANDBY_IMMEDIATE;
    }
    else {
        if (time > 255)
            return -1;
        ATA_NSECTOR = time & 0xff;
        ATA_COMMAND = CMD_STANDBY;
    }

    if (!ata_wait_for_rdy())
        return -1;

    return 0;
}

void blink(void)
{
    while(1)
    {
        int i;
        PBDR ^= 0x40; /* toggle PB6 */
        for(i = 0;i < 500000;i++)
        {
        }
    }
}

char highhex(int  x)
{
    return hexchars[(x >> 4) & 0xf];
}

char lowhex(int  x)
{
    return hexchars[x & 0xf];
}

/*
 * Assembly macros
 */

#define BREAKPOINT()   asm("trapa	#0x20"::);


/*
 * Routines to handle hex data
 */

static int hex (char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);
    return (-1);
}

/* convert the memory, pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static char *mem2hex (char *mem, char *buf, int count)
{
    int i;
    int ch;
    for (i = 0; i < count; i++)
    {
        ch = *mem++;
        *buf++ = highhex (ch);
        *buf++ = lowhex (ch);
    }
    *buf = 0;
    return (buf);
}

/* convert the hex array pointed to by buf into binary, to be placed in mem */
/* return a pointer to the character after the last byte written */
static char *hex2mem (char *buf, char *mem, int count)
{
    int i;
    unsigned char ch;
    for (i = 0; i < count; i++)
    {
        ch = hex (*buf++) << 4;
        ch = ch + hex (*buf++);
        *mem++ = ch;
    }
    return (mem);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
static int hex2int (char **ptr, int *intValue)
{
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex (**ptr);
        if (hexValue >= 0)
        {
            *intValue = (*intValue << 4) | hexValue;
            numChars++;
        }
        else
            break;

        (*ptr)++;
    }

    return (numChars);
}

/*
 * Routines to get and put packets
 */

/* scan for the sequence $<data>#<checksum>     */

unsigned char *getpacket (void)
{
    unsigned char *buffer = &remcomInBuffer[0];
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while (1)
    {
        /* wait around for the start character, ignore all other characters */
        while ((ch = serial_getc ()) != '$')
            ;

      retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* now, read until a # or end of buffer is found */
        while (count < BUFMAX)
        {
            ch = serial_getc ();
            if (ch == '$')
                goto retry;
            if (ch == '#')
                break;
            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }
        buffer[count] = 0;

        if (ch == '#')
        {
            ch = serial_getc ();
            xmitcsum = hex (ch) << 4;
            ch = serial_getc ();
            xmitcsum += hex (ch);

            if (checksum != xmitcsum)
            {
                serial_putc ('-');	/* failed checksum */
            }
            else
            {
                serial_putc ('+');	/* successful transfer */

                /* if a sequence char is present, reply the sequence ID */
                if (buffer[2] == ':')
                {
                    serial_putc (buffer[0]);
                    serial_putc (buffer[1]);

                    return &buffer[3];
                }

                return &buffer[0];
            }
        }
    }
}


/* send the packet in buffer. */

static void putpacket (register char *buffer)
{
    register  int checksum;

    /*  $<packet info>#<checksum>. */
    do
    {
        char *src = buffer;
        serial_putc ('$');
        checksum = 0;

        while (*src)
        {
            int runlen;

            /* Do run length encoding */
            for (runlen = 0; runlen < 100; runlen ++) 
            {
                if (src[0] != src[runlen] || runlen == 99) 
                {
                    if (runlen > 3) 
                    {
                        int encode;
                        /* Got a useful amount */
                        serial_putc (*src);
                        checksum += *src;
                        serial_putc ('*');
                        checksum += '*';
                        checksum += (encode = runlen + ' ' - 4);
                        serial_putc (encode);
                        src += runlen;
                    }
                    else
                    {
                        serial_putc (*src);
                        checksum += *src;
                        src++;
                    }
                    break;
                }
            }
        }


        serial_putc ('#');
        serial_putc (highhex(checksum));
        serial_putc (lowhex(checksum));
    }
    while  (serial_getc() != '+');
}


/* a bus error has occurred, perform a longjmp
   to return execution and allow handling of the error */

void handle_buserror (void)
{
    longjmp (remcomEnv, 1);
}

#define	SIGINT	2	/* interrupt */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGBUS	10	/* bus error */

/*
 * this function takes the SH-1 exception number and attempts to
 * translate this number into a unix compatible signal value
 */
static int computeSignal (int exceptionVector)
{
    int sigval;
    switch (exceptionVector)
    {
        case INVALID_INSN_VEC:
            sigval = SIGILL;
            break;			
        case INVALID_SLOT_VEC:
            sigval = SIGILL;
            break;			
        case CPU_BUS_ERROR_VEC:
            sigval = SIGBUS;
            break;			
        case DMA_BUS_ERROR_VEC:
            sigval = SIGBUS;
            break;	
        case NMI_VEC:
            sigval = SIGINT;
            break;	

        case TRAP_VEC:
        case USER_VEC:
            sigval = SIGTRAP;
            break;

        default:
            sigval = SIGEMT;		/* "software generated"*/
            break;
    }
    return (sigval);
}

void doSStep (void)
{
    short *instrMem;
    int displacement;
    int reg;
    unsigned short opcode;

    instrMem = (short *) registers[PC];

    opcode = *instrMem;
    stepped = 1;

    if ((opcode & COND_BR_MASK) == BT_INSTR)
    {
        if (registers[SR] & T_BIT_MASK)
        {
            displacement = (opcode & COND_DISP) << 1;
            if (displacement & 0x80)
                displacement |= 0xffffff00;
            /*
             * Remember PC points to second instr.
             * after PC of branch ... so add 4
             */
            instrMem = (short *) (registers[PC] + displacement + 4);
        }
        else
            instrMem += 1;
    }
    else if ((opcode & COND_BR_MASK) == BF_INSTR)
    {
        if (registers[SR] & T_BIT_MASK)
            instrMem += 1;
        else
        {
            displacement = (opcode & COND_DISP) << 1;
            if (displacement & 0x80)
                displacement |= 0xffffff00;
            /*
             * Remember PC points to second instr.
             * after PC of branch ... so add 4
             */
            instrMem = (short *) (registers[PC] + displacement + 4);
        }
    }
    else if ((opcode & UCOND_DBR_MASK) == BRA_INSTR)
    {
        displacement = (opcode & UCOND_DISP) << 1;
        if (displacement & 0x0800)
            displacement |= 0xfffff000;

        /*
         * Remember PC points to second instr.
         * after PC of branch ... so add 4
         */
        instrMem = (short *) (registers[PC] + displacement + 4);
    }
    else if ((opcode & UCOND_RBR_MASK) == JSR_INSTR)
    {
        reg = (char) ((opcode & UCOND_REG) >> 8);

        instrMem = (short *) registers[reg];
    }
    else if (opcode == RTS_INSTR)
        instrMem = (short *) registers[PR];
    else if (opcode == RTE_INSTR)
        instrMem = (short *) registers[15];
    else if ((opcode & TRAPA_MASK) == TRAPA_INSTR)
        instrMem = (short *) ((opcode & ~TRAPA_MASK) << 2);
    else
        instrMem += 1;

    instrBuffer.memAddr = instrMem;
    instrBuffer.oldInstr = *instrMem;
    *instrMem = SSTEP_INSTR;
}


/* Undo the effect of a previous doSStep.  If we single stepped,
   restore the old instruction. */
void undoSStep (void)
{
    if (stepped)
    {
        short *instrMem;
        instrMem = instrBuffer.memAddr;
        *instrMem = instrBuffer.oldInstr;
    }
    stepped = 0;
}

/*
 * This function does all exception handling.  It only does two things -
 * it figures out why it was called and tells gdb, and then it reacts
 * to gdb's requests.
 *
*/
void gdb_handle_exception (int exceptionVector)
{
    int sigval, stepping;
    int addr, length;
    char *ptr;

    /* reply to host that an exception has occurred */
    sigval = computeSignal (exceptionVector);
    remcomOutBuffer[0] = 'S';
    remcomOutBuffer[1] = highhex(sigval);
    remcomOutBuffer[2] = lowhex (sigval);
    remcomOutBuffer[3] = 0;

    putpacket (remcomOutBuffer);

    /*
     * exception 127 indicates a software trap
     * inserted in place of code ... so back up
     * PC by one instruction, since this instruction
     * will later be replaced by its original one!
     */
    if (exceptionVector == USER_VEC
        || exceptionVector == TRAP_VEC)
        registers[PC] -= 2;

    /*
     * Do the things needed to undo
     * any stepping we may have done!
     */
    undoSStep ();

    stepping = 0;

    while (1)
    {
        remcomOutBuffer[0] = 0;
        ptr = getpacket ();

        switch (*ptr++)
        {
        case '?':
            remcomOutBuffer[0] = 'S';
            remcomOutBuffer[1] = highhex (sigval);
            remcomOutBuffer[2] = lowhex (sigval);
            remcomOutBuffer[3] = 0;
            break;
        case 'd':
            remote_debug = !(remote_debug);     /* toggle debug flag */
            break;
        case 'g':   /* return the value of the CPU registers */
            mem2hex ((char *) registers, remcomOutBuffer, NUMREGBYTES);
            break;
        case 'G':   /* set the value of the CPU registers - return OK */
            hex2mem (ptr, (char *) registers, NUMREGBYTES);
            strcpy (remcomOutBuffer, "OK");
            break;

            /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
            if (setjmp (remcomEnv) == 0)
            {
                dofault = 0;
                /* TRY, TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
                if (hex2int (&ptr, &addr))
                    if (*(ptr++) == ',')
                        if (hex2int (&ptr, &length))
                        {
                            ptr = 0;
                            mem2hex ((char *) addr, remcomOutBuffer, length);
                        }
                if (ptr)
                    strcpy (remcomOutBuffer, "E01");
            }
            else
                strcpy (remcomOutBuffer, "E03");

            /* restore handler for bus error */
            dofault = 1;
            break;

            /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
        case 'M':
            if (setjmp (remcomEnv) == 0)
            {
                dofault = 0;

                /* TRY, TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
                if (hex2int (&ptr, &addr))
                    if (*(ptr++) == ',')
                        if (hex2int (&ptr, &length))
                            if (*(ptr++) == ':')
                            {
                                hex2mem (ptr, (char *) addr, length);
                                ptr = 0;
                                strcpy (remcomOutBuffer, "OK");
                            }
                if (ptr)
                    strcpy (remcomOutBuffer, "E02");
            }
            else
                strcpy (remcomOutBuffer, "E03");

            /* restore handler for bus error */
            dofault = 1;
            break;

            /* cAA..AA    Continue at address AA..AA(optional) */
            /* sAA..AA   Step one instruction from AA..AA(optional) */
        case 's':
            stepping = 1;
        case 'c':
        {
            /* tRY, to read optional parameter, pc unchanged if no parm */
            if (hex2int (&ptr, &addr))
                registers[PC] = addr;

            if (stepping)
                doSStep ();
        }

        return;
        break;

        /* kill the program */
        case 'k':               /* do nothing */
            break;

        default:
            break;
        }                       /* switch */

        /* reply to the request */
        putpacket (remcomOutBuffer);
    }
}


/* We've had an exception - go into the gdb stub */
void handle_exception(int exceptionVector)
{
    gdb_handle_exception (exceptionVector);
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */
void breakpoint (void)
{
    BREAKPOINT ();
}

/**** Processor-specific routines start here ****/
/**** Processor-specific routines start here ****/
/**** Processor-specific routines start here ****/

extern int stack[];

/* SH1/SH2 exception vector table format */
typedef struct
{
        void (*func_cold) (void);
        int *stack_cold;
        void (*func_warm) (void);
        int *stack_warm;
        void (*(handler[128 - 4])) (void);
} vec_type;

/* vectable is the SH1/SH2 vector table. It must be at address 0
** or wherever your vbr points.
** Note that we only define the first 128 vectors, since the Jukebox
** firmware has its entry point at 0x200
*/
const vec_type vectable  __attribute__ ((section (".vectors"))) =
{ 
    &start,			        /* 0: Power-on reset PC */
    stack,                              /* 1: Power-on reset SP */
    &start,			        /* 2: Manual reset PC */
    stack,                              /* 3: Manual reset SP */
    {
        &catch_exception_4,		/* 4: General invalid instruction */
        &catch_exception_5,  	        /* 5: Reserved for system */
        &catch_exception_6,		/* 6: Invalid slot instruction */
        &catch_exception_7,	        /* 7: Reserved for system */
        &catch_exception_8,	        /* 8: Reserved for system */
        &catch_exception_9,		/* 9: CPU bus error */
        &catch_exception_10,		/* 10: DMA bus error */
        &catch_exception_11,		/* 11: NMI */
        &catch_exception_12,	        /* 12: User break */
        &catch_exception_13,	        /* 13: Reserved for system */
        &catch_exception_14,	        /* 14: Reserved for system */
        &catch_exception_15,	        /* 15: Reserved for system */
        &catch_exception_16,	        /* 16: Reserved for system */
        &catch_exception_17,	        /* 17: Reserved for system */
        &catch_exception_18,	        /* 18: Reserved for system */
        &catch_exception_19,	        /* 19: Reserved for system */
        &catch_exception_20,	        /* 20: Reserved for system */
        &catch_exception_21,	        /* 21: Reserved for system */
        &catch_exception_22,	        /* 22: Reserved for system */
        &catch_exception_23,	        /* 23: Reserved for system */
        &catch_exception_24,	        /* 24: Reserved for system */
        &catch_exception_25,	        /* 25: Reserved for system */
        &catch_exception_26,	        /* 26: Reserved for system */
        &catch_exception_27,	        /* 27: Reserved for system */
        &catch_exception_28,	        /* 28: Reserved for system */
        &catch_exception_29,	        /* 29: Reserved for system */
        &catch_exception_30,	        /* 30: Reserved for system */
        &catch_exception_31,	        /* 31: Reserved for system */
        &catch_exception_32,		/* 32: Trap instr (user vectors) */
        &catch_exception_33,		/* 33: Trap instr (user vectors) */
        &catch_exception_34,		/* 34: Trap instr (user vectors) */
        &catch_exception_35,		/* 35: Trap instr (user vectors) */
        &catch_exception_36,		/* 36: Trap instr (user vectors) */
        &catch_exception_37,		/* 37: Trap instr (user vectors) */
        &catch_exception_38,		/* 38: Trap instr (user vectors) */
        &catch_exception_39,		/* 39: Trap instr (user vectors) */
        &catch_exception_40,		/* 40: Trap instr (user vectors) */
        &catch_exception_41,		/* 41: Trap instr (user vectors) */
        &catch_exception_42,		/* 42: Trap instr (user vectors) */
        &catch_exception_43,		/* 43: Trap instr (user vectors) */
        &catch_exception_44,		/* 44: Trap instr (user vectors) */
        &catch_exception_45,		/* 45: Trap instr (user vectors) */
        &catch_exception_46,		/* 46: Trap instr (user vectors) */
        &catch_exception_47,		/* 47: Trap instr (user vectors) */
        &catch_exception_48,		/* 48: Trap instr (user vectors) */
        &catch_exception_49,		/* 49: Trap instr (user vectors) */
        &catch_exception_50,		/* 50: Trap instr (user vectors) */
        &catch_exception_51,		/* 51: Trap instr (user vectors) */
        &catch_exception_52,		/* 52: Trap instr (user vectors) */
        &catch_exception_53,		/* 53: Trap instr (user vectors) */
        &catch_exception_54,		/* 54: Trap instr (user vectors) */
        &catch_exception_55,		/* 55: Trap instr (user vectors) */
        &catch_exception_56,		/* 56: Trap instr (user vectors) */
        &catch_exception_57,		/* 57: Trap instr (user vectors) */
        &catch_exception_58,		/* 58: Trap instr (user vectors) */
        &catch_exception_59,		/* 59: Trap instr (user vectors) */
        &catch_exception_60,		/* 60: Trap instr (user vectors) */
        &catch_exception_61,		/* 61: Trap instr (user vectors) */
        &catch_exception_62,		/* 62: Trap instr (user vectors) */
        &catch_exception_63,		/* 63: Trap instr (user vectors) */
        &catch_exception_64,	        /* 64: IRQ0 */
        &catch_exception_65,	        /* 65: IRQ1 */
        &catch_exception_66,	        /* 66: IRQ2 */
        &catch_exception_67,	        /* 67: IRQ3 */
        &catch_exception_68,	        /* 68: IRQ4 */
        &catch_exception_69,	        /* 69: IRQ5 */
        &catch_exception_70,	        /* 70: IRQ6 */
        &catch_exception_71,	        /* 71: IRQ7 */
        &catch_exception_72,
        &catch_exception_73,
        &catch_exception_74,
        &catch_exception_75,
        &catch_exception_76,
        &catch_exception_77,
        &catch_exception_78,
        &catch_exception_79,
        &catch_exception_80,
        &catch_exception_81,
        &catch_exception_82,
        &catch_exception_83,
        &catch_exception_84,
        &catch_exception_85,
        &catch_exception_86,
        &catch_exception_87,
        &catch_exception_88,
        &catch_exception_89,
        &catch_exception_90,
        &catch_exception_91,
        &catch_exception_92,
        &catch_exception_93,
        &catch_exception_94,
        &catch_exception_95,
        &catch_exception_96,
        &catch_exception_97,
        &catch_exception_98,
        &catch_exception_99,
        &catch_exception_100,
        &catch_exception_101,
        &catch_exception_102,
        &catch_exception_103,
        &catch_exception_104,
        &catch_exception_105,
        &catch_exception_106,
        &catch_exception_107,
        &catch_exception_108,
        &catch_exception_109,
        &catch_exception_110,
        &catch_exception_111,
        &catch_exception_112,
        &catch_exception_113,
        &catch_exception_114,
        &catch_exception_115,
        &catch_exception_116,
        &catch_exception_117,
        &catch_exception_118,
        &catch_exception_119,
        &catch_exception_120,
        &catch_exception_121,
        &catch_exception_122,
        &catch_exception_123,
        &catch_exception_124,
        &catch_exception_125,
        &catch_exception_126,
        &catch_exception_127}};

void INIT (void)
{
    /* Disable all timer interrupts */
    TIER0 = 0;
    TIER1 = 0;
    TIER2 = 0;
    TIER3 = 0;
    TIER4 = 0;

    init_serial();

    in_nmi = 0;
    dofault = 1;
    stepped = 0;

    ata_spindown(-1);
    
    stub_sp = stub_stack;
    breakpoint ();

    /* We should never come here */
    blink();
}

void sr(void)
{
    /* Calling Reset does the same as pressing the button */
    asm (".global _Reset\n"
         "        .global _WarmReset\n"
         "_Reset:\n"
         "_WarmReset:\n"
         "         mov.l L_sp,r15\n"
         "         bra   _INIT\n"
         "         nop\n"
         "         .align 2\n"
         "L_sp:    .long _stack");

    asm("saveRegisters:\n");
    asm(" mov.l	@(L_reg, pc), r0\n"
        "	mov.l	@r15+, r1		! pop R0\n"
        "	mov.l	r2, @(0x08, r0)		! save R2\n"
        "	mov.l	r1, @r0			! save R0\n"
        "	mov.l	@r15+, r1		! pop R1\n"
        "	mov.l	r3, @(0x0c, r0)		! save R3\n"
        "	mov.l	r1, @(0x04, r0)		! save R1\n"
        "	mov.l	r4, @(0x10, r0)		! save R4\n"
        "	mov.l	r5, @(0x14, r0)		! save R5\n"
        "	mov.l	r6, @(0x18, r0)		! save R6\n"
        "	mov.l	r7, @(0x1c, r0)		! save R7\n"
        "	mov.l	r8, @(0x20, r0)		! save R8\n"
        "	mov.l	r9, @(0x24, r0)		! save R9\n"
        "	mov.l	r10, @(0x28, r0)	! save R10\n"
        "	mov.l	r11, @(0x2c, r0)	! save R11\n"
        "	mov.l	r12, @(0x30, r0)	! save R12\n"
        "	mov.l	r13, @(0x34, r0)	! save R13\n"
        "	mov.l	r14, @(0x38, r0)	! save R14\n"
        "	mov.l	@r15+, r4		! save arg to handleException\n"
        "	add	#8, r15			! hide PC/SR values on stack\n"
        "	mov.l	r15, @(0x3c, r0)	! save R15\n"
        "	add	#-8, r15		! save still needs old SP value\n"
        "	add	#92, r0			! readjust register pointer\n"
        "	mov	r15, r2\n"
        "	add	#4, r2\n"
        "	mov.l	@r2, r2			! R2 has SR\n"
        "	mov.l	@r15, r1		! R1 has PC\n"
        "	mov.l	r2, @-r0		! save SR\n"
        "	sts.l	macl, @-r0		! save MACL\n"
        "	sts.l	mach, @-r0		! save MACH\n"
        "	stc.l	vbr, @-r0		! save VBR\n"
        "	stc.l	gbr, @-r0		! save GBR\n"
        "	sts.l	pr, @-r0		! save PR\n"
        "	mov.l	@(L_stubstack, pc), r2\n"
        "	mov.l	@(L_hdl_except, pc), r3\n"
        "	mov.l	@r2, r15\n"
        "	jsr	@r3\n"
        "	mov.l	r1, @-r0		! save PC\n"
        "	mov.l	@(L_stubstack, pc), r0\n"
        "	mov.l	@(L_reg, pc), r1\n"
        "	bra	restoreRegisters\n"
        "	mov.l	r15, @r0		! save __stub_stack\n"
	
        "	.align 2\n"
        "L_reg:\n"
        "	.long	_registers\n"
        "L_stubstack:\n"
        "	.long	_stub_sp\n"
        "L_hdl_except:\n"
        "	.long	_handle_exception");
}

void rr(void)
{
    asm("	.align 2	\n"
        "        .global _resume\n"
        "_resume:\n"
        "	mov	r4,r1\n"
        "restoreRegisters:\n"
        "	add	#8, r1			! skip to R2\n"
        "	mov.l	@r1+, r2		! restore R2\n"
        "	mov.l	@r1+, r3		! restore R3\n"
        "	mov.l	@r1+, r4		! restore R4\n"
        "	mov.l	@r1+, r5		! restore R5\n"
        "	mov.l	@r1+, r6		! restore R6\n"
        "	mov.l	@r1+, r7		! restore R7\n"
        "	mov.l	@r1+, r8		! restore R8\n"
        "	mov.l	@r1+, r9		! restore R9\n"
        "	mov.l	@r1+, r10		! restore R10\n"
        "	mov.l	@r1+, r11		! restore R11\n"
        "	mov.l	@r1+, r12		! restore R12\n"
        "	mov.l	@r1+, r13		! restore R13\n"
        "	mov.l	@r1+, r14		! restore R14\n"
        "	mov.l	@r1+, r15		! restore programs stack\n"
        "	mov.l	@r1+, r0\n"
        "	add	#-8, r15		! uncover PC/SR on stack \n"
        "	mov.l	r0, @r15		! restore PC onto stack\n"
        "	lds.l	@r1+, pr		! restore PR\n"
        "	ldc.l	@r1+, gbr		! restore GBR\n"
        "	ldc.l	@r1+, vbr		! restore VBR\n"
        "	lds.l	@r1+, mach		! restore MACH\n"
        "	lds.l	@r1+, macl		! restore MACL\n"
        "	mov.l	@r1, r0	\n"
        "	add	#-88, r1		! readjust reg pointer to R1\n"
        "	mov.l	r0, @(4, r15)		! restore SR onto stack+4\n"
        "	mov.l	r2, @-r15\n"
        "	mov.l	L_in_nmi, r0\n"
        "	mov		#0, r2\n"
        "	mov.b	r2, @r0\n"
        "	mov.l	@r15+, r2\n"
        "	mov.l	@r1+, r0		! restore R0\n"
        "	rte\n"
        "	mov.l	@r1, r1			! restore R1");
}

static inline void code_for_catch_exception(unsigned int n)
{
    asm("		.globl	_catch_exception_%O0" : : "X" (n) ); 
    asm("	_catch_exception_%O0:" :: "X" (n) );
    
    asm("		add	#-4, r15 	! reserve spot on stack ");
    asm("		mov.l	r1, @-r15	! push R1		");
    
    if (n == NMI_VEC) 
    {
        /* Special case for NMI - make sure that they don't nest */
        asm("	mov.l	r0, @-r15	! push R0");
        asm("	mov.l	L_in_nmi, r0");
        asm("	tas.b	@r0		! Fend off against addtnl NMIs");
        asm("	bt		noNMI");
        asm("	mov.l	@r15+, r0");
        asm("	mov.l	@r15+, r1");
        asm("	add		#4, r15");
        asm("	rte");
        asm("	nop");
        asm(".align 2");
        asm("L_in_nmi: .long	_in_nmi");
        asm("noNMI:");
    }
    else
    {

        if (n == CPU_BUS_ERROR_VEC)
        {
            /* Exception 9 (bus errors) are disasbleable - so that you
               can probe memory and get zero instead of a fault.
               Because the vector table may be in ROM we don't revector
               the interrupt like all the other stubs, we check in here
            */
            asm("mov.l	L_dofault,r1");
            asm("mov.l	@r1,r1");
            asm("tst	r1,r1");
            asm("bf	faultaway");
            asm("bsr	_handle_buserror");
            asm(".align	2");
            asm("L_dofault: .long _dofault");
            asm("faultaway:");
        }
        asm("		mov	#15<<4, r1				");
        asm("		ldc	r1, sr		! disable interrupts	");
        asm("		mov.l	r0, @-r15	! push R0		");
    }

    /* Prepare for saving context, we've already pushed r0 and r1, stick
       exception number into the frame */
    asm("		mov	r15, r0				");
    asm("		add	#8, r0				");
    asm("		mov	%0,r1" :: "X" (n));
    asm("		extu.b  r1,r1				");
    asm("		bra	saveRegisters	! save register values	");
    asm("		mov.l	r1, @r0		! save exception # 	");
}

/* Here we call all defined exceptions, so the inline assembler gets
   generated */
void exceptions (void)
{
    code_for_catch_exception (4);
    code_for_catch_exception (5);
    code_for_catch_exception (6);
    code_for_catch_exception (7);
    code_for_catch_exception (8);
    code_for_catch_exception (9);
    code_for_catch_exception (10);
    code_for_catch_exception (11);
    code_for_catch_exception (12);
    code_for_catch_exception (13);
    code_for_catch_exception (14);
    code_for_catch_exception (15);
    code_for_catch_exception (16);
    code_for_catch_exception (17);
    code_for_catch_exception (18);
    code_for_catch_exception (19);
    code_for_catch_exception (20);
    code_for_catch_exception (21);
    code_for_catch_exception (22);
    code_for_catch_exception (23);
    code_for_catch_exception (24);
    code_for_catch_exception (25);
    code_for_catch_exception (26);
    code_for_catch_exception (27);
    code_for_catch_exception (28);
    code_for_catch_exception (29);
    code_for_catch_exception (30);
    code_for_catch_exception (31);
    code_for_catch_exception (32);
    code_for_catch_exception (33);
    code_for_catch_exception (34);
    code_for_catch_exception (35);
    code_for_catch_exception (36);
    code_for_catch_exception (37);
    code_for_catch_exception (38);
    code_for_catch_exception (39);
    code_for_catch_exception (40);
    code_for_catch_exception (41);
    code_for_catch_exception (42);
    code_for_catch_exception (43);
    code_for_catch_exception (44);
    code_for_catch_exception (45);
    code_for_catch_exception (46);
    code_for_catch_exception (47);
    code_for_catch_exception (48);
    code_for_catch_exception (49);
    code_for_catch_exception (50);
    code_for_catch_exception (51);
    code_for_catch_exception (52);
    code_for_catch_exception (53);
    code_for_catch_exception (54);
    code_for_catch_exception (55);
    code_for_catch_exception (56);
    code_for_catch_exception (57);
    code_for_catch_exception (58);
    code_for_catch_exception (59);
    code_for_catch_exception (60);
    code_for_catch_exception (61);
    code_for_catch_exception (62);
    code_for_catch_exception (63);
    code_for_catch_exception (64);
    code_for_catch_exception (65);
    code_for_catch_exception (66);
    code_for_catch_exception (67);
    code_for_catch_exception (68);
    code_for_catch_exception (69);
    code_for_catch_exception (70);
    code_for_catch_exception (71);
    code_for_catch_exception (72);
    code_for_catch_exception (73);
    code_for_catch_exception (74);
    code_for_catch_exception (75);
    code_for_catch_exception (76);
    code_for_catch_exception (77);
    code_for_catch_exception (78);
    code_for_catch_exception (79);
    code_for_catch_exception (80);
    code_for_catch_exception (81);
    code_for_catch_exception (82);
    code_for_catch_exception (83);
    code_for_catch_exception (84);
    code_for_catch_exception (85);
    code_for_catch_exception (86);
    code_for_catch_exception (87);
    code_for_catch_exception (88);
    code_for_catch_exception (89);
    code_for_catch_exception (90);
    code_for_catch_exception (91);
    code_for_catch_exception (92);
    code_for_catch_exception (93);
    code_for_catch_exception (94);
    code_for_catch_exception (95);
    code_for_catch_exception (96);
    code_for_catch_exception (97);
    code_for_catch_exception (98);
    code_for_catch_exception (99);
    code_for_catch_exception (100);
    code_for_catch_exception (101);
    code_for_catch_exception (102);
    code_for_catch_exception (103);
    code_for_catch_exception (104);
    code_for_catch_exception (105);
    code_for_catch_exception (106);
    code_for_catch_exception (107);
    code_for_catch_exception (108);
    code_for_catch_exception (109);
    code_for_catch_exception (110);
    code_for_catch_exception (111);
    code_for_catch_exception (112);
    code_for_catch_exception (113);
    code_for_catch_exception (114);
    code_for_catch_exception (115);
    code_for_catch_exception (116);
    code_for_catch_exception (117);
    code_for_catch_exception (118);
    code_for_catch_exception (119);
    code_for_catch_exception (120);
    code_for_catch_exception (121);
    code_for_catch_exception (122);
    code_for_catch_exception (123);
    code_for_catch_exception (124);
    code_for_catch_exception (125);
    code_for_catch_exception (126);
    code_for_catch_exception (127);
}

/*
 * Port B Control Register (PBCR1)
 */
#define	PB15MD1 	0x8000
#define	PB15MD0 	0x4000
#define	PB14MD1 	0x2000
#define	PB14MD0 	0x1000
#define	PB13MD1 	0x0800
#define	PB13MD0 	0x0400
#define	PB12MD1 	0x0200
#define	PB12MD0 	0x0100
#define	PB11MD1 	0x0080
#define	PB11MD0 	0x0040
#define	PB10MD1 	0x0020
#define	PB10MD0 	0x0010
#define	PB9MD1 		0x0008
#define	PB9MD0 		0x0004
#define	PB8MD1 		0x0002
#define	PB8MD0 		0x0001

#define	PB15MD 		PB15MD1|PB14MD0
#define	PB14MD 		PB14MD1|PB14MD0
#define	PB13MD 		PB13MD1|PB13MD0
#define	PB12MD 		PB12MD1|PB12MD0
#define	PB11MD 		PB11MD1|PB11MD0
#define	PB10MD 		PB10MD1|PB10MD0
#define	PB9MD 		PB9MD1|PB9MD0
#define	PB8MD 		PB8MD1|PB8MD0

#define PB_TXD1 	PB11MD1
#define PB_RXD1 	PB10MD1
#define PB_TXD0 	PB9MD1
#define PB_RXD0 	PB8MD1

#define	PB7MD 	PB7MD1|PB7MD0
#define	PB6MD 	PB6MD1|PB6MD0
#define	PB5MD 	PB5MD1|PB5MD0
#define	PB4MD 	PB4MD1|PB4MD0
#define	PB3MD 	PB3MD1|PB3MD0
#define	PB2MD 	PB2MD1|PB2MD0
#define	PB1MD 	PB1MD1|PB1MD0
#define	PB0MD 	PB0MD1|PB0MD0


void handleError (char theSSR);

void nop (void)
{
}

void init_serial (void)
{
    int i;

    /* Clear Channel 1's SCR   */
    SCR1 = 0;

    /* Set communication to be async, 8-bit data,
       no parity, 1 stop bit and use internal clock */
    SMR1 = 0;

#ifdef RECORDER
    #warning 115200
    BRR1 = 2; /* 115200 */
#else
    BRR1 = 9; /* 38400 */
#endif

    SCR1 &= ~(SCI_CKE1 | SCI_CKE0);

    /* let the hardware settle */
    for (i = 0; i < 1000; i++)
        nop ();

    /* Turn on in and out */
    SCR1 |= SCI_RE | SCI_TE;

    /* Set the PFC to make RXD1 (pin PB8) an input pin
       and TXD1 (pin PB9) an output pin */
    PBCR1 &= ~(PB_TXD1 | PB_RXD1);
    PBCR1 |= PB_TXD1 | PB_RXD1;
}


int serial_waitc(void)
{
    char mySSR;
    mySSR = SSR1 & ( SCI_PER | SCI_FER | SCI_ORER );
    if ( mySSR )
        handleError ( mySSR );
    return SSR1 & SCI_RDRF ;
}

char serial_getc (void)
{
    char ch;
    char mySSR;

    while ( ! serial_waitc())
        ;

    ch = RDR1;
    SSR1 &= ~SCI_RDRF;

    mySSR = SSR1 & (SCI_PER | SCI_FER | SCI_ORER);

    if (mySSR)
        handleError (mySSR);

    return ch;
}

void serial_putc (char ch)
{
    while (!(SSR1 & SCI_TDRE))
    {
        ;
    }
    
    /*
     * Write data into TDR and clear TDRE
     */
    TDR1 = ch;
    SSR1 &= ~SCI_TDRE;
}

void handleError (char theSSR)
{
    /* Clear all error bits, otherwise the receiver will stop */
    SSR1 &= ~(SCI_ORER | SCI_PER | SCI_FER);
}

void *memcpy(void *dest, const void *src0, size_t n)
{
    char *dst = (char *) dest;
    char *src = (char *) src0;
    
    void *save = dest;
    
    while(n--)
    {
        *dst++ = *src++;
    }
    
    return save;
}
