/*
 * motion_comp.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 */


#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

#define predict_o(i) (ref[i])
#define predict_x(i) (avg2 (ref[i], ref[i+1]))
#define predict_y(i) (avg2 (ref[i], (ref+stride)[i]))
#define predict_xy(i) (avg4 (ref[i], ref[i+1], \
                             (ref+stride)[i], (ref+stride)[i+1]))

#define put(predictor,i) dest[i] = predictor (i)
#define avg(predictor,i) dest[i] = avg2 (predictor (i), dest[i])

/* mc function template */
#define MC_FUNC(op, xy) \
    MC_FUNC_16(op, xy) \
    MC_FUNC_8(op, xy)

#define MC_FUNC_16(op, xy) \
    void MC_##op##_##xy##_16 (uint8_t * dest, const uint8_t * ref, \
                              const int stride, int height)        \
    {                                                              \
        do {                                                       \
            op (predict_##xy, 0);                                  \
            op (predict_##xy, 1);                                  \
            op (predict_##xy, 2);                                  \
            op (predict_##xy, 3);                                  \
            op (predict_##xy, 4);                                  \
            op (predict_##xy, 5);                                  \
            op (predict_##xy, 6);                                  \
            op (predict_##xy, 7);                                  \
            op (predict_##xy, 8);                                  \
            op (predict_##xy, 9);                                  \
            op (predict_##xy, 10);                                 \
            op (predict_##xy, 11);                                 \
            op (predict_##xy, 12);                                 \
            op (predict_##xy, 13);                                 \
            op (predict_##xy, 14);                                 \
            op (predict_##xy, 15);                                 \
            ref += stride;                                         \
            dest += stride;                                        \
        } while (--height);                                        \
    }

#define MC_FUNC_8(op, xy) \
    void MC_##op##_##xy##_8 (uint8_t * dest, const uint8_t * ref, \
                             const int stride, int height)        \
    {                                                             \
        do {                                                      \
            op (predict_##xy, 0);                                 \
            op (predict_##xy, 1);                                 \
            op (predict_##xy, 2);                                 \
            op (predict_##xy, 3);                                 \
            op (predict_##xy, 4);                                 \
            op (predict_##xy, 5);                                 \
            op (predict_##xy, 6);                                 \
            op (predict_##xy, 7);                                 \
            ref += stride;                                        \
            dest += stride;                                       \
        } while (--height);                                       \
    }
