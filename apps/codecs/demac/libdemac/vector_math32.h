/*

libdemac - A Monkey's Audio decoder

$Id:$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

/* 32-bit vector math functions */

#define scalarproduct4_rev32(x,y) ((x[0]  * y[0]) + (x[1] * y[-1]) + \
                                   (x[2] * y[-2]) + (x[3] * y[-3]))

#define scalarproduct5_rev32(x,y) ((x[0]  * y[0]) + (x[1] * y[-1]) + \
                                   (x[2] * y[-2]) + (x[3] * y[-3]) + \
                                   (x[4] * y[-4]))

#define vector_sub4_rev32(x, y) { x[0] -= y[0]; \
                                  x[1] -= y[-1]; \
                                  x[2] -= y[-2]; \
                                  x[3] -= y[-3]; }

#define vector_sub5_rev32(x, y) { x[0] -= y[0]; \
                                  x[1] -= y[-1]; \
                                  x[2] -= y[-2]; \
                                  x[3] -= y[-3]; \
                                  x[4] -= y[-4]; }

#define vector_add4_rev32(x, y) { x[0] += y[0]; \
                                  x[1] += y[-1]; \
                                  x[2] += y[-2]; \
                                  x[3] += y[-3]; }

#define vector_add5_rev32(x, y) { x[0] += y[0]; \
                                  x[1] += y[-1]; \
                                  x[2] += y[-2]; \
                                  x[3] += y[-3]; \
                                  x[4] += y[-4]; }
