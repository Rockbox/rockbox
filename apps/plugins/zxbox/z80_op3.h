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

extern z80t z80op_add_a_b(z80t);
extern z80t z80op_add_a_c(z80t);
extern z80t z80op_add_a_d(z80t);
extern z80t z80op_add_a_e(z80t);
extern z80t z80op_add_a_h(z80t);
extern z80t z80op_add_a_l(z80t);
extern z80t z80op_add_a_ihl(z80t);
extern z80t z80op_add_a_a(z80t);
extern z80t z80op_adc_a_b(z80t);
extern z80t z80op_adc_a_c(z80t);
extern z80t z80op_adc_a_d(z80t);
extern z80t z80op_adc_a_e(z80t);
extern z80t z80op_adc_a_h(z80t);
extern z80t z80op_adc_a_l(z80t);
extern z80t z80op_adc_a_ihl(z80t);
extern z80t z80op_adc_a_a(z80t);

extern z80t z80op_sub_b(z80t);
extern z80t z80op_sub_c(z80t);
extern z80t z80op_sub_d(z80t);
extern z80t z80op_sub_e(z80t);
extern z80t z80op_sub_h(z80t);
extern z80t z80op_sub_l(z80t);
extern z80t z80op_sub_ihl(z80t);
extern z80t z80op_sub_a(z80t);
extern z80t z80op_sbc_a_b(z80t);
extern z80t z80op_sbc_a_c(z80t);
extern z80t z80op_sbc_a_d(z80t);
extern z80t z80op_sbc_a_e(z80t);
extern z80t z80op_sbc_a_h(z80t);
extern z80t z80op_sbc_a_l(z80t);
extern z80t z80op_sbc_a_ihl(z80t);
extern z80t z80op_sbc_a_a(z80t);

extern z80t z80op_and_b(z80t);
extern z80t z80op_and_c(z80t);
extern z80t z80op_and_d(z80t);
extern z80t z80op_and_e(z80t);
extern z80t z80op_and_h(z80t);
extern z80t z80op_and_l(z80t);
extern z80t z80op_and_ihl(z80t);
extern z80t z80op_and_a(z80t);
extern z80t z80op_xor_b(z80t);
extern z80t z80op_xor_c(z80t);
extern z80t z80op_xor_d(z80t);
extern z80t z80op_xor_e(z80t);
extern z80t z80op_xor_h(z80t);
extern z80t z80op_xor_l(z80t);
extern z80t z80op_xor_ihl(z80t);
extern z80t z80op_xor_a(z80t);

extern z80t z80op_or_b(z80t);
extern z80t z80op_or_c(z80t);
extern z80t z80op_or_d(z80t);
extern z80t z80op_or_e(z80t);
extern z80t z80op_or_h(z80t);
extern z80t z80op_or_l(z80t);
extern z80t z80op_or_ihl(z80t);
extern z80t z80op_or_a(z80t);
extern z80t z80op_cp_b(z80t);
extern z80t z80op_cp_c(z80t);
extern z80t z80op_cp_d(z80t);
extern z80t z80op_cp_e(z80t);
extern z80t z80op_cp_h(z80t);
extern z80t z80op_cp_l(z80t);
extern z80t z80op_cp_ihl(z80t);
extern z80t z80op_cp_a(z80t);


/* IX */

extern z80t z80op_add_a_ixh(z80t);
extern z80t z80op_add_a_ixl(z80t);
extern z80t z80op_add_a_iixd(z80t);

extern z80t z80op_adc_a_ixh(z80t);
extern z80t z80op_adc_a_ixl(z80t);
extern z80t z80op_adc_a_iixd(z80t);

extern z80t z80op_sub_ixh(z80t);
extern z80t z80op_sub_ixl(z80t);
extern z80t z80op_sub_iixd(z80t);

extern z80t z80op_sbc_a_ixh(z80t);
extern z80t z80op_sbc_a_ixl(z80t);
extern z80t z80op_sbc_a_iixd(z80t);

extern z80t z80op_and_ixh(z80t);
extern z80t z80op_and_ixl(z80t);
extern z80t z80op_and_iixd(z80t);

extern z80t z80op_xor_ixh(z80t);
extern z80t z80op_xor_ixl(z80t);
extern z80t z80op_xor_iixd(z80t);

extern z80t z80op_or_ixh(z80t);
extern z80t z80op_or_ixl(z80t);
extern z80t z80op_or_iixd(z80t);

extern z80t z80op_cp_ixh(z80t);
extern z80t z80op_cp_ixl(z80t);
extern z80t z80op_cp_iixd(z80t);


/* IY */

extern z80t z80op_add_a_iyh(z80t);
extern z80t z80op_add_a_iyl(z80t);
extern z80t z80op_add_a_iiyd(z80t);

extern z80t z80op_adc_a_iyh(z80t);
extern z80t z80op_adc_a_iyl(z80t);
extern z80t z80op_adc_a_iiyd(z80t);

extern z80t z80op_sub_iyh(z80t);
extern z80t z80op_sub_iyl(z80t);
extern z80t z80op_sub_iiyd(z80t);

extern z80t z80op_sbc_a_iyh(z80t);
extern z80t z80op_sbc_a_iyl(z80t);
extern z80t z80op_sbc_a_iiyd(z80t);

extern z80t z80op_and_iyh(z80t);
extern z80t z80op_and_iyl(z80t);
extern z80t z80op_and_iiyd(z80t);

extern z80t z80op_xor_iyh(z80t);
extern z80t z80op_xor_iyl(z80t);
extern z80t z80op_xor_iiyd(z80t);

extern z80t z80op_or_iyh(z80t);
extern z80t z80op_or_iyl(z80t);
extern z80t z80op_or_iiyd(z80t);

extern z80t z80op_cp_iyh(z80t);
extern z80t z80op_cp_iyl(z80t);
extern z80t z80op_cp_iiyd(z80t);
