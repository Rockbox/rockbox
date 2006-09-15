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

extern z80t z80op_ret_nz(z80t);
extern z80t z80op_pop_bc(z80t);
extern z80t z80op_jp_nz_nn(z80t);
extern z80t z80op_jp_nn(z80t);
extern z80t z80op_call_nz_nn(z80t);
extern z80t z80op_push_bc(z80t);
extern z80t z80op_add_a_n(z80t);
extern z80t z80op_rst_00(z80t);
extern z80t z80op_ret_z(z80t);
extern z80t z80op_ret(z80t);
extern z80t z80op_jp_z_nn(z80t);
extern z80t z80op_special_cb(z80t);
extern z80t z80op_call_z_nn(z80t);
extern z80t z80op_call_nn(z80t);
extern z80t z80op_adc_a_n(z80t);
extern z80t z80op_rst_08(z80t);

extern z80t z80op_ret_nc(z80t);
extern z80t z80op_pop_de(z80t);
extern z80t z80op_jp_nc_nn(z80t);
extern z80t z80op_out_in_a(z80t);
extern z80t z80op_call_nc_nn(z80t);
extern z80t z80op_push_de(z80t);
extern z80t z80op_sub_n(z80t);
extern z80t z80op_rst_10(z80t);
extern z80t z80op_ret_c(z80t);
extern z80t z80op_exx(z80t);
extern z80t z80op_jp_c_nn(z80t);
extern z80t z80op_in_a_in(z80t);
extern z80t z80op_call_c_nn(z80t);
extern z80t z80op_special_dd(z80t);
extern z80t z80op_sbc_a_n(z80t);
extern z80t z80op_rst_18(z80t);

extern z80t z80op_ret_po(z80t);
extern z80t z80op_pop_hl(z80t);
extern z80t z80op_jp_po_nn(z80t);
extern z80t z80op_ex_isp_hl(z80t);
extern z80t z80op_call_po_nn(z80t);
extern z80t z80op_push_hl(z80t);
extern z80t z80op_and_n(z80t);
extern z80t z80op_rst_20(z80t);
extern z80t z80op_ret_pe(z80t);
extern z80t z80op_jp_hl(z80t);
extern z80t z80op_jp_pe_nn(z80t);
extern z80t z80op_ex_de_hl(z80t);
extern z80t z80op_call_pe_nn(z80t);
extern z80t z80op_special_ed(z80t);
extern z80t z80op_xor_n(z80t);
extern z80t z80op_rst_28(z80t);

extern z80t z80op_ret_p(z80t);
extern z80t z80op_pop_af(z80t);
extern z80t z80op_jp_p_nn(z80t);
extern z80t z80op_di(z80t);
extern z80t z80op_call_p_nn(z80t);
extern z80t z80op_push_af(z80t);
extern z80t z80op_or_n(z80t);
extern z80t z80op_rst_30(z80t);
extern z80t z80op_ret_m(z80t);
extern z80t z80op_ld_sp_hl(z80t);
extern z80t z80op_jp_m_nn(z80t);
extern z80t z80op_ei(z80t);
extern z80t z80op_call_m_nn(z80t);
extern z80t z80op_special_fd(z80t);
extern z80t z80op_cp_n(z80t);
extern z80t z80op_rst_38(z80t);

/* IX */

extern z80t z80op_pop_ix(z80t);
extern z80t z80op_push_ix(z80t);

extern z80t z80op_jp_ix(z80t);
extern z80t z80op_ld_sp_ix(z80t);
extern z80t z80op_ex_isp_ix(z80t);

/* IY */

extern z80t z80op_pop_iy(z80t);
extern z80t z80op_push_iy(z80t);

extern z80t z80op_jp_iy(z80t);
extern z80t z80op_ld_sp_iy(z80t);
extern z80t z80op_ex_isp_iy(z80t);
