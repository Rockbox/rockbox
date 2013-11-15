/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

   $Id: misc.c,v 1.4 1996/08/26 15:04:20 ahornby Exp $
******************************************************************************/

/*

 * This WAS part of the x64 Commodore 64 emulator.
 *
 * This file contains misc funtions to help debugging.
 * Included are:
 *      o Show numeric conversions
 *      o Show CPU registers
 *      o Show Stack contents
 *      o Print binary number
 *      o Print instruction hexas from memory
 *      o Print instruction from memory
 *      o Decode instruction
 *      o Find effective address for operand
 *      o Create a copy of string
 *      o Move memory
 *
 * sprint_opcode returns mnemonic code of machine instruction.
 * sprint_binary returns binary form of given code (8bit)
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jarkko Sonninen (sonninen@lut.fi)
 *   Jouko Valta     (jopi@stekt.oulu.fi)
 *
 */

//#include <stdio.h>
/* #include <string.h> */
#include "rbconfig.h"
#include <stdlib.h>
#include <ctype.h>

#include "cpu.h"
#include "macro.h"
#include "misc.h"
#include "extern.h"
#include "memory.h"
#include "asm.h"

#define HEX 1

/*
 *  Numeric evaluation with error checking
 *
 *   char   *s;         pointer to input string
 *   int     level;     recursion level
 *   int     mode;      flag: dec/hex mode
 */

int
sconv (char *s, int level, int mode)
{
  static char hexas[] = "0123456789abcdefg";
  char *p = s;
  int base = 0;
  int result = 0, sign = 1;
  int i = 0;

  if (!p)
    return (0);

  switch (tolower (*p))
    {
    case '%':
      p++;
      base = 2;
      break;

    case 'o':
    case '&':
      p++;
      base = 8;
      break;

    case 'x':
    case '$':
      p++;
      base = 16;
      break;

    case 'u':
    case 'i':
    case '#':
      p++;
      base = 10;
      break;

    case '0':			/* 0x 0b 0d */
      if (!*++p)
	return ((mode & MODE_QUERY) ? 1 : 0);
      if (!isdigit (*p))
	return (sconv (p, level + 1, mode));

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      base = (mode & MODE_HEX) ? 16 : 10;
      break;

    case 'a':
    case 'c':
    case 'e':
    case 'f':
      if (mode & MODE_HEX)
	base = 16;
      break;

    case 'b':			/* hex or binary */
      if (mode & MODE_HEX)
	base = 16;
      else
	{
	  base = 2;
	  p++;
	}
      break;

    case 'd':			/* hex or decimal */
      if (mode & MODE_HEX)
	base = 16;
      else
	{
	  base = 10;
	  p++;
	}
      break;

    default:
      break;
    }

  /*
   * now p points to start of string to convert and base hold its base
   * number 2, 8, 10 or 16
   */

  if (!base)
    return (0);

  if (*p == '-')
    {
      sign = -1;
      p++;
    }
  while (tolower (*p))
    {
      for (i = 0; i < base; i++)
	if (*p == hexas[i])
	  {
	    result = result * base + i;
	    break;
	  }
      if (i >= base)
	{
	  /* unknown char has occurred, return value or error */
	  if (strchr (",-+()", *p) || isspace (*p))
	    i = 0;
	  else if (!level && !(mode & MODE_QUERY))
	    printf ("Bad character near '%s'\n", p);

	  break;
	}
      p++;
    }
  /* printf ("mode %02X  last %d base %d value %d\n",
     mode, i, base, result); */

  /* return final value */
  return ((mode & MODE_QUERY) ? (i < base) : result * sign);
}


/*
 * Base conversions.
 * (+) -%&#$
 */

void
show_bases (char *line, int mode)
{
  char buf[20];
  int temparg = sconv (line, 0, mode);

  strcpy (buf, sprint_binary (UPPER (temparg)));
  printf ("\n %17x x\n %17d d\n %17o o\n %s %s b\n\n",
	  temparg, temparg, temparg, buf,
	  sprint_binary (LOWER (temparg)));
}


/*
 * show prints PSW and contents of registers.
 */

void
show (void)
{
  hexflg = 1;
  if (1)
    printf (hexflg ?
	    "PC=%4X AC=%2X XR=%2X YR=%2X PF=%s SP=%2X %3X %3d %s\n" :
	    "PC=%04d AC=%03d XR=%03d YR=%03d PF=%s SP=%03d %3d %2X %s\n",
	    (int) PC, (int) AC, (int) XR, (int) YR,
	    sprint_status (), (int) SP,
	    LOAD (PC), LOAD (PC), sprint_opcode (PC, hexflg));
  else
    printf (hexflg ? "%lx %4X %s\n" : "%ld %4d %s\n",
	    clk, PC, sprint_opcode (PC, hexflg));
}


void
print_stack (BYTE sp)
{
  int i;

  printf ("Stack: ");
  for (i = 0x101 + sp; i < 0x200; i += 2)
    printf ("%02X%02X  ", dbgRead (i + 1), dbgRead (i));
  printf ("\n");
}


char
 *
sprint_binary (BYTE code)
{
  static char bin[9];
  int i;

  bin[8] = 0;			/* Terminator. */

  for (i = 0; i < 8; i++)
    {
      bin[i] = (code & 128) ? '1' : '0';
      code <<= 1;
    }

  return bin;
}


/* ------------------------------------------------------------------------- */

char
 *
sprint_ophex (ADDRESS p)
{
  static char hexbuf[20];
  char *bp;
  int j, len;

  len = clength[lookup[DLOAD (p)].addr_mode];
  *hexbuf = '\0';
  for (j = 0, bp = hexbuf; j < 3; j++, bp += 3)
    {
      if (j < len)
	{
	  sprintf (bp, "%02X ", DLOAD (p + j));
	}
      else
	{
	  strcat (bp, "   ");
	}
    }
  return hexbuf;
}


/* sprint_opcode parameters:

 *      string          the name of the machine instruction
 *      addr_mode       # describing used addressing mode, see "vmachine.h"
 *      base            if 1==base => HEX, 0==base => DEC, see code
 *      opcode          address of memory where machine instruction
 *                      and argument are. First byte is unused, because
 *                      it is the machine code and it is already known -
 *                      string!
 */


char
 *
sprint_opcode (ADDRESS counter, int base)
{
  BYTE x = DLOAD (counter);
  BYTE p1 = DLOAD (counter + 1);
  BYTE p2 = DLOAD (counter + 2);

  return sprint_disassembled (counter, x, p1, p2, base);
}


char
 *
sprint_disassembled (ADDRESS counter, BYTE x, BYTE p1, BYTE p2, int base)
{
  char *string;
  int addr_mode;
  char *buffp;
  static char buff[20];
  int ival;

  ival = p1 & 0xFF;

  buffp = buff;
  string = lookup[x].mnemonic;
  addr_mode = lookup[x].addr_mode;

  sprintf (buff, "%s", string);	/* Print opcode. */
  while (*++buffp);

  switch (addr_mode)
    {
      /*
       * Bits 0 and 1 are usual marks for X and Y indexed addresses, i.e.
       * if  bit #0 is set addressing mode is X indexed something and if
       * bit #1 is set addressing mode is Y indexed something. This is not
       * from MOS6502, but convention used in this program. See
       * "vmachine.h" for details.
       */

      /* Print arguments of the machine instruction. */

    case IMPLIED:
      break;

    case ACCUMULATOR:
      sprintf (buffp, " A");
      break;

    case IMMEDIATE:
      sprintf (buffp, ((base & HEX) ? " #$%02X" : " %3d"), ival);
      break;

    case ZERO_PAGE:
      sprintf (buffp, ((base & HEX) ? " $%02X" : " %3d"), ival);
      break;

    case ZERO_PAGE_X:
      sprintf (buffp, ((base & HEX) ? " $%02X,X" : " %3d,X"), ival);
      break;

    case ZERO_PAGE_Y:
      sprintf (buffp, ((base & HEX) ? " $%02X,Y" : " %3d,Y"), ival);
      break;

    case ABSOLUTE:
      ival |= ((p2 & 0xFF) << 8);
      sprintf (buffp, ((base & HEX) ? " $%04X" : " %5d"), ival);
      break;

    case ABSOLUTE_X:
      ival |= ((p2 & 0xFF) << 8);
      sprintf (buffp, ((base & HEX) ? " $%04X,X" : " %5d,X"), ival);
      break;

    case ABSOLUTE_Y:
      ival |= ((p2 & 0xFF) << 8);
      sprintf (buffp, ((base & HEX) ? " $%04X,Y" : " %5d,Y"), ival);
      break;

    case INDIRECT_X:
      sprintf (buffp, ((base & HEX) ? " ($%02X,X)" : " (%3d,X)"), ival);
      break;

    case INDIRECT_Y:
      sprintf (buffp, ((base & HEX) ? " ($%02X),Y" : " (%3d),Y"), ival);
      break;

    case ABS_INDIRECT:
      ival |= ((p2 & 0xFF) << 8);
      sprintf (buffp, ((base & HEX) ? " ($%04X)" : " (%5d)"), ival);
      break;

    case RELATIVE:
      if (0x80 & ival)
	ival -= 256;
      ival += counter;
      ival += 2;
      sprintf (buffp, ((base & HEX) ? " $%04X" : " %5d"), ival);
      break;
    }

  return buff;
}


int
eff_address (ADDRESS counter, int step)
{
  int addr_mode, eff;
  BYTE x = LOAD (counter);
  BYTE p1 = 0;
  ADDRESS p2 = 0;


  addr_mode = lookup[x].addr_mode;

  switch (clength[addr_mode])
    {
    case 2:
      p1 = LOAD (counter + 1);
      break;
    case 3:
      p2 = LOAD (counter + 1) | (LOAD (counter + 2) << 8);
      break;
    }

  switch (addr_mode)
    {

    case IMPLIED:
    case ACCUMULATOR:
      eff = -1;
      break;

    case IMMEDIATE:
      eff = -1;
      break;

    case ZERO_PAGE:
      eff = p1;
      break;

    case ZERO_PAGE_X:
      eff = (p1 + XR) & 0xff;
      break;

    case ZERO_PAGE_Y:
      eff = (p1 + YR) & 0xff;
      break;

    case ABSOLUTE:
      eff = p2;
      break;

    case ABSOLUTE_X:
      eff = p2 + XR;
      break;

    case ABSOLUTE_Y:
      eff = p2 + YR;
      break;

    case ABS_INDIRECT:		/* loads 2-byte address */
      eff = p2;
      break;

    case INDIRECT_X:
      eff = LOAD_ZERO_ADDR (p1 + XR);
      break;

    case INDIRECT_Y:
      eff = LOAD_ZERO_ADDR (p1) + YR;
      break;

    case RELATIVE:
      eff = REL_ADDR (counter + 2, p1);
      break;

    default:
      eff = -1;
    }

  return eff;
}


/* ------------------------------------------------------------------------- */

char
 *
xstrdup (char *str)
{
  char *t = (char *) malloc (strlen (str) + 1);
  strcpy (t, str);
  return (t);
}


char
 *
strndupp (char *str, int n){
  char *t = (char *) malloc (n + 1);
  strncpy (t, str, n);
  t[n] = '\0';
  return (t);
}


void
memmov (char *target, char *source, unsigned int length)
{
  if (target > source)
    {
      target += length;
      source += length;
      while (length--)
	*--target = *--source;
    }
  else if (target < source)
    {
      while (length--)
	*target++ = *source++;
    }
}
