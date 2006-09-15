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

/* extern z80t z80op_nop(z80t); */
extern z80t z80op_ld_b_c(z80t);
extern z80t z80op_ld_b_d(z80t);
extern z80t z80op_ld_b_e(z80t);
extern z80t z80op_ld_b_h(z80t);
extern z80t z80op_ld_b_l(z80t);
extern z80t z80op_ld_b_ihl(z80t);
extern z80t z80op_ld_b_a(z80t);
extern z80t z80op_ld_c_b(z80t);
/* extern z80t z80op_nop(z80t); */
extern z80t z80op_ld_c_d(z80t);
extern z80t z80op_ld_c_e(z80t);
extern z80t z80op_ld_c_h(z80t);
extern z80t z80op_ld_c_l(z80t);
extern z80t z80op_ld_c_ihl(z80t);
extern z80t z80op_ld_c_a(z80t);

extern z80t z80op_ld_d_b(z80t);
extern z80t z80op_ld_d_c(z80t);
/* extern z80t z80op_nop(z80t); */
extern z80t z80op_ld_d_e(z80t);
extern z80t z80op_ld_d_h(z80t);
extern z80t z80op_ld_d_l(z80t);
extern z80t z80op_ld_d_ihl(z80t);
extern z80t z80op_ld_d_a(z80t);
extern z80t z80op_ld_e_b(z80t);
extern z80t z80op_ld_e_c(z80t);
extern z80t z80op_ld_e_d(z80t);
/* extern z80t z80op_nop(z80t); */
extern z80t z80op_ld_e_h(z80t);
extern z80t z80op_ld_e_l(z80t);
extern z80t z80op_ld_e_ihl(z80t);
extern z80t z80op_ld_e_a(z80t);

extern z80t z80op_ld_h_b(z80t);
extern z80t z80op_ld_h_c(z80t);
extern z80t z80op_ld_h_d(z80t);
extern z80t z80op_ld_h_e(z80t);
/* extern z80t z80op_nop(z80t); */
extern z80t z80op_ld_h_l(z80t);
extern z80t z80op_ld_h_ihl(z80t);
extern z80t z80op_ld_h_a(z80t);
extern z80t z80op_ld_l_b(z80t);
extern z80t z80op_ld_l_c(z80t);
extern z80t z80op_ld_l_d(z80t);
extern z80t z80op_ld_l_e(z80t);
extern z80t z80op_ld_l_h(z80t);
/* extern z80t z80op_nop(z80t); */
extern z80t z80op_ld_l_ihl(z80t);
extern z80t z80op_ld_l_a(z80t);

extern z80t z80op_ld_ihl_b(z80t);
extern z80t z80op_ld_ihl_c(z80t);
extern z80t z80op_ld_ihl_d(z80t);
extern z80t z80op_ld_ihl_e(z80t);
extern z80t z80op_ld_ihl_h(z80t);
extern z80t z80op_ld_ihl_l(z80t);
extern z80t z80op_halt(z80t);
extern z80t z80op_ld_ihl_a(z80t);
extern z80t z80op_ld_a_b(z80t);
extern z80t z80op_ld_a_c(z80t);
extern z80t z80op_ld_a_d(z80t);
extern z80t z80op_ld_a_e(z80t);
extern z80t z80op_ld_a_h(z80t);
extern z80t z80op_ld_a_l(z80t);
extern z80t z80op_ld_a_ihl(z80t);
/* extern z80t z80op_nop(z80t); */


/* IX */

extern z80t z80op_ld_b_ixh(z80t);
extern z80t z80op_ld_b_ixl(z80t);

extern z80t z80op_ld_c_ixh(z80t);
extern z80t z80op_ld_c_ixl(z80t);

extern z80t z80op_ld_d_ixh(z80t);
extern z80t z80op_ld_d_ixl(z80t);

extern z80t z80op_ld_e_ixh(z80t);
extern z80t z80op_ld_e_ixl(z80t);

extern z80t z80op_ld_ixh_b(z80t);
extern z80t z80op_ld_ixh_c(z80t);
extern z80t z80op_ld_ixh_d(z80t);
extern z80t z80op_ld_ixh_e(z80t);
/* extern z80t z80op_ld_ixh_ixh(z80t); */
extern z80t z80op_ld_ixh_ixl(z80t);
extern z80t z80op_ld_ixh_a(z80t);


extern z80t z80op_ld_ixl_b(z80t);
extern z80t z80op_ld_ixl_c(z80t);
extern z80t z80op_ld_ixl_d(z80t);
extern z80t z80op_ld_ixl_e(z80t);
extern z80t z80op_ld_ixl_ixh(z80t);
/* extern z80t z80op_ld_ixl_ixl(z80t); */
extern z80t z80op_ld_ixl_a(z80t);

extern z80t z80op_ld_a_ixh(z80t);
extern z80t z80op_ld_a_ixl(z80t);

extern z80t z80op_ld_iixd_b(z80t);
extern z80t z80op_ld_iixd_c(z80t);
extern z80t z80op_ld_iixd_d(z80t);
extern z80t z80op_ld_iixd_e(z80t);
extern z80t z80op_ld_iixd_h(z80t);
extern z80t z80op_ld_iixd_l(z80t);
extern z80t z80op_ld_iixd_a(z80t);

extern z80t z80op_ld_b_iixd(z80t);
extern z80t z80op_ld_c_iixd(z80t);
extern z80t z80op_ld_d_iixd(z80t);
extern z80t z80op_ld_e_iixd(z80t);
extern z80t z80op_ld_h_iixd(z80t);
extern z80t z80op_ld_l_iixd(z80t);
extern z80t z80op_ld_a_iixd(z80t);

/* IY */

extern z80t z80op_ld_b_iyh(z80t);
extern z80t z80op_ld_b_iyl(z80t);

extern z80t z80op_ld_c_iyh(z80t);
extern z80t z80op_ld_c_iyl(z80t);

extern z80t z80op_ld_d_iyh(z80t);
extern z80t z80op_ld_d_iyl(z80t);

extern z80t z80op_ld_e_iyh(z80t);
extern z80t z80op_ld_e_iyl(z80t);

extern z80t z80op_ld_iyh_b(z80t);
extern z80t z80op_ld_iyh_c(z80t);
extern z80t z80op_ld_iyh_d(z80t);
extern z80t z80op_ld_iyh_e(z80t);
/* extern z80t z80op_ld_iyh_iyh(z80t); */
extern z80t z80op_ld_iyh_iyl(z80t);
extern z80t z80op_ld_iyh_a(z80t);


extern z80t z80op_ld_iyl_b(z80t);
extern z80t z80op_ld_iyl_c(z80t);
extern z80t z80op_ld_iyl_d(z80t);
extern z80t z80op_ld_iyl_e(z80t);
extern z80t z80op_ld_iyl_iyh(z80t);
/* extern z80t z80op_ld_iyl_iyl(z80t); */
extern z80t z80op_ld_iyl_a(z80t);

extern z80t z80op_ld_a_iyh(z80t);
extern z80t z80op_ld_a_iyl(z80t);

extern z80t z80op_ld_iiyd_b(z80t);
extern z80t z80op_ld_iiyd_c(z80t);
extern z80t z80op_ld_iiyd_d(z80t);
extern z80t z80op_ld_iiyd_e(z80t);
extern z80t z80op_ld_iiyd_h(z80t);
extern z80t z80op_ld_iiyd_l(z80t);
extern z80t z80op_ld_iiyd_a(z80t);

extern z80t z80op_ld_b_iiyd(z80t);
extern z80t z80op_ld_c_iiyd(z80t);
extern z80t z80op_ld_d_iiyd(z80t);
extern z80t z80op_ld_e_iiyd(z80t);
extern z80t z80op_ld_h_iiyd(z80t);
extern z80t z80op_ld_l_iiyd(z80t);
extern z80t z80op_ld_a_iiyd(z80t);
