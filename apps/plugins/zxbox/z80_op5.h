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

extern z80t z80op_ill_ed(z80t);

extern z80t z80op_in_b_ic(z80t);
extern z80t z80op_out_ic_b(z80t);
extern z80t z80op_sbc_hl_bc(z80t);
extern z80t z80op_ld_inn_bc(z80t);
extern z80t z80op_neg(z80t);
extern z80t z80op_retn(z80t);
extern z80t z80op_im_0(z80t);
extern z80t z80op_ld_i_a(z80t);
extern z80t z80op_in_c_ic(z80t);
extern z80t z80op_out_ic_c(z80t);
extern z80t z80op_adc_hl_bc(z80t);
extern z80t z80op_ld_bc_inn(z80t);
/* extern z80t z80op_neg(z80t); */
extern z80t z80op_reti(z80t);
/*extern z80t z80op_im_0(z80t); */
extern z80t z80op_ld_r_a(z80t);

extern z80t z80op_in_d_ic(z80t);
extern z80t z80op_out_ic_d(z80t);
extern z80t z80op_sbc_hl_de(z80t);
extern z80t z80op_ld_inn_de(z80t);
/* extern z80t z80op_neg(z80t); */
/* extern z80t z80op_retn(z80t); */
extern z80t z80op_im_1(z80t);
extern z80t z80op_ld_a_i(z80t);
extern z80t z80op_in_e_ic(z80t);
extern z80t z80op_out_ic_e(z80t);
extern z80t z80op_adc_hl_de(z80t);
extern z80t z80op_ld_de_inn(z80t);
/* extern z80t z80op_neg(z80t); */
/* extern z80t z80op_retn(z80t); */
extern z80t z80op_im_2(z80t);
extern z80t z80op_ld_a_r(z80t);

extern z80t z80op_in_h_ic(z80t);
extern z80t z80op_out_ic_h(z80t);
extern z80t z80op_sbc_hl_hl(z80t);
extern z80t z80op_ld_inn_hl_ed(z80t);
/* extern z80t z80op_neg(z80t); */
/* extern z80t z80op_retn(z80t); */
/* extern z80t z80op_im_0(z80t); */
extern z80t z80op_rrd(z80t);
extern z80t z80op_in_l_ic(z80t);
extern z80t z80op_out_ic_l(z80t);
extern z80t z80op_adc_hl_hl(z80t);
extern z80t z80op_ld_hl_inn_ed(z80t);
/* extern z80t z80op_neg(z80t); */
/* extern z80t z80op_retn(z80t); */
/* extern z80t z80op_im_0(z80t); */
extern z80t z80op_rld(z80t);

extern z80t z80op_in_f_ic(z80t);
extern z80t z80op_out_ic_0(z80t);
extern z80t z80op_sbc_hl_sp(z80t);
extern z80t z80op_ld_inn_sp(z80t);
/* extern z80t z80op_neg(z80t); */
/* extern z80t z80op_retn(z80t); */
/* extern z80t z80op_im_1(z80t); */
/* extern z80t z80op_ill_ed(z80t); */
extern z80t z80op_in_a_ic(z80t);
extern z80t z80op_out_ic_a(z80t);
extern z80t z80op_adc_hl_sp(z80t);
extern z80t z80op_ld_sp_inn(z80t);
/* extern z80t z80op_neg(z80t); */
/* extern z80t z80op_retn(z80t); */
/* extern z80t z80op_im_2(z80t); */
/* extern z80t z80op_ill_ed(z80t); */

extern z80t z80op_ldi(z80t);
extern z80t z80op_cpi(z80t);
extern z80t z80op_ini(z80t);
extern z80t z80op_outi(z80t);

extern z80t z80op_ldd(z80t);
extern z80t z80op_cpd(z80t);
extern z80t z80op_ind(z80t);
extern z80t z80op_outd(z80t);

extern z80t z80op_ldir(z80t);
extern z80t z80op_cpir(z80t);
extern z80t z80op_inir(z80t);
extern z80t z80op_otir(z80t);

extern z80t z80op_lddr(z80t);
extern z80t z80op_cpdr(z80t);
extern z80t z80op_indr(z80t);
extern z80t z80op_otdr(z80t);
