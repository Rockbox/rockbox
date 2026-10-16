#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (C) 2026 Marcin Bukat
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
# ZSP500 DSP test suite for the RK27xx, run on the device by ../zsp.lua
# (hwstub_shell). Builds the test images tests/*.bin with the Rockbox DSP
# assembler, tools/zsp/zspasm.pl; the generated tests come from
# tests/gen_*.py (Python 3).
#
#   make            all test images
#   make bench      cycle benchmarks (tools/gen_bench.py), tests/bench/
#   make results    check results/*.txt written by zsp.lua (tests/expect.py)
#   make clean
#
# DMEM_DSP is the DSP-side address of DMEM (0, found by t0b_probe).

ROOT    := $(abspath ../../../../../..)
ZSPASM  := $(ROOT)/tools/zsp/zspasm.pl
export ZSPASM
DMEM_DSP ?= 0
ASM      = perl $(ZSPASM) -q -I tests -D DMEM_DSP=$(DMEM_DSP)
INC      = tests/common.inc tests/prologue.inc tests/epilogue.inc $(ZSPASM) $(ROOT)/tools/zsp/zsp500.isa

TESTS   = t20_final t17b_circ t15_rest t16_flags t17_circ t14_settle t13_inferred t12_hazards t11_timers t0a_loop t0b_probe t0d_hazard t0e_storelag t0f_storedata t0g_loads t0h_slot0 t0c_units t1_call t2_branch t2b_bgt32 t3_regs t4_mem \
          t5_imm t6_creg t7_xbits t7_xbits_ref t8_alu t9_cexe tclk_count
INVALID = E000 DC00 0100 7941 FE00
TCVALS  = 0001 0002 0004 0008 0010 0020 0040 0080 0100 0200 0400 0800 1000 2000 4000 8000
CPTESTS = 0 1 2 3 4 5 6 7 8
JMPTESTS = 0 1 2 3 4 5
IRQTESTS = t21_ireq $(addprefix t22_vec_,a e f g h i j m s1 s13 s14 s15) t23_nmi t24_space \
           $(addprefix t25_smode_,0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15) \
           $(addprefix t26_ext_,none all b0 b2 b3 b4 b5 b6 b7 b8 b9 b10 b11) t27_ext t28_mbox t29_mbox t30_bustime t31_bususe \
           $(addprefix t32_,a b c d e f) t33_sdstore t34_sdbuf t35_dma t36_dma2 t37_dmairq t38_armirq t39_circ t40_circ2 t41_circ3 t42_brev t43_circ4 t44_brevst t45_reti_smode t46_prio t47_prio2 \
           t48_src14 t49_stdshort t50_nest t51_circ5 t52_cexe \
           t53_vxor_a t53_vxor_b t53_vxor_c $(addprefix t54_,s1_t500 s1_t2000 s1_t8000 b15 s1_nmi) t55_hwflag2 t56_hwflag2b t57_hwflag2c t58_hwflag2d t59_bisect t60_hwflag2e t61_cexe2 t62_nestprio t63_circ6
BINS    = $(IRQTESTS:%=tests/%.bin) $(JMPTESTS:%=tests/t19_jump_%.bin) $(CPTESTS:%=tests/t18_cp_%.bin) $(TESTS:%=tests/%.bin) $(INVALID:%=tests/t10_%.bin) $(TCVALS:%=tests/t11b_%.bin)

all: $(BINS)

tests/%.bin: tests/%.s $(INC)
	$(ASM) -o $@ -l $(@:.bin=.lst) $<

tests/t7_xbits.bin: tests/t7_xbits.s $(INC)
	$(ASM) -D XB=1 -o $@ -l $(@:.bin=.lst) $<

tests/t7_xbits_ref.bin: tests/t7_xbits.s $(INC)
	$(ASM) -D XB=0 -o $@ -l $(@:.bin=.lst) $<

tests/t10_%.bin: tests/t10_invalid.s $(INC)
	$(ASM) -D OPC=0x$* -o $@ -l $(@:.bin=.lst) $<

tests/t11b_%.bin: tests/t11b_tc.s $(INC)
	$(ASM) -D TC=0x$* -o $@ -l $(@:.bin=.lst) $<

# generated tests (each generator also writes the .json its checker reads).
# A generator writing several tests runs once, behind a stamp file, so that
# make -j does not run it for each of them at once.
tests/t8_alu.s: tests/gen_t8.py
	cd tests && python3 gen_t8.py
tests/t12_hazards.s: tests/gen_t12.py
	cd tests && python3 gen_t12.py
tests/t13_inferred.s: tests/gen_t13.py
	cd tests && python3 gen_t13.py
tests/t14_settle.s: tests/gen_t14.py
	cd tests && python3 gen_t14.py
tests/t15_rest.s: tests/gen_t15.py
	cd tests && python3 gen_t15.py
tests/t16_flags.s: tests/gen_t16.py
	cd tests && python3 gen_t16.py
tests/t17_circ.s: tests/gen_t17.py
	cd tests && python3 gen_t17.py
tests/t17b_circ.s: tests/gen_t17.py
	cd tests && python3 gen_t17.py b
tests/.gen_t18: tests/gen_t18.py
	cd tests && python3 gen_t18.py && touch .gen_t18
$(CPTESTS:%=tests/t18_cp_%.s): tests/.gen_t18 ;
tests/.gen_t19: tests/gen_t19.py
	cd tests && python3 gen_t19.py && touch .gen_t19
$(JMPTESTS:%=tests/t19_jump_%.s): tests/.gen_t19 ;
tests/t20_final.s: tests/gen_t20.py
	cd tests && python3 gen_t20.py
tests/.gen_t21: tests/gen_t21.py
	cd tests && python3 gen_t21.py && touch .gen_t21
$(IRQTESTS:%=tests/%.s): tests/.gen_t21 ;

# check on-device results (results/*.txt written by zsp.lua)
results:
	python3 tests/expect.py results

# cycle benchmarks: make bench, then ZSP.bench() in zsp.lua and
# tools/bench_report.py (or tools/zsp/zspbench.py to compare zspsim.py)
bench: tests/bench/cases.json
	@for s in tests/bench/*.s; do $(ASM) -o $${s%.s}.bin -l $${s%.s}.lst $$s || exit 1; done
	@echo "built $$(ls tests/bench/*.bin | wc -l) benchmark images"

tests/bench/cases.json: tools/gen_bench.py
	python3 tools/gen_bench.py

GENERATED = $(IRQTESTS:%=tests/%.s) $(JMPTESTS:%=tests/t19_jump_%.s) $(CPTESTS:%=tests/t18_cp_%.s) \
            $(addprefix tests/,t8_alu.s t12_hazards.s t13_inferred.s t14_settle.s t15_rest.s t16_flags.s \
                               t17_circ.s t17b_circ.s t20_final.s)

clean:
	rm -f $(GENERATED) tests/*.bin tests/*.lst tests/*.json tests/.gen_*
	rm -rf tests/bench tests/__pycache__ tools/__pycache__

.PHONY: all results bench clean
