/* 
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*             akey.h
 *
 *     Header of the AKEY module
 *
 *     Keyboard handeling routines and key identification labels
 *
 *     Created:   92/12/01    Szeredi Miklos
 */


#ifndef _AKEY_H
#define _AKEY_H


#define  NOKEY           -1
#define  UNKNOWNKEY      -2
#define  ERRKEY          -3
#define  MOUSEKEY        -256

#define  UPKEY           (256 + 1)
#define  DOWNKEY         (256 + 2)
#define  RIGHTKEY        (256 + 3)
#define  LEFTKEY         (256 + 4)
#define  INSKEY          (256 + 5)
#define  DELKEY          (256 + 6)
#define  HOMEKEY         (256 + 7)
#define  ENDKEY          (256 + 8)
#define  PUKEY           (256 + 9)
#define  PDKEY           (256 + 10)
#define  BSKEY           (256 + 16)

#define  FKEYOFFS        (256 + 128)
#define  FKEYFIRST       (FKEYOFFS + 1)
#define  FKEYLAST        (FKEYOFFS + 12)
#define  FKEY(num)       (FKEYOFFS + num)

#define  ALTKEY          (1 << 11)
#define  CTRKEY          (1 << 10)
#define  SHKEY           (1 << 9)

#define  BKTABKEY        (TABKEY | SHKEY)

#define  CTL(ch)         ((ch) - 96)
#define  META(ch)        ((ch) | ALTKEY)

#define  TABKEY          9
#define  LFKEY           13 
#define  CRKEY           10
#define  ENTERKEY        LFKEY
#define  ESCKEY          27

#define  lastakey()      __lastakey
#define  waitakey()      ((void)readakey())
#define  setakey(key)    ((void)(__lastakey = (key)))
#define  setakeydo(todo) (__atodo = (todo))


typedef  int             keytype;
typedef  void            (*__atodotype)(void);
#ifdef __cplusplus
extern "C" {
#endif

extern   __atodotype      __atodo;

extern   keytype         __lastakey;

   extern   keytype         getakey(void);
/* extern   void            setakey(keytype);    */ /* MACRO */
   extern   keytype         readakey(void);
/* extern   keytype         lastakey(void);      */ /* MACRO */
/* extern   void            waitakey(void);      */ /* MACRO */
   extern   int             pressedakey(void);
   extern   void            clearakeybuff(void);
/* extern   void            setakeydo(dofn __atodotype); */ /* MACRO */
   extern   void            ungetakey(void);
   extern   int             insertakey(keytype key);
   extern   void            setasmalldelay(int delay);

   extern   int             initakey(void);
   extern   void            closeakey(void);

#ifdef __cplusplus
}
#endif


#endif /* _AKEY_H */

/*         End of akey.h                 */

