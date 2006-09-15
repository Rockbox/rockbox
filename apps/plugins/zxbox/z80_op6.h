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

extern z80t z80op_rlc_b(z80t);
extern z80t z80op_rlc_c(z80t);
extern z80t z80op_rlc_d(z80t);
extern z80t z80op_rlc_e(z80t);
extern z80t z80op_rlc_h(z80t);
extern z80t z80op_rlc_l(z80t);
extern z80t z80op_rlc_ihl(z80t);
extern z80t z80op_rlc_a(z80t);
extern z80t z80op_rrc_b(z80t);
extern z80t z80op_rrc_c(z80t);
extern z80t z80op_rrc_d(z80t);
extern z80t z80op_rrc_e(z80t);
extern z80t z80op_rrc_h(z80t);
extern z80t z80op_rrc_l(z80t);
extern z80t z80op_rrc_ihl(z80t);
extern z80t z80op_rrc_a(z80t);

extern z80t z80op_rl_b(z80t);
extern z80t z80op_rl_c(z80t);
extern z80t z80op_rl_d(z80t);
extern z80t z80op_rl_e(z80t);
extern z80t z80op_rl_h(z80t);
extern z80t z80op_rl_l(z80t);
extern z80t z80op_rl_ihl(z80t);
extern z80t z80op_rl_a(z80t);
extern z80t z80op_rr_b(z80t);
extern z80t z80op_rr_c(z80t);
extern z80t z80op_rr_d(z80t);
extern z80t z80op_rr_e(z80t);
extern z80t z80op_rr_h(z80t);
extern z80t z80op_rr_l(z80t);
extern z80t z80op_rr_ihl(z80t);
extern z80t z80op_rr_a(z80t);

extern z80t z80op_sla_b(z80t);
extern z80t z80op_sla_c(z80t);
extern z80t z80op_sla_d(z80t);
extern z80t z80op_sla_e(z80t);
extern z80t z80op_sla_h(z80t);
extern z80t z80op_sla_l(z80t);
extern z80t z80op_sla_ihl(z80t);
extern z80t z80op_sla_a(z80t);
extern z80t z80op_sra_b(z80t);
extern z80t z80op_sra_c(z80t);
extern z80t z80op_sra_d(z80t);
extern z80t z80op_sra_e(z80t);
extern z80t z80op_sra_h(z80t);
extern z80t z80op_sra_l(z80t);
extern z80t z80op_sra_ihl(z80t);
extern z80t z80op_sra_a(z80t);

extern z80t z80op_sll_b(z80t);
extern z80t z80op_sll_c(z80t);
extern z80t z80op_sll_d(z80t);
extern z80t z80op_sll_e(z80t);
extern z80t z80op_sll_h(z80t);
extern z80t z80op_sll_l(z80t);
extern z80t z80op_sll_ihl(z80t);
extern z80t z80op_sll_a(z80t);
extern z80t z80op_srl_b(z80t);
extern z80t z80op_srl_c(z80t);
extern z80t z80op_srl_d(z80t);
extern z80t z80op_srl_e(z80t);
extern z80t z80op_srl_h(z80t);
extern z80t z80op_srl_l(z80t);
extern z80t z80op_srl_ihl(z80t);
extern z80t z80op_srl_a(z80t);

extern z80t z80op_bit_0_b(z80t);
extern z80t z80op_bit_0_c(z80t);
extern z80t z80op_bit_0_d(z80t);
extern z80t z80op_bit_0_e(z80t);
extern z80t z80op_bit_0_h(z80t);
extern z80t z80op_bit_0_l(z80t);
extern z80t z80op_bit_0_ihl(z80t);
extern z80t z80op_bit_0_a(z80t);
extern z80t z80op_bit_1_b(z80t);
extern z80t z80op_bit_1_c(z80t);
extern z80t z80op_bit_1_d(z80t);
extern z80t z80op_bit_1_e(z80t);
extern z80t z80op_bit_1_h(z80t);
extern z80t z80op_bit_1_l(z80t);
extern z80t z80op_bit_1_ihl(z80t);
extern z80t z80op_bit_1_a(z80t);

extern z80t z80op_bit_2_b(z80t);
extern z80t z80op_bit_2_c(z80t);
extern z80t z80op_bit_2_d(z80t);
extern z80t z80op_bit_2_e(z80t);
extern z80t z80op_bit_2_h(z80t);
extern z80t z80op_bit_2_l(z80t);
extern z80t z80op_bit_2_ihl(z80t);
extern z80t z80op_bit_2_a(z80t);
extern z80t z80op_bit_3_b(z80t);
extern z80t z80op_bit_3_c(z80t);
extern z80t z80op_bit_3_d(z80t);
extern z80t z80op_bit_3_e(z80t);
extern z80t z80op_bit_3_h(z80t);
extern z80t z80op_bit_3_l(z80t);
extern z80t z80op_bit_3_ihl(z80t);
extern z80t z80op_bit_3_a(z80t);

extern z80t z80op_bit_4_b(z80t);
extern z80t z80op_bit_4_c(z80t);
extern z80t z80op_bit_4_d(z80t);
extern z80t z80op_bit_4_e(z80t);
extern z80t z80op_bit_4_h(z80t);
extern z80t z80op_bit_4_l(z80t);
extern z80t z80op_bit_4_ihl(z80t);
extern z80t z80op_bit_4_a(z80t);
extern z80t z80op_bit_5_b(z80t);
extern z80t z80op_bit_5_c(z80t);
extern z80t z80op_bit_5_d(z80t);
extern z80t z80op_bit_5_e(z80t);
extern z80t z80op_bit_5_h(z80t);
extern z80t z80op_bit_5_l(z80t);
extern z80t z80op_bit_5_ihl(z80t);
extern z80t z80op_bit_5_a(z80t);

extern z80t z80op_bit_6_b(z80t);
extern z80t z80op_bit_6_c(z80t);
extern z80t z80op_bit_6_d(z80t);
extern z80t z80op_bit_6_e(z80t);
extern z80t z80op_bit_6_h(z80t);
extern z80t z80op_bit_6_l(z80t);
extern z80t z80op_bit_6_ihl(z80t);
extern z80t z80op_bit_6_a(z80t);
extern z80t z80op_bit_7_b(z80t);
extern z80t z80op_bit_7_c(z80t);
extern z80t z80op_bit_7_d(z80t);
extern z80t z80op_bit_7_e(z80t);
extern z80t z80op_bit_7_h(z80t);
extern z80t z80op_bit_7_l(z80t);
extern z80t z80op_bit_7_ihl(z80t);
extern z80t z80op_bit_7_a(z80t);

extern z80t z80op_res_0_b(z80t);
extern z80t z80op_res_0_c(z80t);
extern z80t z80op_res_0_d(z80t);
extern z80t z80op_res_0_e(z80t);
extern z80t z80op_res_0_h(z80t);
extern z80t z80op_res_0_l(z80t);
extern z80t z80op_res_0_ihl(z80t);
extern z80t z80op_res_0_a(z80t);
extern z80t z80op_res_1_b(z80t);
extern z80t z80op_res_1_c(z80t);
extern z80t z80op_res_1_d(z80t);
extern z80t z80op_res_1_e(z80t);
extern z80t z80op_res_1_h(z80t);
extern z80t z80op_res_1_l(z80t);
extern z80t z80op_res_1_ihl(z80t);
extern z80t z80op_res_1_a(z80t);

extern z80t z80op_res_2_b(z80t);
extern z80t z80op_res_2_c(z80t);
extern z80t z80op_res_2_d(z80t);
extern z80t z80op_res_2_e(z80t);
extern z80t z80op_res_2_h(z80t);
extern z80t z80op_res_2_l(z80t);
extern z80t z80op_res_2_ihl(z80t);
extern z80t z80op_res_2_a(z80t);
extern z80t z80op_res_3_b(z80t);
extern z80t z80op_res_3_c(z80t);
extern z80t z80op_res_3_d(z80t);
extern z80t z80op_res_3_e(z80t);
extern z80t z80op_res_3_h(z80t);
extern z80t z80op_res_3_l(z80t);
extern z80t z80op_res_3_ihl(z80t);
extern z80t z80op_res_3_a(z80t);

extern z80t z80op_res_4_b(z80t);
extern z80t z80op_res_4_c(z80t);
extern z80t z80op_res_4_d(z80t);
extern z80t z80op_res_4_e(z80t);
extern z80t z80op_res_4_h(z80t);
extern z80t z80op_res_4_l(z80t);
extern z80t z80op_res_4_ihl(z80t);
extern z80t z80op_res_4_a(z80t);
extern z80t z80op_res_5_b(z80t);
extern z80t z80op_res_5_c(z80t);
extern z80t z80op_res_5_d(z80t);
extern z80t z80op_res_5_e(z80t);
extern z80t z80op_res_5_h(z80t);
extern z80t z80op_res_5_l(z80t);
extern z80t z80op_res_5_ihl(z80t);
extern z80t z80op_res_5_a(z80t);

extern z80t z80op_res_6_b(z80t);
extern z80t z80op_res_6_c(z80t);
extern z80t z80op_res_6_d(z80t);
extern z80t z80op_res_6_e(z80t);
extern z80t z80op_res_6_h(z80t);
extern z80t z80op_res_6_l(z80t);
extern z80t z80op_res_6_ihl(z80t);
extern z80t z80op_res_6_a(z80t);
extern z80t z80op_res_7_b(z80t);
extern z80t z80op_res_7_c(z80t);
extern z80t z80op_res_7_d(z80t);
extern z80t z80op_res_7_e(z80t);
extern z80t z80op_res_7_h(z80t);
extern z80t z80op_res_7_l(z80t);
extern z80t z80op_res_7_ihl(z80t);
extern z80t z80op_res_7_a(z80t);

extern z80t z80op_set_0_b(z80t);
extern z80t z80op_set_0_c(z80t);
extern z80t z80op_set_0_d(z80t);
extern z80t z80op_set_0_e(z80t);
extern z80t z80op_set_0_h(z80t);
extern z80t z80op_set_0_l(z80t);
extern z80t z80op_set_0_ihl(z80t);
extern z80t z80op_set_0_a(z80t);
extern z80t z80op_set_1_b(z80t);
extern z80t z80op_set_1_c(z80t);
extern z80t z80op_set_1_d(z80t);
extern z80t z80op_set_1_e(z80t);
extern z80t z80op_set_1_h(z80t);
extern z80t z80op_set_1_l(z80t);
extern z80t z80op_set_1_ihl(z80t);
extern z80t z80op_set_1_a(z80t);

extern z80t z80op_set_2_b(z80t);
extern z80t z80op_set_2_c(z80t);
extern z80t z80op_set_2_d(z80t);
extern z80t z80op_set_2_e(z80t);
extern z80t z80op_set_2_h(z80t);
extern z80t z80op_set_2_l(z80t);
extern z80t z80op_set_2_ihl(z80t);
extern z80t z80op_set_2_a(z80t);
extern z80t z80op_set_3_b(z80t);
extern z80t z80op_set_3_c(z80t);
extern z80t z80op_set_3_d(z80t);
extern z80t z80op_set_3_e(z80t);
extern z80t z80op_set_3_h(z80t);
extern z80t z80op_set_3_l(z80t);
extern z80t z80op_set_3_ihl(z80t);
extern z80t z80op_set_3_a(z80t);

extern z80t z80op_set_4_b(z80t);
extern z80t z80op_set_4_c(z80t);
extern z80t z80op_set_4_d(z80t);
extern z80t z80op_set_4_e(z80t);
extern z80t z80op_set_4_h(z80t);
extern z80t z80op_set_4_l(z80t);
extern z80t z80op_set_4_ihl(z80t);
extern z80t z80op_set_4_a(z80t);
extern z80t z80op_set_5_b(z80t);
extern z80t z80op_set_5_c(z80t);
extern z80t z80op_set_5_d(z80t);
extern z80t z80op_set_5_e(z80t);
extern z80t z80op_set_5_h(z80t);
extern z80t z80op_set_5_l(z80t);
extern z80t z80op_set_5_ihl(z80t);
extern z80t z80op_set_5_a(z80t);

extern z80t z80op_set_6_b(z80t);
extern z80t z80op_set_6_c(z80t);
extern z80t z80op_set_6_d(z80t);
extern z80t z80op_set_6_e(z80t);
extern z80t z80op_set_6_h(z80t);
extern z80t z80op_set_6_l(z80t);
extern z80t z80op_set_6_ihl(z80t);
extern z80t z80op_set_6_a(z80t);
extern z80t z80op_set_7_b(z80t);
extern z80t z80op_set_7_c(z80t);
extern z80t z80op_set_7_d(z80t);
extern z80t z80op_set_7_e(z80t);
extern z80t z80op_set_7_h(z80t);
extern z80t z80op_set_7_l(z80t);
extern z80t z80op_set_7_ihl(z80t);
extern z80t z80op_set_7_a(z80t);
