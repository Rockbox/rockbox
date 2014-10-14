/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __INTERN_H__
#define __INTERN_H__

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <rbendian.h>
#include "util.h"

#define AWMAX(x,y) ((x)>(y)?(x):(y))
#define AWMIN(x,y) ((x)<(y)?(x):(y))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

/*
  The C programming language does not support templates but there is a solution,
  see http://arnold.uthar.net/index.php?n=Work.TemplatesC for a work around
  Or would just writing out all the functions one by one just be easier

********I'm not sure if we even need this at all!************

Does this do framebuffer swapping?

templete<typename T>
inline void SWAP(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}
*/

struct Ptr {
    uint8_t* pc;
};

extern uint8_t* scriptPtr_pc;

inline uint8_t scriptPtr_fetchByte(struct Ptr* p) {
    return *p->pc++;
}

inline uint16_t scriptPtr_fetchWord(struct Ptr* p) {
    uint16_t i = READ_BE_UINT16(b->pc);
    scriptPtr_pc += 2;
    return i;
}

struct Point {
    int16_t x, y;
};

#endif
