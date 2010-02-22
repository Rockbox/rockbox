/* Based on:
  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


/* partial change history:
 *
 * 2004-10-10 mbp: Lift out character type dependencies into macros.
 *
 * Eric Sosman pointed out that ctype functions take a parameter whose
 * value must be that of an unsigned int, even on platforms that have
 * negative chars in their default char type.
 *
 *
 * Changes for Rockbox:
 * This version is changed slightly to deal better with the datatypes,
 * it does not equal to the original software.
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "strnatcmp.h"

#define assert(x)   /* nothing */

/* Convert char to int regardless of whether char is signed or not */
static inline int
to_int(char c)
{
    return (int) ((unsigned char) c);
}

/* These are defined as macros to make it easier to adapt this code to
 * different characters types or comparison functions. */

static inline int
nat_isdigit(int a)
{
     return isdigit(a);
}

static inline int
nat_unify_case(int a)
{
     /* We use 'tolower' and not 'toupper' so that '_' gets sorted
        before the letters */
     return tolower(a);
}



static int
compare_right(char const *a, char const *b)
{
     int bias = 0;
     int ca, cb;
     
     /* The longest run of digits wins.  That aside, the greatest
        value wins, but we can't know that it will until we've scanned
        both numbers to know that they have the same magnitude, so we
        remember it in BIAS. */
     for (;; a++, b++) {
      ca = to_int(*a);
      cb = to_int(*b);
          if (!nat_isdigit(ca)  &&  !nat_isdigit(cb))
               return bias;
          else if (!nat_isdigit(ca))
               return -1;
          else if (!nat_isdigit(cb))
               return +1;
          else if (ca < cb) {
               if (!bias)
                    bias = -1;
          } else if (ca > cb) {
               if (!bias)
                    bias = +1;
          } else if (!ca  &&  !cb)
               return bias;
     }

     return 0;
}


static int
compare_left(char const *a, char const *b)
{
     /* Compare two left-aligned numbers: the first to have a
        different value wins. */
     for (;; a++, b++) {
          if (!nat_isdigit(*a)  &&  !nat_isdigit(*b))
               return 0;
          else if (!nat_isdigit(*a))
               return -1;
          else if (!nat_isdigit(*b))
               return +1;
          else if (*a < *b)
               return -1;
          else if (*a > *b)
               return +1;
     }
          
     return 0;
}

static int strnatcmp0(char const *a, char const *b, int fold_case)
{
     int ai, bi;
     int ca, cb;
     int fractional, result;
     
     assert(a && b);
     ai = bi = 0;
     while (1) {
      ca = to_int(a[ai]);
      cb = to_int(b[bi]);

          /* process run of digits */
          if (nat_isdigit(ca)  &&  nat_isdigit(cb)) {
               fractional = (ca == '0' || cb == '0');

               if (fractional) {
                    if ((result = compare_left(a+ai, b+bi)) != 0)
                         return result;
               } else {
                    if ((result = compare_right(a+ai, b+bi)) != 0)
                         return result;
               }
          }

          if (!ca && !cb) {
               /* The strings compare the same.  Call str[case]cmp() to ensure
                  consistent results. */
               if(fold_case)
                   return strcasecmp(a,b);
               else
                   return strcmp(a,b);
          }

          if (fold_case) {
               ca = nat_unify_case(ca);
               cb = nat_unify_case(cb);
          }
          
          if (ca < cb)
               return -1;
          else if (ca > cb)
               return +1;

          ++ai; ++bi;
     }
}



int strnatcmp(const char *a, const char *b) {
     return strnatcmp0(a, b, 0);
}


/* Compare, recognizing numeric string and ignoring case. */
int strnatcasecmp(const char *a, const char *b) {
     return strnatcmp0(a, b, 1);
}
