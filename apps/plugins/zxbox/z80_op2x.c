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

/* IX */

LD_R_R(b, ixh, RB, XH, 0, 4)
LD_R_R(b, ixl, RB, XL, 0, 5)

LD_R_R(c, ixh, RC, XH, 1, 4)
LD_R_R(c, ixl, RC, XL, 1, 5)

LD_R_R(d, ixh, RD, XH, 2, 4)
LD_R_R(d, ixl, RD, XL, 2, 5)

LD_R_R(e, ixh, RE, XH, 3, 4)
LD_R_R(e, ixl, RE, XL, 3, 5)

LD_R_R(ixh, b,   XH, RB, 4, 0)
LD_R_R(ixh, c,   XH, RC, 4, 1)
LD_R_R(ixh, d,   XH, RD, 4, 2)
LD_R_R(ixh, e,   XH, RE, 4, 3)
LD_R_R(ixh, ixl, XH, XL, 4, 5)
LD_R_R(ixh, a,   XH, RA, 4, 7)

LD_R_R(ixl, b,   XL, RB, 5, 0)
LD_R_R(ixl, c,   XL, RC, 5, 1)
LD_R_R(ixl, d,   XL, RD, 5, 2)
LD_R_R(ixl, e,   XL, RE, 5, 3)
LD_R_R(ixl, ixh, XL, XH, 5, 4)
LD_R_R(ixl, a,   XL, RA, 5, 7)


LD_R_R(a, ixh, RA, XH, 7, 4)
LD_R_R(a, ixl, RA, XL, 7, 5)

LD_ID_R(ix, IX, b, RB, 0)
LD_ID_R(ix, IX, c, RC, 1)
LD_ID_R(ix, IX, d, RD, 2)
LD_ID_R(ix, IX, e, RE, 3)
LD_ID_R(ix, IX, h, RH, 4)
LD_ID_R(ix, IX, l, RL, 5)
LD_ID_R(ix, IX, a, RA, 6)

LD_R_ID(ix, IX, b, RB, 0)
LD_R_ID(ix, IX, c, RC, 1)
LD_R_ID(ix, IX, d, RD, 2)
LD_R_ID(ix, IX, e, RE, 3)
LD_R_ID(ix, IX, h, RH, 4)
LD_R_ID(ix, IX, l, RL, 5)
LD_R_ID(ix, IX, a, RA, 6)


/* IY */

LD_R_R(b, iyh, RB, YH, 0, 4)
LD_R_R(b, iyl, RB, YL, 0, 5)

LD_R_R(c, iyh, RC, YH, 1, 4)
LD_R_R(c, iyl, RC, YL, 1, 5)

LD_R_R(d, iyh, RD, YH, 2, 4)
LD_R_R(d, iyl, RD, YL, 2, 5)

LD_R_R(e, iyh, RE, YH, 3, 4)
LD_R_R(e, iyl, RE, YL, 3, 5)

LD_R_R(iyh, b,   YH, RB, 4, 0)
LD_R_R(iyh, c,   YH, RC, 4, 1)
LD_R_R(iyh, d,   YH, RD, 4, 2)
LD_R_R(iyh, e,   YH, RE, 4, 3)
LD_R_R(iyh, iyl, YH, YL, 4, 5)
LD_R_R(iyh, a,   YH, RA, 4, 7)

LD_R_R(iyl, b,   YL, RB, 5, 0)
LD_R_R(iyl, c,   YL, RC, 5, 1)
LD_R_R(iyl, d,   YL, RD, 5, 2)
LD_R_R(iyl, e,   YL, RE, 5, 3)
LD_R_R(iyl, iyh, YL, YH, 5, 4)
LD_R_R(iyl, a,   YL, RA, 5, 7)


LD_R_R(a, iyh, RA, YH, 7, 4)
LD_R_R(a, iyl, RA, YL, 7, 5)

LD_ID_R(iy, IY, b, RB, 0)
LD_ID_R(iy, IY, c, RC, 1)
LD_ID_R(iy, IY, d, RD, 2)
LD_ID_R(iy, IY, e, RE, 3)
LD_ID_R(iy, IY, h, RH, 4)
LD_ID_R(iy, IY, l, RL, 5)
LD_ID_R(iy, IY, a, RA, 6)

LD_R_ID(iy, IY, b, RB, 0)
LD_R_ID(iy, IY, c, RC, 1)
LD_R_ID(iy, IY, d, RD, 2)
LD_R_ID(iy, IY, e, RE, 3)
LD_R_ID(iy, IY, h, RH, 4)
LD_R_ID(iy, IY, l, RL, 5)
LD_R_ID(iy, IY, a, RA, 6)
