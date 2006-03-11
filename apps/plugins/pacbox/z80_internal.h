/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef Z80_INTERNAL_H_
#define Z80_INTERNAL_H_

// Implementation of opcodes 0x00 to 0xFF
void opcode_00(void);   // NOP
void opcode_01(void);   // LD   BC,nn
void opcode_02(void);   // LD   (BC),A
void opcode_03(void);   // INC  BC
void opcode_04(void);   // INC  B
void opcode_05(void);   // DEC  B
void opcode_06(void);   // LD   B,n
void opcode_07(void);   // RLCA
void opcode_08(void);   // EX   AF,AF'
void opcode_09(void);   // ADD  HL,BC
void opcode_0a(void);   // LD   A,(BC)
void opcode_0b(void);   // DEC  BC
void opcode_0c(void);   // INC  C
void opcode_0d(void);   // DEC  C
void opcode_0e(void);   // LD   C,n
void opcode_0f(void);   // RRCA
void opcode_10(void);   // DJNZ d
void opcode_11(void);   // LD   DE,nn
void opcode_12(void);   // LD   (DE),A
void opcode_13(void);   // INC  DE
void opcode_14(void);   // INC  D
void opcode_15(void);   // DEC  D
void opcode_16(void);   // LD   D,n
void opcode_17(void);   // RLA
void opcode_18(void);   // JR   d
void opcode_19(void);   // ADD  HL,DE
void opcode_1a(void);   // LD   A,(DE)
void opcode_1b(void);   // DEC  DE
void opcode_1c(void);   // INC  E
void opcode_1d(void);   // DEC  E
void opcode_1e(void);   // LD   E,n
void opcode_1f(void);   // RRA
void opcode_20(void);   // JR   NZ,d
void opcode_21(void);   // LD   HL,nn
void opcode_22(void);   // LD   (nn),HL
void opcode_23(void);   // INC  HL
void opcode_24(void);   // INC  H
void opcode_25(void);   // DEC  H
void opcode_26(void);   // LD   H,n
void opcode_27(void);   // DAA
void opcode_28(void);   // JR   Z,d
void opcode_29(void);   // ADD  HL,HL
void opcode_2a(void);   // LD   HL,(nn)
void opcode_2b(void);   // DEC  HL
void opcode_2c(void);   // INC  L
void opcode_2d(void);   // DEC  L
void opcode_2e(void);   // LD   L,n
void opcode_2f(void);   // CPL
void opcode_30(void);   // JR   NC,d
void opcode_31(void);   // LD   SP,nn
void opcode_32(void);   // LD   (nn),A
void opcode_33(void);   // INC  SP
void opcode_34(void);   // INC  (HL)
void opcode_35(void);   // DEC  (HL)
void opcode_36(void);   // LD   (HL),n
void opcode_37(void);   // SCF
void opcode_38(void);   // JR   C,d
void opcode_39(void);   // ADD  HL,SP
void opcode_3a(void);   // LD   A,(nn)
void opcode_3b(void);   // DEC  SP
void opcode_3c(void);   // INC  A
void opcode_3d(void);   // DEC  A
void opcode_3e(void);   // LD   A,n
void opcode_3f(void);   // CCF
void opcode_40(void);   // LD   B,B
void opcode_41(void);   // LD   B,C
void opcode_42(void);   // LD   B,D
void opcode_43(void);   // LD   B,E
void opcode_44(void);   // LD   B,H
void opcode_45(void);   // LD   B,L
void opcode_46(void);   // LD   B,(HL)
void opcode_47(void);   // LD   B,A
void opcode_48(void);   // LD   C,B
void opcode_49(void);   // LD   C,C
void opcode_4a(void);   // LD   C,D
void opcode_4b(void);   // LD   C,E
void opcode_4c(void);   // LD   C,H
void opcode_4d(void);   // LD   C,L
void opcode_4e(void);   // LD   C,(HL)
void opcode_4f(void);   // LD   C,A
void opcode_50(void);   // LD   D,B
void opcode_51(void);   // LD   D,C
void opcode_52(void);   // LD   D,D
void opcode_53(void);   // LD   D,E
void opcode_54(void);   // LD   D,H
void opcode_55(void);   // LD   D,L
void opcode_56(void);   // LD   D,(HL)
void opcode_57(void);   // LD   D,A
void opcode_58(void);   // LD   E,B
void opcode_59(void);   // LD   E,C
void opcode_5a(void);   // LD   E,D
void opcode_5b(void);   // LD   E,E
void opcode_5c(void);   // LD   E,H
void opcode_5d(void);   // LD   E,L
void opcode_5e(void);   // LD   E,(HL)
void opcode_5f(void);   // LD   E,A
void opcode_60(void);   // LD   H,B
void opcode_61(void);   // LD   H,C
void opcode_62(void);   // LD   H,D
void opcode_63(void);   // LD   H,E
void opcode_64(void);   // LD   H,H
void opcode_65(void);   // LD   H,L
void opcode_66(void);   // LD   H,(HL)
void opcode_67(void);   // LD   H,A
void opcode_68(void);   // LD   L,B
void opcode_69(void);   // LD   L,C
void opcode_6a(void);   // LD   L,D
void opcode_6b(void);   // LD   L,E
void opcode_6c(void);   // LD   L,H
void opcode_6d(void);   // LD   L,L
void opcode_6e(void);   // LD   L,(HL)
void opcode_6f(void);   // LD   L,A
void opcode_70(void);   // LD   (HL),B
void opcode_71(void);   // LD   (HL),C
void opcode_72(void);   // LD   (HL),D
void opcode_73(void);   // LD   (HL),E
void opcode_74(void);   // LD   (HL),H
void opcode_75(void);   // LD   (HL),L
void opcode_76(void);   // HALT
void opcode_77(void);   // LD   (HL),A
void opcode_78(void);   // LD   A,B
void opcode_79(void);   // LD   A,C
void opcode_7a(void);   // LD   A,D
void opcode_7b(void);   // LD   A,E
void opcode_7c(void);   // LD   A,H
void opcode_7d(void);   // LD   A,L
void opcode_7e(void);   // LD   A,(HL)
void opcode_7f(void);   // LD   A,A
void opcode_80(void);   // ADD  A,B
void opcode_81(void);   // ADD  A,C
void opcode_82(void);   // ADD  A,D
void opcode_83(void);   // ADD  A,E
void opcode_84(void);   // ADD  A,H
void opcode_85(void);   // ADD  A,L
void opcode_86(void);   // ADD  A,(HL)
void opcode_87(void);   // ADD  A,A
void opcode_88(void);   // ADC  A,B
void opcode_89(void);   // ADC  A,C
void opcode_8a(void);   // ADC  A,D
void opcode_8b(void);   // ADC  A,E
void opcode_8c(void);   // ADC  A,H
void opcode_8d(void);   // ADC  A,L
void opcode_8e(void);   // ADC  A,(HL)
void opcode_8f(void);   // ADC  A,A
void opcode_90(void);   // SUB  B
void opcode_91(void);   // SUB  C
void opcode_92(void);   // SUB  D
void opcode_93(void);   // SUB  E
void opcode_94(void);   // SUB  H
void opcode_95(void);   // SUB  L
void opcode_96(void);   // SUB  (HL)
void opcode_97(void);   // SUB  A
void opcode_98(void);   // SBC  A,B
void opcode_99(void);   // SBC  A,C
void opcode_9a(void);   // SBC  A,D
void opcode_9b(void);   // SBC  A,E
void opcode_9c(void);   // SBC  A,H
void opcode_9d(void);   // SBC  A,L
void opcode_9e(void);   // SBC  A,(HL)
void opcode_9f(void);   // SBC  A,A
void opcode_a0(void);   // AND  B
void opcode_a1(void);   // AND  C
void opcode_a2(void);   // AND  D
void opcode_a3(void);   // AND  E
void opcode_a4(void);   // AND  H
void opcode_a5(void);   // AND  L
void opcode_a6(void);   // AND  (HL)
void opcode_a7(void);   // AND  A
void opcode_a8(void);   // XOR  B
void opcode_a9(void);   // XOR  C
void opcode_aa(void);   // XOR  D
void opcode_ab(void);   // XOR  E
void opcode_ac(void);   // XOR  H
void opcode_ad(void);   // XOR  L
void opcode_ae(void);   // XOR  (HL)
void opcode_af(void);   // XOR  A
void opcode_b0(void);   // OR   B
void opcode_b1(void);   // OR   C
void opcode_b2(void);   // OR   D
void opcode_b3(void);   // OR   E
void opcode_b4(void);   // OR   H
void opcode_b5(void);   // OR   L
void opcode_b6(void);   // OR   (HL)
void opcode_b7(void);   // OR   A
void opcode_b8(void);   // CP   B
void opcode_b9(void);   // CP   C
void opcode_ba(void);   // CP   D
void opcode_bb(void);   // CP   E
void opcode_bc(void);   // CP   H
void opcode_bd(void);   // CP   L
void opcode_be(void);   // CP   (HL)
void opcode_bf(void);   // CP   A
void opcode_c0(void);   // RET  NZ
void opcode_c1(void);   // POP  BC
void opcode_c2(void);   // JP   NZ,nn
void opcode_c3(void);   // JP   nn
void opcode_c4(void);   // CALL NZ,nn
void opcode_c5(void);   // PUSH BC
void opcode_c6(void);   // ADD  A,n
void opcode_c7(void);   // RST  0
void opcode_c8(void);   // RET  Z
void opcode_c9(void);   // RET
void opcode_ca(void);   // JP   Z,nn
void opcode_cb(void);   // [Prefix]
void opcode_cc(void);   // CALL Z,nn
void opcode_cd(void);   // CALL nn
void opcode_ce(void);   // ADC  A,n
void opcode_cf(void);   // RST  8
void opcode_d0(void);   // RET  NC
void opcode_d1(void);   // POP  DE
void opcode_d2(void);   // JP   NC,nn
void opcode_d3(void);   // OUT  (n),A
void opcode_d4(void);   // CALL NC,nn
void opcode_d5(void);   // PUSH DE
void opcode_d6(void);   // SUB  n
void opcode_d7(void);   // RST  10H
void opcode_d8(void);   // RET  C
void opcode_d9(void);   // EXX
void opcode_da(void);   // JP   C,nn
void opcode_db(void);   // IN   A,(n)
void opcode_dc(void);   // CALL C,nn
void opcode_dd(void);   // [IX Prefix]
void opcode_de(void);   // SBC  A,n
void opcode_df(void);   // RST  18H
void opcode_e0(void);   // RET  PO
void opcode_e1(void);   // POP  HL
void opcode_e2(void);   // JP   PO,nn
void opcode_e3(void);   // EX   (SP),HL
void opcode_e4(void);   // CALL PO,nn
void opcode_e5(void);   // PUSH HL
void opcode_e6(void);   // AND  n
void opcode_e7(void);   // RST  20H
void opcode_e8(void);   // RET  PE
void opcode_e9(void);   // JP   (HL)
void opcode_ea(void);   // JP   PE,nn
void opcode_eb(void);   // EX   DE,HL
void opcode_ec(void);   // CALL PE,nn
void opcode_ed(void);   // [Prefix]
void opcode_ee(void);   // XOR  n
void opcode_ef(void);   // RST  28H
void opcode_f0(void);   // RET  P
void opcode_f1(void);   // POP  AF
void opcode_f2(void);   // JP   P,nn
void opcode_f3(void);   // DI
void opcode_f4(void);   // CALL P,nn
void opcode_f5(void);   // PUSH AF
void opcode_f6(void);   // OR   n
void opcode_f7(void);   // RST  30H
void opcode_f8(void);   // RET  M
void opcode_f9(void);   // LD   SP,HL
void opcode_fa(void);   // JP   M,nn
void opcode_fb(void);   // EI
void opcode_fc(void);   // CALL M,nn
void opcode_fd(void);   // [IY Prefix]
void opcode_fe(void);   // CP   n
void opcode_ff(void);   // RST  38H
                    
// Handlers for the 0xCB prefix
void opcode_cb_00(void);    // RLC B
void opcode_cb_01(void);    // RLC C
void opcode_cb_02(void);    // RLC D
void opcode_cb_03(void);    // RLC E
void opcode_cb_04(void);    // RLC H
void opcode_cb_05(void);    // RLC L
void opcode_cb_06(void);    // RLC (HL)
void opcode_cb_07(void);    // RLC A
void opcode_cb_08(void);    // RRC B
void opcode_cb_09(void);    // RRC C
void opcode_cb_0a(void);    // RRC D
void opcode_cb_0b(void);    // RRC E
void opcode_cb_0c(void);    // RRC H
void opcode_cb_0d(void);    // RRC L
void opcode_cb_0e(void);    // RRC (HL)
void opcode_cb_0f(void);    // RRC A
void opcode_cb_10(void);    // RL B
void opcode_cb_11(void);    // RL C
void opcode_cb_12(void);    // RL D
void opcode_cb_13(void);    // RL E
void opcode_cb_14(void);    // RL H
void opcode_cb_15(void);    // RL L
void opcode_cb_16(void);    // RL (HL)
void opcode_cb_17(void);    // RL A
void opcode_cb_18(void);    // RR B
void opcode_cb_19(void);    // RR C
void opcode_cb_1a(void);    // RR D
void opcode_cb_1b(void);    // RR E
void opcode_cb_1c(void);    // RR H
void opcode_cb_1d(void);    // RR L
void opcode_cb_1e(void);    // RR (HL)
void opcode_cb_1f(void);    // RR A
void opcode_cb_20(void);    // SLA B
void opcode_cb_21(void);    // SLA C
void opcode_cb_22(void);    // SLA D
void opcode_cb_23(void);    // SLA E
void opcode_cb_24(void);    // SLA H
void opcode_cb_25(void);    // SLA L
void opcode_cb_26(void);    // SLA (HL)
void opcode_cb_27(void);    // SLA A
void opcode_cb_28(void);    // SRA B
void opcode_cb_29(void);    // SRA C
void opcode_cb_2a(void);    // SRA D
void opcode_cb_2b(void);    // SRA E
void opcode_cb_2c(void);    // SRA H
void opcode_cb_2d(void);    // SRA L
void opcode_cb_2e(void);    // SRA (HL)
void opcode_cb_2f(void);    // SRA A
void opcode_cb_30(void);    // SLL B    [undocumented]
void opcode_cb_31(void);    // SLL C    [undocumented]
void opcode_cb_32(void);    // SLL D    [undocumented]
void opcode_cb_33(void);    // SLL E    [undocumented]
void opcode_cb_34(void);    // SLL H    [undocumented]
void opcode_cb_35(void);    // SLL L    [undocumented]
void opcode_cb_36(void);    // SLL (HL) [undocumented]
void opcode_cb_37(void);    // SLL A    [undocumented]
void opcode_cb_38(void);    // SRL B
void opcode_cb_39(void);    // SRL C
void opcode_cb_3a(void);    // SRL D
void opcode_cb_3b(void);    // SRL E
void opcode_cb_3c(void);    // SRL H
void opcode_cb_3d(void);    // SRL L
void opcode_cb_3e(void);    // SRL (HL)
void opcode_cb_3f(void);    // SRL A
void opcode_cb_40(void);    // BIT 0, B
void opcode_cb_41(void);    // BIT 0, C
void opcode_cb_42(void);    // BIT 0, D
void opcode_cb_43(void);    // BIT 0, E
void opcode_cb_44(void);    // BIT 0, H
void opcode_cb_45(void);    // BIT 0, L
void opcode_cb_46(void);    // BIT 0, (HL)
void opcode_cb_47(void);    // BIT 0, A
void opcode_cb_48(void);    // BIT 1, B
void opcode_cb_49(void);    // BIT 1, C
void opcode_cb_4a(void);    // BIT 1, D
void opcode_cb_4b(void);    // BIT 1, E
void opcode_cb_4c(void);    // BIT 1, H
void opcode_cb_4d(void);    // BIT 1, L
void opcode_cb_4e(void);    // BIT 1, (HL)
void opcode_cb_4f(void);    // BIT 1, A
void opcode_cb_50(void);    // BIT 2, B
void opcode_cb_51(void);    // BIT 2, C
void opcode_cb_52(void);    // BIT 2, D
void opcode_cb_53(void);    // BIT 2, E
void opcode_cb_54(void);    // BIT 2, H
void opcode_cb_55(void);    // BIT 2, L
void opcode_cb_56(void);    // BIT 2, (HL)
void opcode_cb_57(void);    // BIT 2, A
void opcode_cb_58(void);    // BIT 3, B
void opcode_cb_59(void);    // BIT 3, C
void opcode_cb_5a(void);    // BIT 3, D
void opcode_cb_5b(void);    // BIT 3, E
void opcode_cb_5c(void);    // BIT 3, H
void opcode_cb_5d(void);    // BIT 3, L
void opcode_cb_5e(void);    // BIT 3, (HL)
void opcode_cb_5f(void);    // BIT 3, A
void opcode_cb_60(void);    // BIT 4, B
void opcode_cb_61(void);    // BIT 4, C
void opcode_cb_62(void);    // BIT 4, D
void opcode_cb_63(void);    // BIT 4, E
void opcode_cb_64(void);    // BIT 4, H
void opcode_cb_65(void);    // BIT 4, L
void opcode_cb_66(void);    // BIT 4, (HL)
void opcode_cb_67(void);    // BIT 4, A
void opcode_cb_68(void);    // BIT 5, B
void opcode_cb_69(void);    // BIT 5, C
void opcode_cb_6a(void);    // BIT 5, D
void opcode_cb_6b(void);    // BIT 5, E
void opcode_cb_6c(void);    // BIT 5, H
void opcode_cb_6d(void);    // BIT 5, L
void opcode_cb_6e(void);    // BIT 5, (HL)
void opcode_cb_6f(void);    // BIT 5, A
void opcode_cb_70(void);    // BIT 6, B
void opcode_cb_71(void);    // BIT 6, C
void opcode_cb_72(void);    // BIT 6, D
void opcode_cb_73(void);    // BIT 6, E
void opcode_cb_74(void);    // BIT 6, H
void opcode_cb_75(void);    // BIT 6, L
void opcode_cb_76(void);    // BIT 6, (HL)
void opcode_cb_77(void);    // BIT 6, A
void opcode_cb_78(void);    // BIT 7, B
void opcode_cb_79(void);    // BIT 7, C
void opcode_cb_7a(void);    // BIT 7, D
void opcode_cb_7b(void);    // BIT 7, E
void opcode_cb_7c(void);    // BIT 7, H
void opcode_cb_7d(void);    // BIT 7, L
void opcode_cb_7e(void);    // BIT 7, (HL)
void opcode_cb_7f(void);    // BIT 7, A
void opcode_cb_80(void);    // RES 0, B
void opcode_cb_81(void);    // RES 0, C
void opcode_cb_82(void);    // RES 0, D
void opcode_cb_83(void);    // RES 0, E
void opcode_cb_84(void);    // RES 0, H
void opcode_cb_85(void);    // RES 0, L
void opcode_cb_86(void);    // RES 0, (HL)
void opcode_cb_87(void);    // RES 0, A
void opcode_cb_88(void);    // RES 1, B
void opcode_cb_89(void);    // RES 1, C
void opcode_cb_8a(void);    // RES 1, D
void opcode_cb_8b(void);    // RES 1, E
void opcode_cb_8c(void);    // RES 1, H
void opcode_cb_8d(void);    // RES 1, L
void opcode_cb_8e(void);    // RES 1, (HL)
void opcode_cb_8f(void);    // RES 1, A
void opcode_cb_90(void);    // RES 2, B
void opcode_cb_91(void);    // RES 2, C
void opcode_cb_92(void);    // RES 2, D
void opcode_cb_93(void);    // RES 2, E
void opcode_cb_94(void);    // RES 2, H
void opcode_cb_95(void);    // RES 2, L
void opcode_cb_96(void);    // RES 2, (HL)
void opcode_cb_97(void);    // RES 2, A
void opcode_cb_98(void);    // RES 3, B
void opcode_cb_99(void);    // RES 3, C
void opcode_cb_9a(void);    // RES 3, D
void opcode_cb_9b(void);    // RES 3, E
void opcode_cb_9c(void);    // RES 3, H
void opcode_cb_9d(void);    // RES 3, L
void opcode_cb_9e(void);    // RES 3, (HL)
void opcode_cb_9f(void);    // RES 3, A
void opcode_cb_a0(void);    // RES 4, B
void opcode_cb_a1(void);    // RES 4, C
void opcode_cb_a2(void);    // RES 4, D
void opcode_cb_a3(void);    // RES 4, E
void opcode_cb_a4(void);    // RES 4, H
void opcode_cb_a5(void);    // RES 4, L
void opcode_cb_a6(void);    // RES 4, (HL)
void opcode_cb_a7(void);    // RES 4, A
void opcode_cb_a8(void);    // RES 5, B
void opcode_cb_a9(void);    // RES 5, C
void opcode_cb_aa(void);    // RES 5, D
void opcode_cb_ab(void);    // RES 5, E
void opcode_cb_ac(void);    // RES 5, H
void opcode_cb_ad(void);    // RES 5, L
void opcode_cb_ae(void);    // RES 5, (HL)
void opcode_cb_af(void);    // RES 5, A
void opcode_cb_b0(void);    // RES 6, B
void opcode_cb_b1(void);    // RES 6, C
void opcode_cb_b2(void);    // RES 6, D
void opcode_cb_b3(void);    // RES 6, E
void opcode_cb_b4(void);    // RES 6, H
void opcode_cb_b5(void);    // RES 6, L
void opcode_cb_b6(void);    // RES 6, (HL)
void opcode_cb_b7(void);    // RES 6, A
void opcode_cb_b8(void);    // RES 7, B
void opcode_cb_b9(void);    // RES 7, C
void opcode_cb_ba(void);    // RES 7, D
void opcode_cb_bb(void);    // RES 7, E
void opcode_cb_bc(void);    // RES 7, H
void opcode_cb_bd(void);    // RES 7, L
void opcode_cb_be(void);    // RES 7, (HL)
void opcode_cb_bf(void);    // RES 7, A
void opcode_cb_c0(void);    // SET 0, B
void opcode_cb_c1(void);    // SET 0, C
void opcode_cb_c2(void);    // SET 0, D
void opcode_cb_c3(void);    // SET 0, E
void opcode_cb_c4(void);    // SET 0, H
void opcode_cb_c5(void);    // SET 0, L
void opcode_cb_c6(void);    // SET 0, (HL)
void opcode_cb_c7(void);    // SET 0, A
void opcode_cb_c8(void);    // SET 1, B
void opcode_cb_c9(void);    // SET 1, C
void opcode_cb_ca(void);    // SET 1, D
void opcode_cb_cb(void);    // SET 1, E
void opcode_cb_cc(void);    // SET 1, H
void opcode_cb_cd(void);    // SET 1, L
void opcode_cb_ce(void);    // SET 1, (HL)
void opcode_cb_cf(void);    // SET 1, A
void opcode_cb_d0(void);    // SET 2, B
void opcode_cb_d1(void);    // SET 2, C
void opcode_cb_d2(void);    // SET 2, D
void opcode_cb_d3(void);    // SET 2, E
void opcode_cb_d4(void);    // SET 2, H
void opcode_cb_d5(void);    // SET 2, L
void opcode_cb_d6(void);    // SET 2, (HL)
void opcode_cb_d7(void);    // SET 2, A
void opcode_cb_d8(void);    // SET 3, B
void opcode_cb_d9(void);    // SET 3, C
void opcode_cb_da(void);    // SET 3, D
void opcode_cb_db(void);    // SET 3, E
void opcode_cb_dc(void);    // SET 3, H
void opcode_cb_dd(void);    // SET 3, L
void opcode_cb_de(void);    // SET 3, (HL)
void opcode_cb_df(void);    // SET 3, A
void opcode_cb_e0(void);    // SET 4, B
void opcode_cb_e1(void);    // SET 4, C
void opcode_cb_e2(void);    // SET 4, D
void opcode_cb_e3(void);    // SET 4, E
void opcode_cb_e4(void);    // SET 4, H
void opcode_cb_e5(void);    // SET 4, L
void opcode_cb_e6(void);    // SET 4, (HL)
void opcode_cb_e7(void);    // SET 4, A
void opcode_cb_e8(void);    // SET 5, B
void opcode_cb_e9(void);    // SET 5, C
void opcode_cb_ea(void);    // SET 5, D
void opcode_cb_eb(void);    // SET 5, E
void opcode_cb_ec(void);    // SET 5, H
void opcode_cb_ed(void);    // SET 5, L
void opcode_cb_ee(void);    // SET 5, (HL)
void opcode_cb_ef(void);    // SET 5, A
void opcode_cb_f0(void);    // SET 6, B
void opcode_cb_f1(void);    // SET 6, C
void opcode_cb_f2(void);    // SET 6, D
void opcode_cb_f3(void);    // SET 6, E
void opcode_cb_f4(void);    // SET 6, H
void opcode_cb_f5(void);    // SET 6, L
void opcode_cb_f6(void);    // SET 6, (HL)
void opcode_cb_f7(void);    // SET 6, A
void opcode_cb_f8(void);    // SET 7, B
void opcode_cb_f9(void);    // SET 7, C
void opcode_cb_fa(void);    // SET 7, D
void opcode_cb_fb(void);    // SET 7, E
void opcode_cb_fc(void);    // SET 7, H
void opcode_cb_fd(void);    // SET 7, L
void opcode_cb_fe(void);    // SET 7, (HL)
void opcode_cb_ff(void);    // SET 7, A

// Handlers for the 0xED prefix
void opcode_ed_40(void);    // IN B, (C)
void opcode_ed_41(void);    // OUT (C), B
void opcode_ed_42(void);    // SBC HL, BC
void opcode_ed_43(void);    // LD (nn), BC
void opcode_ed_44(void);    // NEG
void opcode_ed_45(void);    // RETN
void opcode_ed_46(void);    // IM 0
void opcode_ed_47(void);    // LD I, A
void opcode_ed_48(void);    // IN C, (C)
void opcode_ed_49(void);    // OUT (C), C
void opcode_ed_4a(void);    // ADC HL, BC
void opcode_ed_4b(void);    // LD BC, (nn)
void opcode_ed_4c(void);    // NEG      [undocumented]
void opcode_ed_4d(void);    // RETI
void opcode_ed_4e(void);    // IM 0/1   [undocumented]
void opcode_ed_4f(void);    // LD R, A
void opcode_ed_50(void);    // IN D, (C)
void opcode_ed_51(void);    // OUT (C), D
void opcode_ed_52(void);    // SBC HL, DE
void opcode_ed_53(void);    // LD (nn), DE
void opcode_ed_54(void);    // NEG      [undocumented]
void opcode_ed_55(void);    // RETN     [undocumented]
void opcode_ed_56(void);    // IM 1
void opcode_ed_57(void);    // LD A, I
void opcode_ed_58(void);    // IN E, (C)
void opcode_ed_59(void);    // OUT (C), E
void opcode_ed_5a(void);    // ADC HL, DE
void opcode_ed_5b(void);    // LD DE, (nn)
void opcode_ed_5c(void);    // NEG      [undocumented]
void opcode_ed_5d(void);    // RETN     [undocumented]
void opcode_ed_5e(void);    // IM 2
void opcode_ed_5f(void);    // LD A, R
void opcode_ed_60(void);    // IN H, (C)
void opcode_ed_61(void);    // OUT (C), H
void opcode_ed_62(void);    // SBC HL, HL
void opcode_ed_63(void);    // LD (nn), HL
void opcode_ed_64(void);    // NEG      [undocumented]
void opcode_ed_65(void);    // RETN     [undocumented]
void opcode_ed_66(void);    // IM 0     [undocumented]
void opcode_ed_67(void);    // RRD
void opcode_ed_68(void);    // IN L, (C)
void opcode_ed_69(void);    // OUT (C), L
void opcode_ed_6a(void);    // ADC HL, HL
void opcode_ed_6b(void);    // LD HL, (nn)
void opcode_ed_6c(void);    // NEG      [undocumented]
void opcode_ed_6d(void);    // RETN     [undocumented]
void opcode_ed_6e(void);    // IM 0/1   [undocumented]
void opcode_ed_6f(void);    // RLD
void opcode_ed_70(void);    // IN (C)/IN F, (C) [undocumented]
void opcode_ed_71(void);    // OUT (C), 0       [undocumented]
void opcode_ed_72(void);    // SBC HL, SP
void opcode_ed_73(void);    // LD (nn), SP
void opcode_ed_74(void);    // NEG      [undocumented]
void opcode_ed_75(void);    // RETN     [undocumented]
void opcode_ed_76(void);    // IM 1     [undocumented]
void opcode_ed_78(void);    // IN A, (C)
void opcode_ed_79(void);    // OUT (C), A
void opcode_ed_7a(void);    // ADC HL, SP
void opcode_ed_7b(void);    // nLD SP, (nn)
void opcode_ed_7c(void);    // NEG      [undocumented]
void opcode_ed_7d(void);    // RETN     [undocumented]
void opcode_ed_7e(void);    // IM 2     [undocumented]
void opcode_ed_a0(void);    // LDI
void opcode_ed_a1(void);    // CPI
void opcode_ed_a2(void);    // INI
void opcode_ed_a3(void);    // OUTI
void opcode_ed_a8(void);    // LDD
void opcode_ed_a9(void);    // CPD
void opcode_ed_aa(void);    // IND
void opcode_ed_ab(void);    // OUTD
void opcode_ed_b0(void);    // LDIR
void opcode_ed_b1(void);    // CPIR
void opcode_ed_b2(void);    // INIR
void opcode_ed_b3(void);    // OTIR
void opcode_ed_b8(void);    // LDDR
void opcode_ed_b9(void);    // CPDR
void opcode_ed_ba(void);    // INDR
void opcode_ed_bb(void);    // OTDR

// Handlers for the 0xDD prefix (IX)
void opcode_dd_09(void);    // ADD IX, BC
void opcode_dd_19(void);    // ADD IX, DE
void opcode_dd_21(void);    // LD IX, nn
void opcode_dd_22(void);    // LD (nn), IX
void opcode_dd_23(void);    // INC IX
void opcode_dd_24(void);    // INC IXH      [undocumented]
void opcode_dd_25(void);    // DEC IXH      [undocumented]
void opcode_dd_26(void);    // LD IXH, n    [undocumented]
void opcode_dd_29(void);    // ADD IX, IX
void opcode_dd_2a(void);    // LD IX, (nn)
void opcode_dd_2b(void);    // DEC IX
void opcode_dd_2c(void);    // INC IXL      [undocumented]
void opcode_dd_2d(void);    // DEC IXL      [undocumented]
void opcode_dd_2e(void);    // LD IXL, n    [undocumented]
void opcode_dd_34(void);    // INC (IX + d)
void opcode_dd_35(void);    // DEC (IX + d)
void opcode_dd_36(void);    // LD (IX + d), n
void opcode_dd_39(void);    // ADD IX, SP
void opcode_dd_44(void);    // LD B, IXH    [undocumented]
void opcode_dd_45(void);    // LD B, IXL    [undocumented]
void opcode_dd_46(void);    // LD B, (IX + d)
void opcode_dd_4c(void);    // LD C, IXH    [undocumented]
void opcode_dd_4d(void);    // LD C, IXL    [undocumented]
void opcode_dd_4e(void);    // LD C, (IX + d)
void opcode_dd_54(void);    // LD D, IXH    [undocumented]
void opcode_dd_55(void);    // LD D, IXL    [undocumented]
void opcode_dd_56(void);    // LD D, (IX + d)
void opcode_dd_5c(void);    // LD E, IXH    [undocumented]
void opcode_dd_5d(void);    // LD E, IXL    [undocumented]
void opcode_dd_5e(void);    // LD E, (IX + d)
void opcode_dd_60(void);    // LD IXH, B    [undocumented]
void opcode_dd_61(void);    // LD IXH, C    [undocumented]
void opcode_dd_62(void);    // LD IXH, D    [undocumented]
void opcode_dd_63(void);    // LD IXH, E    [undocumented]
void opcode_dd_64(void);    // LD IXH, IXH  [undocumented]
void opcode_dd_65(void);    // LD IXH, IXL  [undocumented]
void opcode_dd_66(void);    // LD H, (IX + d)
void opcode_dd_67(void);    // LD IXH, A    [undocumented]
void opcode_dd_68(void);    // LD IXL, B    [undocumented]
void opcode_dd_69(void);    // LD IXL, C    [undocumented]
void opcode_dd_6a(void);    // LD IXL, D    [undocumented]
void opcode_dd_6b(void);    // LD IXL, E    [undocumented]
void opcode_dd_6c(void);    // LD IXL, IXH  [undocumented]
void opcode_dd_6d(void);    // LD IXL, IXL  [undocumented]
void opcode_dd_6e(void);    // LD L, (IX + d)
void opcode_dd_6f(void);    // LD IXL, A    [undocumented]
void opcode_dd_70(void);    // LD (IX + d), B
void opcode_dd_71(void);    // LD (IX + d), C
void opcode_dd_72(void);    // LD (IX + d), D
void opcode_dd_73(void);    // LD (IX + d), E
void opcode_dd_74(void);    // LD (IX + d), H
void opcode_dd_75(void);    // LD (IX + d), L
void opcode_dd_77(void);    // LD (IX + d), A
void opcode_dd_7c(void);    // LD A, IXH    [undocumented]
void opcode_dd_7d(void);    // LD A, IXL    [undocumented]
void opcode_dd_7e(void);    // LD A, (IX + d)
void opcode_dd_84(void);    // ADD A, IXH   [undocumented]
void opcode_dd_85(void);    // ADD A, IXL   [undocumented]
void opcode_dd_86(void);    // ADD A, (IX + d)
void opcode_dd_8c(void);    // ADC A, IXH   [undocumented]
void opcode_dd_8d(void);    // ADC A, IXL   [undocumented]
void opcode_dd_8e(void);    // ADC A, (IX + d)
void opcode_dd_94(void);    // SUB IXH      [undocumented]
void opcode_dd_95(void);    // SUB IXL      [undocumented]
void opcode_dd_96(void);    // SUB (IX + d)
void opcode_dd_9c(void);    // SBC A, IXH   [undocumented]
void opcode_dd_9d(void);    // SBC A, IXL   [undocumented]
void opcode_dd_9e(void);    // SBC A, (IX + d)
void opcode_dd_a4(void);    // AND IXH      [undocumented]
void opcode_dd_a5(void);    // AND IXL      [undocumented]
void opcode_dd_a6(void);    // AND (IX + d)
void opcode_dd_ac(void);    // XOR IXH      [undocumented]
void opcode_dd_ad(void);    // XOR IXL      [undocumented]
void opcode_dd_ae(void);    // XOR (IX + d)
void opcode_dd_b4(void);    // OR IXH       [undocumented]
void opcode_dd_b5(void);    // OR IXL       [undocumented]
void opcode_dd_b6(void);    // OR (IX + d)
void opcode_dd_bc(void);    // CP IXH       [undocumented]
void opcode_dd_bd(void);    // CP IXL       [undocumented]
void opcode_dd_be(void);    // CP (IX + d)
void opcode_dd_cb(void);    // 
void opcode_dd_e1(void);    // POP IX
void opcode_dd_e3(void);    // EX (SP), IX
void opcode_dd_e5(void);    // PUSH IX
void opcode_dd_e9(void);    // JP (IX)
void opcode_dd_f9(void);    // LD SP, IX

// Handlers for the 0xFD prefix (IY)
void opcode_fd_09(void);    // ADD IY, BC
void opcode_fd_19(void);    // ADD IY, DE
void opcode_fd_21(void);    // LD IY, nn
void opcode_fd_22(void);    // LD (nn), IY
void opcode_fd_23(void);    // INC IY
void opcode_fd_24(void);    // INC IYH      [undocumented]
void opcode_fd_25(void);    // DEC IYH      [undocumented]
void opcode_fd_26(void);    // LD IYH, n    [undocumented]
void opcode_fd_29(void);    // ADD IY, IY
void opcode_fd_2a(void);    // LD IY, (nn)
void opcode_fd_2b(void);    // DEC IY
void opcode_fd_2c(void);    // INC IYL      [undocumented]
void opcode_fd_2d(void);    // DEC IYL      [undocumented]
void opcode_fd_2e(void);    // LD IYL, n    [undocumented]
void opcode_fd_34(void);    // INC (IY + d)
void opcode_fd_35(void);    // DEC (IY + d)
void opcode_fd_36(void);    // LD (IY + d), n
void opcode_fd_39(void);    // ADD IY, SP
void opcode_fd_44(void);    // LD B, IYH    [undocumented]
void opcode_fd_45(void);    // LD B, IYL    [undocumented]
void opcode_fd_46(void);    // LD B, (IY + d)
void opcode_fd_4c(void);    // LD C, IYH    [undocumented]
void opcode_fd_4d(void);    // LD C, IYL    [undocumented]
void opcode_fd_4e(void);    // LD C, (IY + d)
void opcode_fd_54(void);    // LD D, IYH    [undocumented]
void opcode_fd_55(void);    // LD D, IYL    [undocumented]
void opcode_fd_56(void);    // LD D, (IY + d)
void opcode_fd_5c(void);    // LD E, IYH    [undocumented]
void opcode_fd_5d(void);    // LD E, IYL    [undocumented]
void opcode_fd_5e(void);    // LD E, (IY + d)
void opcode_fd_60(void);    // LD IYH, B    [undocumented]
void opcode_fd_61(void);    // LD IYH, C    [undocumented]
void opcode_fd_62(void);    // LD IYH, D    [undocumented]
void opcode_fd_63(void);    // LD IYH, E    [undocumented]
void opcode_fd_64(void);    // LD IYH, IYH  [undocumented]
void opcode_fd_65(void);    // LD IYH, IYL  [undocumented]
void opcode_fd_66(void);    // LD H, (IY + d)
void opcode_fd_67(void);    // LD IYH, A    [undocumented]
void opcode_fd_68(void);    // LD IYL, B    [undocumented]
void opcode_fd_69(void);    // LD IYL, C    [undocumented]
void opcode_fd_6a(void);    // LD IYL, D    [undocumented]
void opcode_fd_6b(void);    // LD IYL, E    [undocumented]
void opcode_fd_6c(void);    // LD IYL, IYH  [undocumented]
void opcode_fd_6d(void);    // LD IYL, IYL  [undocumented]
void opcode_fd_6e(void);    // LD L, (IY + d)
void opcode_fd_6f(void);    // LD IYL, A    [undocumented]
void opcode_fd_70(void);    // LD (IY + d), B
void opcode_fd_71(void);    // LD (IY + d), C
void opcode_fd_72(void);    // LD (IY + d), D
void opcode_fd_73(void);    // LD (IY + d), E
void opcode_fd_74(void);    // LD (IY + d), H
void opcode_fd_75(void);    // LD (IY + d), L
void opcode_fd_77(void);    // LD (IY + d), A
void opcode_fd_7c(void);    // LD A, IYH    [undocumented]
void opcode_fd_7d(void);    // LD A, IYL    [undocumented]
void opcode_fd_7e(void);    // LD A, (IY + d)
void opcode_fd_84(void);    // ADD A, IYH   [undocumented]
void opcode_fd_85(void);    // ADD A, IYL   [undocumented]
void opcode_fd_86(void);    // ADD A, (IY + d)
void opcode_fd_8c(void);    // ADC A, IYH   [undocumented]
void opcode_fd_8d(void);    // ADC A, IYL   [undocumented]
void opcode_fd_8e(void);    // ADC A, (IY + d)
void opcode_fd_94(void);    // SUB IYH      [undocumented]
void opcode_fd_95(void);    // SUB IYL      [undocumented]
void opcode_fd_96(void);    // SUB (IY + d)
void opcode_fd_9c(void);    // SBC A, IYH   [undocumented]
void opcode_fd_9d(void);    // SBC A, IYL   [undocumented]
void opcode_fd_9e(void);    // SBC A, (IY + d)
void opcode_fd_a4(void);    // AND IYH      [undocumented]
void opcode_fd_a5(void);    // AND IYL      [undocumented]
void opcode_fd_a6(void);    // AND (IY + d)
void opcode_fd_ac(void);    // XOR IYH      [undocumented]
void opcode_fd_ad(void);    // XOR IYL      [undocumented]
void opcode_fd_ae(void);    // XOR (IY + d)
void opcode_fd_b4(void);    // OR IYH       [undocumented]
void opcode_fd_b5(void);    // OR IYL       [undocumented]
void opcode_fd_b6(void);    // OR (IY + d)
void opcode_fd_bc(void);    // CP IYH       [undocumented]
void opcode_fd_bd(void);    // CP IYL       [undocumented]
void opcode_fd_be(void);    // CP (IY + d)
void opcode_fd_cb(void);    // 
void opcode_fd_e1(void);    // POP IY
void opcode_fd_e3(void);    // EX (SP), IY
void opcode_fd_e5(void);    // PUSH IY
void opcode_fd_e9(void);    // JP (IY)
void opcode_fd_f9(void);    // LD SP, IY

// Handlers for 0xDDCB and 0xFDCB prefixes
void opcode_xycb_00( unsigned );    // LD B, RLC (IX + d)   [undocumented]   
void opcode_xycb_01( unsigned );    // LD C, RLC (IX + d)   [undocumented]
void opcode_xycb_02( unsigned );    // LD D, RLC (IX + d)   [undocumented]
void opcode_xycb_03( unsigned );    // LD E, RLC (IX + d)   [undocumented]
void opcode_xycb_04( unsigned );    // LD H, RLC (IX + d)   [undocumented]
void opcode_xycb_05( unsigned );    // LD L, RLC (IX + d)   [undocumented]
void opcode_xycb_06( unsigned );    // RLC (IX + d)
void opcode_xycb_07( unsigned );    // LD A, RLC (IX + d)   [undocumented]
void opcode_xycb_08( unsigned );    // LD B, RRC (IX + d)   [undocumented]
void opcode_xycb_09( unsigned );    // LD C, RRC (IX + d)   [undocumented]
void opcode_xycb_0a( unsigned );    // LD D, RRC (IX + d)   [undocumented]
void opcode_xycb_0b( unsigned );    // LD E, RRC (IX + d)   [undocumented]
void opcode_xycb_0c( unsigned );    // LD H, RRC (IX + d)   [undocumented]
void opcode_xycb_0d( unsigned );    // LD L, RRC (IX + d)   [undocumented]
void opcode_xycb_0e( unsigned );    // RRC (IX + d)
void opcode_xycb_0f( unsigned );    // LD A, RRC (IX + d)   [undocumented]
void opcode_xycb_10( unsigned );    // LD B, RL (IX + d)    [undocumented]
void opcode_xycb_11( unsigned );    // LD C, RL (IX + d)    [undocumented]
void opcode_xycb_12( unsigned );    // LD D, RL (IX + d)    [undocumented]
void opcode_xycb_13( unsigned );    // LD E, RL (IX + d)    [undocumented]
void opcode_xycb_14( unsigned );    // LD H, RL (IX + d)    [undocumented]
void opcode_xycb_15( unsigned );    // LD L, RL (IX + d)    [undocumented]
void opcode_xycb_16( unsigned );    // RL (IX + d)
void opcode_xycb_17( unsigned );    // LD A, RL (IX + d)    [undocumented]
void opcode_xycb_18( unsigned );    // LD B, RR (IX + d)    [undocumented]
void opcode_xycb_19( unsigned );    // LD C, RR (IX + d)    [undocumented]
void opcode_xycb_1a( unsigned );    // LD D, RR (IX + d)    [undocumented]
void opcode_xycb_1b( unsigned );    // LD E, RR (IX + d)    [undocumented]
void opcode_xycb_1c( unsigned );    // LD H, RR (IX + d)    [undocumented]
void opcode_xycb_1d( unsigned );    // LD L, RR (IX + d)    [undocumented]
void opcode_xycb_1e( unsigned );    // RR (IX + d)
void opcode_xycb_1f( unsigned );    // LD A, RR (IX + d)    [undocumented]
void opcode_xycb_20( unsigned );    // LD B, SLA (IX + d)   [undocumented]
void opcode_xycb_21( unsigned );    // LD C, SLA (IX + d)   [undocumented]
void opcode_xycb_22( unsigned );    // LD D, SLA (IX + d)   [undocumented]
void opcode_xycb_23( unsigned );    // LD E, SLA (IX + d)   [undocumented]
void opcode_xycb_24( unsigned );    // LD H, SLA (IX + d)   [undocumented]
void opcode_xycb_25( unsigned );    // LD L, SLA (IX + d)   [undocumented]
void opcode_xycb_26( unsigned );    // SLA (IX + d)
void opcode_xycb_27( unsigned );    // LD A, SLA (IX + d)   [undocumented]
void opcode_xycb_28( unsigned );    // LD B, SRA (IX + d)   [undocumented]
void opcode_xycb_29( unsigned );    // LD C, SRA (IX + d)   [undocumented]
void opcode_xycb_2a( unsigned );    // LD D, SRA (IX + d)   [undocumented]
void opcode_xycb_2b( unsigned );    // LD E, SRA (IX + d)   [undocumented]
void opcode_xycb_2c( unsigned );    // LD H, SRA (IX + d)   [undocumented]
void opcode_xycb_2d( unsigned );    // LD L, SRA (IX + d)   [undocumented]
void opcode_xycb_2e( unsigned );    // SRA (IX + d)
void opcode_xycb_2f( unsigned );    // LD A, SRA (IX + d)   [undocumented]
void opcode_xycb_30( unsigned );    // LD B, SLL (IX + d)   [undocumented]
void opcode_xycb_31( unsigned );    // LD C, SLL (IX + d)   [undocumented]
void opcode_xycb_32( unsigned );    // LD D, SLL (IX + d)   [undocumented]
void opcode_xycb_33( unsigned );    // LD E, SLL (IX + d)   [undocumented]
void opcode_xycb_34( unsigned );    // LD H, SLL (IX + d)   [undocumented]
void opcode_xycb_35( unsigned );    // LD L, SLL (IX + d)   [undocumented]
void opcode_xycb_36( unsigned );    // SLL (IX + d)         [undocumented]
void opcode_xycb_37( unsigned );    // LD A, SLL (IX + d)   [undocumented]
void opcode_xycb_38( unsigned );    // LD B, SRL (IX + d)   [undocumented]
void opcode_xycb_39( unsigned );    // LD C, SRL (IX + d)   [undocumented]
void opcode_xycb_3a( unsigned );    // LD D, SRL (IX + d)   [undocumented]
void opcode_xycb_3b( unsigned );    // LD E, SRL (IX + d)   [undocumented]
void opcode_xycb_3c( unsigned );    // LD H, SRL (IX + d)   [undocumented]
void opcode_xycb_3d( unsigned );    // LD L, SRL (IX + d)   [undocumented]
void opcode_xycb_3e( unsigned );    // SRL (IX + d)
void opcode_xycb_3f( unsigned );    // LD A, SRL (IX + d)   [undocumented]
void opcode_xycb_40( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_41( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_42( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_43( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_44( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_45( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_46( unsigned );    // BIT 0, (IX + d)
void opcode_xycb_47( unsigned );    // BIT 0, (IX + d)      [undocumented]
void opcode_xycb_48( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_49( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_4a( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_4b( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_4c( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_4d( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_4e( unsigned );    // BIT 1, (IX + d)
void opcode_xycb_4f( unsigned );    // BIT 1, (IX + d)      [undocumented]
void opcode_xycb_50( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_51( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_52( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_53( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_54( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_55( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_56( unsigned );    // BIT 2, (IX + d)
void opcode_xycb_57( unsigned );    // BIT 2, (IX + d)      [undocumented]
void opcode_xycb_58( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_59( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_5a( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_5b( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_5c( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_5d( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_5e( unsigned );    // BIT 3, (IX + d)
void opcode_xycb_5f( unsigned );    // BIT 3, (IX + d)      [undocumented]
void opcode_xycb_60( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_61( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_62( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_63( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_64( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_65( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_66( unsigned );    // BIT 4, (IX + d)
void opcode_xycb_67( unsigned );    // BIT 4, (IX + d)      [undocumented]
void opcode_xycb_68( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_69( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_6a( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_6b( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_6c( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_6d( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_6e( unsigned );    // BIT 5, (IX + d)
void opcode_xycb_6f( unsigned );    // BIT 5, (IX + d)      [undocumented]
void opcode_xycb_70( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_71( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_72( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_73( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_74( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_75( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_76( unsigned );    // BIT 6, (IX + d)
void opcode_xycb_77( unsigned );    // BIT 6, (IX + d)      [undocumented]
void opcode_xycb_78( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_79( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_7a( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_7b( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_7c( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_7d( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_7e( unsigned );    // BIT 7, (IX + d)
void opcode_xycb_7f( unsigned );    // BIT 7, (IX + d)      [undocumented]
void opcode_xycb_80( unsigned );    // LD B, RES 0, (IX + d)    [undocumented]
void opcode_xycb_81( unsigned );    // LD C, RES 0, (IX + d)    [undocumented]
void opcode_xycb_82( unsigned );    // LD D, RES 0, (IX + d)    [undocumented]
void opcode_xycb_83( unsigned );    // LD E, RES 0, (IX + d)    [undocumented]
void opcode_xycb_84( unsigned );    // LD H, RES 0, (IX + d)    [undocumented]
void opcode_xycb_85( unsigned );    // LD L, RES 0, (IX + d)    [undocumented]
void opcode_xycb_86( unsigned );    // RES 0, (IX + d)
void opcode_xycb_87( unsigned );    // LD A, RES 0, (IX + d)    [undocumented]
void opcode_xycb_88( unsigned );    // LD B, RES 1, (IX + d)    [undocumented]
void opcode_xycb_89( unsigned );    // LD C, RES 1, (IX + d)    [undocumented]
void opcode_xycb_8a( unsigned );    // LD D, RES 1, (IX + d)    [undocumented]
void opcode_xycb_8b( unsigned );    // LD E, RES 1, (IX + d)    [undocumented]
void opcode_xycb_8c( unsigned );    // LD H, RES 1, (IX + d)    [undocumented]
void opcode_xycb_8d( unsigned );    // LD L, RES 1, (IX + d)    [undocumented]
void opcode_xycb_8e( unsigned );    // RES 1, (IX + d)
void opcode_xycb_8f( unsigned );    // LD A, RES 1, (IX + d)    [undocumented]
void opcode_xycb_90( unsigned );    // LD B, RES 2, (IX + d)    [undocumented]
void opcode_xycb_91( unsigned );    // LD C, RES 2, (IX + d)    [undocumented]
void opcode_xycb_92( unsigned );    // LD D, RES 2, (IX + d)    [undocumented]
void opcode_xycb_93( unsigned );    // LD E, RES 2, (IX + d)    [undocumented]
void opcode_xycb_94( unsigned );    // LD H, RES 2, (IX + d)    [undocumented]
void opcode_xycb_95( unsigned );    // LD L, RES 2, (IX + d)    [undocumented]
void opcode_xycb_96( unsigned );    // RES 2, (IX + d)
void opcode_xycb_97( unsigned );    // LD A, RES 2, (IX + d)    [undocumented]
void opcode_xycb_98( unsigned );    // LD B, RES 3, (IX + d)    [undocumented]
void opcode_xycb_99( unsigned );    // LD C, RES 3, (IX + d)    [undocumented]
void opcode_xycb_9a( unsigned );    // LD D, RES 3, (IX + d)    [undocumented]
void opcode_xycb_9b( unsigned );    // LD E, RES 3, (IX + d)    [undocumented]
void opcode_xycb_9c( unsigned );    // LD H, RES 3, (IX + d)    [undocumented]
void opcode_xycb_9d( unsigned );    // LD L, RES 3, (IX + d)    [undocumented]
void opcode_xycb_9e( unsigned );    // RES 3, (IX + d)
void opcode_xycb_9f( unsigned );    // LD A, RES 3, (IX + d)    [undocumented]
void opcode_xycb_a0( unsigned );    // LD B, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a1( unsigned );    // LD C, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a2( unsigned );    // LD D, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a3( unsigned );    // LD E, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a4( unsigned );    // LD H, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a5( unsigned );    // LD L, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a6( unsigned );    // RES 4, (IX + d)
void opcode_xycb_a7( unsigned );    // LD A, RES 4, (IX + d)    [undocumented]
void opcode_xycb_a8( unsigned );    // LD B, RES 5, (IX + d)    [undocumented]
void opcode_xycb_a9( unsigned );    // LD C, RES 5, (IX + d)    [undocumented]
void opcode_xycb_aa( unsigned );    // LD D, RES 5, (IX + d)    [undocumented]
void opcode_xycb_ab( unsigned );    // LD E, RES 5, (IX + d)    [undocumented]
void opcode_xycb_ac( unsigned );    // LD H, RES 5, (IX + d)    [undocumented]
void opcode_xycb_ad( unsigned );    // LD L, RES 5, (IX + d)    [undocumented]
void opcode_xycb_ae( unsigned );    // RES 5, (IX + d)
void opcode_xycb_af( unsigned );    // LD A, RES 5, (IX + d)    [undocumented]
void opcode_xycb_b0( unsigned );    // LD B, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b1( unsigned );    // LD C, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b2( unsigned );    // LD D, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b3( unsigned );    // LD E, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b4( unsigned );    // LD H, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b5( unsigned );    // LD L, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b6( unsigned );    // RES 6, (IX + d)
void opcode_xycb_b7( unsigned );    // LD A, RES 6, (IX + d)    [undocumented]
void opcode_xycb_b8( unsigned );    // LD B, RES 7, (IX + d)    [undocumented]
void opcode_xycb_b9( unsigned );    // LD C, RES 7, (IX + d)    [undocumented]
void opcode_xycb_ba( unsigned );    // LD D, RES 7, (IX + d)    [undocumented]
void opcode_xycb_bb( unsigned );    // LD E, RES 7, (IX + d)    [undocumented]
void opcode_xycb_bc( unsigned );    // LD H, RES 7, (IX + d)    [undocumented]
void opcode_xycb_bd( unsigned );    // LD L, RES 7, (IX + d)    [undocumented]
void opcode_xycb_be( unsigned );    // RES 7, (IX + d)
void opcode_xycb_bf( unsigned );    // LD A, RES 7, (IX + d)    [undocumented]
void opcode_xycb_c0( unsigned );    // LD B, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c1( unsigned );    // LD C, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c2( unsigned );    // LD D, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c3( unsigned );    // LD E, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c4( unsigned );    // LD H, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c5( unsigned );    // LD L, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c6( unsigned );    // SET 0, (IX + d)
void opcode_xycb_c7( unsigned );    // LD A, SET 0, (IX + d)    [undocumented]
void opcode_xycb_c8( unsigned );    // LD B, SET 1, (IX + d)    [undocumented]
void opcode_xycb_c9( unsigned );    // LD C, SET 1, (IX + d)    [undocumented]
void opcode_xycb_ca( unsigned );    // LD D, SET 1, (IX + d)    [undocumented]
void opcode_xycb_cb( unsigned );    // LD E, SET 1, (IX + d)    [undocumented]
void opcode_xycb_cc( unsigned );    // LD H, SET 1, (IX + d)    [undocumented]
void opcode_xycb_cd( unsigned );    // LD L, SET 1, (IX + d)    [undocumented]
void opcode_xycb_ce( unsigned );    // SET 1, (IX + d)
void opcode_xycb_cf( unsigned );    // LD A, SET 1, (IX + d)    [undocumented]
void opcode_xycb_d0( unsigned );    // LD B, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d1( unsigned );    // LD C, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d2( unsigned );    // LD D, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d3( unsigned );    // LD E, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d4( unsigned );    // LD H, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d5( unsigned );    // LD L, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d6( unsigned );    // SET 2, (IX + d)
void opcode_xycb_d7( unsigned );    // LD A, SET 2, (IX + d)    [undocumented]
void opcode_xycb_d8( unsigned );    // LD B, SET 3, (IX + d)    [undocumented]
void opcode_xycb_d9( unsigned );    // LD C, SET 3, (IX + d)    [undocumented]
void opcode_xycb_da( unsigned );    // LD D, SET 3, (IX + d)    [undocumented]
void opcode_xycb_db( unsigned );    // LD E, SET 3, (IX + d)    [undocumented]
void opcode_xycb_dc( unsigned );    // LD H, SET 3, (IX + d)    [undocumented]
void opcode_xycb_dd( unsigned );    // LD L, SET 3, (IX + d)    [undocumented]
void opcode_xycb_de( unsigned );    // SET 3, (IX + d)
void opcode_xycb_df( unsigned );    // LD A, SET 3, (IX + d)    [undocumented]
void opcode_xycb_e0( unsigned );    // LD B, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e1( unsigned );    // LD C, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e2( unsigned );    // LD D, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e3( unsigned );    // LD E, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e4( unsigned );    // LD H, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e5( unsigned );    // LD L, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e6( unsigned );    // SET 4, (IX + d)
void opcode_xycb_e7( unsigned );    // LD A, SET 4, (IX + d)    [undocumented]
void opcode_xycb_e8( unsigned );    // LD B, SET 5, (IX + d)    [undocumented]
void opcode_xycb_e9( unsigned );    // LD C, SET 5, (IX + d)    [undocumented]
void opcode_xycb_ea( unsigned );    // LD D, SET 5, (IX + d)    [undocumented]
void opcode_xycb_eb( unsigned );    // LD E, SET 5, (IX + d)    [undocumented]
void opcode_xycb_ec( unsigned );    // LD H, SET 5, (IX + d)    [undocumented]
void opcode_xycb_ed( unsigned );    // LD L, SET 5, (IX + d)    [undocumented]
void opcode_xycb_ee( unsigned );    // SET 5, (IX + d)
void opcode_xycb_ef( unsigned );    // LD A, SET 5, (IX + d)    [undocumented]
void opcode_xycb_f0( unsigned );    // LD B, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f1( unsigned );    // LD C, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f2( unsigned );    // LD D, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f3( unsigned );    // LD E, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f4( unsigned );    // LD H, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f5( unsigned );    // LD L, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f6( unsigned );    // SET 6, (IX + d)
void opcode_xycb_f7( unsigned );    // LD A, SET 6, (IX + d)    [undocumented]
void opcode_xycb_f8( unsigned );    // LD B, SET 7, (IX + d)    [undocumented]
void opcode_xycb_f9( unsigned );    // LD C, SET 7, (IX + d)    [undocumented]
void opcode_xycb_fa( unsigned );    // LD D, SET 7, (IX + d)    [undocumented]
void opcode_xycb_fb( unsigned );    // LD E, SET 7, (IX + d)    [undocumented]
void opcode_xycb_fc( unsigned );    // LD H, SET 7, (IX + d)    [undocumented]
void opcode_xycb_fd( unsigned );    // LD L, SET 7, (IX + d)    [undocumented]
void opcode_xycb_fe( unsigned );    // SET 7, (IX + d)
void opcode_xycb_ff( unsigned );    // LD A, SET 7, (IX + d)    [undocumented]

// Trivia: there are 1018 opcode_xxx() functions in this class, 
// for a total of 1274 emulated opcodes. Fortunately, most of them
// were automatically generated by custom made programs and scripts.

#endif
