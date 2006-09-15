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

extern z80t z80op_nop(z80t);
extern z80t z80op_ld_bc_nn(z80t);
extern z80t z80op_ld_ibc_a(z80t);
extern z80t z80op_inc_bc(z80t);
extern z80t z80op_inc_b(z80t);
extern z80t z80op_dec_b(z80t);
extern z80t z80op_ld_b_n(z80t);
extern z80t z80op_rlca(z80t);
extern z80t z80op_ex_af_afb(z80t);
extern z80t z80op_add_hl_bc(z80t);
extern z80t z80op_ld_a_ibc(z80t);
extern z80t z80op_dec_bc(z80t);
extern z80t z80op_inc_c(z80t);
extern z80t z80op_dec_c(z80t);
extern z80t z80op_ld_c_n(z80t);
extern z80t z80op_rrca(z80t);

extern z80t z80op_djnz_e(z80t);
extern z80t z80op_ld_de_nn(z80t);
extern z80t z80op_ld_ide_a(z80t);
extern z80t z80op_inc_de(z80t);
extern z80t z80op_inc_d(z80t);
extern z80t z80op_dec_d(z80t);
extern z80t z80op_ld_d_n(z80t);
extern z80t z80op_rla(z80t);
extern z80t z80op_jr_e(z80t);
extern z80t z80op_add_hl_de(z80t);
extern z80t z80op_ld_a_ide(z80t);
extern z80t z80op_dec_de(z80t);
extern z80t z80op_inc_e(z80t);
extern z80t z80op_dec_e(z80t);
extern z80t z80op_ld_e_n(z80t);
extern z80t z80op_rra(z80t);

extern z80t z80op_jr_nz_e(z80t);
extern z80t z80op_ld_hl_nn(z80t);
extern z80t z80op_ld_inn_hl(z80t);
extern z80t z80op_inc_hl(z80t);
extern z80t z80op_inc_h(z80t);
extern z80t z80op_dec_h(z80t);
extern z80t z80op_ld_h_n(z80t);
extern z80t z80op_daa(z80t);
extern z80t z80op_jr_z_e(z80t);
extern z80t z80op_add_hl_hl(z80t);
extern z80t z80op_ld_hl_inn(z80t);
extern z80t z80op_dec_hl(z80t);
extern z80t z80op_inc_l(z80t);
extern z80t z80op_dec_l(z80t);
extern z80t z80op_ld_l_n(z80t);
extern z80t z80op_cpl(z80t);

extern z80t z80op_jr_nc_e(z80t);
extern z80t z80op_ld_sp_nn(z80t);
extern z80t z80op_ld_inn_a(z80t);
extern z80t z80op_inc_sp(z80t);
extern z80t z80op_inc_ihl(z80t);
extern z80t z80op_dec_ihl(z80t);
extern z80t z80op_ld_ihl_n(z80t);
extern z80t z80op_scf(z80t);
extern z80t z80op_jr_c_e(z80t);
extern z80t z80op_add_hl_sp(z80t);
extern z80t z80op_ld_a_inn(z80t);
extern z80t z80op_dec_sp(z80t);
extern z80t z80op_inc_a(z80t);
extern z80t z80op_dec_a(z80t);
extern z80t z80op_ld_a_n(z80t);
extern z80t z80op_ccf(z80t);

/* IX */

extern z80t z80op_add_ix_bc(z80t);
extern z80t z80op_add_ix_de(z80t);
extern z80t z80op_add_ix_ix(z80t);
extern z80t z80op_add_ix_sp(z80t);
extern z80t z80op_ld_ix_nn(z80t);
extern z80t z80op_ld_inn_ix(z80t);
extern z80t z80op_ld_ix_inn(z80t);
extern z80t z80op_inc_ix(z80t);
extern z80t z80op_dec_ix(z80t);
extern z80t z80op_inc_ixh(z80t);
extern z80t z80op_inc_ixl(z80t);
extern z80t z80op_inc_iixd(z80t);
extern z80t z80op_dec_ixh(z80t);
extern z80t z80op_dec_ixl(z80t);
extern z80t z80op_dec_iixd(z80t);
extern z80t z80op_ld_ixh_n(z80t);
extern z80t z80op_ld_ixl_n(z80t);
extern z80t z80op_ld_iixd_n(z80t);

/* IY */

extern z80t z80op_add_iy_bc(z80t);
extern z80t z80op_add_iy_de(z80t);
extern z80t z80op_add_iy_iy(z80t);
extern z80t z80op_add_iy_sp(z80t);
extern z80t z80op_ld_iy_nn(z80t);
extern z80t z80op_ld_inn_iy(z80t);
extern z80t z80op_ld_iy_inn(z80t);
extern z80t z80op_inc_iy(z80t);
extern z80t z80op_dec_iy(z80t);
extern z80t z80op_inc_iyh(z80t);
extern z80t z80op_inc_iyl(z80t);
extern z80t z80op_inc_iiyd(z80t);
extern z80t z80op_dec_iyh(z80t);
extern z80t z80op_dec_iyl(z80t);
extern z80t z80op_dec_iiyd(z80t);
extern z80t z80op_ld_iyh_n(z80t);
extern z80t z80op_ld_iyl_n(z80t);
extern z80t z80op_ld_iiyd_n(z80t);

