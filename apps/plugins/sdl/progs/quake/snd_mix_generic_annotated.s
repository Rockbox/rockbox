	.cpu arm926ej-s
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 2
	.eabi_attribute 18, 4
	.file	"snd_mix_generic.c"
@ GNU C (GCC) version 4.4.4 (arm-elf-eabi)
@	compiled by GNU C version 4.8.2 20140206 (prerelease), GMP version 4.3.2, MPFR version 2.4.2.
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -imultilib arm926ej-s -D__USES_INITFINI__
@ ./snd_mix_generic.c -mcpu=arm926ej-s -O3 -fverbose-asm
@ options enabled:  -falign-loops -fargument-alias -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fcommon -fcprop-registers
@ -fcrossjumping -fcse-follow-jumps -fdefer-pop
@ -fdelete-null-pointer-checks -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations
@ -fforward-propagate -ffunction-cse -fgcse -fgcse-after-reload -fgcse-lm
@ -fguess-branch-probability -fident -fif-conversion -fif-conversion2
@ -findirect-inlining -finline -finline-functions
@ -finline-functions-called-once -finline-small-functions -fipa-cp
@ -fipa-cp-clone -fipa-pure-const -fipa-reference -fira-share-save-slots
@ -fira-share-spill-slots -fivopts -fkeep-static-consts
@ -fleading-underscore -fmath-errno -fmerge-constants -fmerge-debug-strings
@ -fmove-loop-invariants -fomit-frame-pointer -foptimize-register-move
@ -foptimize-sibling-calls -fpeephole -fpeephole2 -fpredictive-commoning
@ -freg-struct-return -fregmove -freorder-blocks -freorder-functions
@ -frerun-cse-after-loop -fsched-interblock -fsched-spec
@ -fsched-stalled-insns-dep -fschedule-insns -fschedule-insns2
@ -fsection-anchors -fsigned-zeros -fsplit-ivs-in-unroller
@ -fsplit-wide-types -fstrict-aliasing -fstrict-overflow -fthread-jumps
@ -ftoplevel-reorder -ftrapping-math -ftree-builtin-call-dce -ftree-ccp
@ -ftree-ch -ftree-copy-prop -ftree-copyrename -ftree-cselim -ftree-dce
@ -ftree-dominator-opts -ftree-dse -ftree-fre -ftree-loop-im
@ -ftree-loop-ivcanon -ftree-loop-optimize -ftree-parallelize-loops=
@ -ftree-pre -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-sra
@ -ftree-switch-conversion -ftree-ter -ftree-vect-loop-version
@ -ftree-vectorize -ftree-vrp -funit-at-a-time -funswitch-loops
@ -fverbose-asm -fzero-initialized-in-bss -mlittle-endian -msched-prolog

@ Compiler executable checksum: 6bc9eb36ed7f27ae4e8601da7e401eb4

	.text
	.align	2
	.global	SND_PaintChannelFrom8
	.type	SND_PaintChannelFrom8, %function
SND_PaintChannelFrom8:
        // r0: true_lvol
        // r1: true_rvol
        // r2: *sfx
        // r3: count

	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r3, #0	@ count
	stmfd	sp!, {r4, r5, r6, r7, r8, sl}	@,
	ble	.L10	@, // exit early if count <= 0
	ldr	ip, .L13	@ tmp207, // ip = paintbuffer
	ldr	r5, .L13+4	@ tmp224, // r5 = 32767
	ldr	ip, [ip, #0]	@ ivtmp.61, paintbuffer // ip = &paintbuffer

        // this does {rvol,lvol} &= 0xffff in a really convoluted way
	mov	r1, r1, asl #16	@ tmp206, true_rvol, // r1 = rvol << 16
	mov	r0, r0, asl #16	@ tmp205, true_lvol, // r0 = lvol << 16
	mov	r6, r1, lsr #16	@ pretmp.46, tmp206, // rvol_16 = r1 >> 16
	mov	r7, r0, lsr #16	@ pretmp.44, tmp205, // lvol_16 = r0 >> 16
	mov	r1, #0	@ i, // i = 0
        // loop?
.L9:
	ldrsb	r0, [r2, r1]	@ tmp208,* i // r0 = (byte) sfx[i]
	ldrsh	r8, [ip, #0]	@ tmp213,* ivtmp.61 // r8 = (int16) paintbuffer[0]

        // r0 &= 0xffff
	mov	r0, r0, asl #16	@ tmp209, tmp208, // r0 <<= 16
	mov	r0, r0, lsr #16	@ D.1289, tmp209, // r0 >>= 16


	mul	r4, r0, r7	@ tmp210, D.1289, pretmp.44 // r4 = sfx[i] * (lvol_16)

	mov	r4, r4, asl #16	@ tmp212, tmp210,        // r4 <<= 16

	add	r4, r8, r4, asr #16	@, val, tmp213, tmp212, // r8 += r4 & 0xffff

        // handle saturation
	cmp	r4, r5	@ val, tmp224 // compare r4, 32767
	ldrgt	r8, .L13+4	@ D.1341, // r8 = 32767 if saturated
	bgt	.L4	@,                // jump to .L4 if saturated
	cmn	r4, #32768	@ val, // compare r4, -32768
	movge	r4, r4, asl #16	@ tmp216, val, // if not saturated, &= 0xffff
	movlt	r8, #32768	@ D.1341,         // r8 = -32768 if saturated
	movge	r8, r4, lsr #16	@ D.1341, tmp216, // r8 = val

        // r8 is final value of paintbuffer[2*i+0]

        // now handle right channel
.L4:
        // sl is scaled right channel

        // this does paintbuffer[1] += rvol * sfx[i]
	mul	sl, r0, r6	@ tmp217, D.1289, pretmp.46 // sl = sfx[i] * rvol_16
	ldrsh	r4, [ip, #2]	@ tmp220, // r4 = paintbuffer[1]
	mov	r0, sl, asl #16	@ tmp219, tmp217, // r0 = sl << 16
	add	r0, r4, r0, asr #16	@, val, tmp220, tmp219, // r0 = r4 + (r0 >> 16)

        // now saturate
	cmp	r0, r5	@ val, tmp224 // compare final right channel to 32767

	strh	r8, [ip, #0]	@ movhi	@ D.1341,* ivtmp.61 // write left channel back to paintbuffer
	ldrgt	r0, .L13+4	@ D.1348, // r0 = 32767 if saturated
	bgt	.L7	@, // skip to .L7
	cmn	r0, #32768	@ val, // check < -32768
	movge	r0, r0, asl #16	@ tmp223, val, // <<= 16 if not saturated
	movlt	r0, #32768	@ D.1348, // if < -32768, saturate
	movge	r0, r0, lsr #16	@ D.1348, tmp223, // >>= 16 if not saturated
.L7:
	add	r1, r1, #1	@ i, i, // i++
	cmp	r1, r3	@ i, count // i =? count
	strh	r0, [ip, #2]	@ movhi	@ D.1348 , // store right channel to paintbuffer
	add	ip, ip, #4	@ ivtmp.61, ivtmp.61, // paintbuffer += 4 bytes
	bne	.L9	@, // restart if i != count
.L10:
	ldmfd	sp!, {r4, r5, r6, r7, r8, sl} // pop stack
	bx	lr // return
.L14:
	.align	2
.L13:
	.word	paintbuffer
	.word	32767
	.size	SND_PaintChannelFrom8, .-SND_PaintChannelFrom8
	.align	2
	.global	SND_PaintChannelFrom16
	.type	SND_PaintChannelFrom16, %function
SND_PaintChannelFrom16:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r3, #0	@ count
	stmfd	sp!, {r4, r5, r6, r7, r8, sl}	@,
	ble	.L24	@,
	ldr	ip, .L26	@ tmp146,
	ldr	r7, .L26+4	@ tmp164,
	ldr	ip, [ip, #0]	@ ivtmp.119, paintbuffer
	mov	r4, #0	@ i,
.L23:
	mov	r5, r4, asl #1	@ tmp147, i,
	ldrsh	r5, [r2, r5]	@ data,
	ldrsh	r8, [ip, #0]	@ tmp152,* ivtmp.119
	mul	r6, r5, r0	@ tmp148, data, true_lvol
	mov	r6, r6, asl #8	@ tmp151, tmp148,
	add	r6, r8, r6, asr #16	@, val, tmp152, tmp151,
	cmp	r6, r7	@ val, tmp164
	ldrgt	r8, .L26+4	@ D.1355,
	bgt	.L18	@,
	cmn	r6, #32768	@ val,
	movge	r6, r6, asl #16	@ tmp155, val,
	movlt	r8, #32768	@ D.1355,
	movge	r8, r6, lsr #16	@ D.1355, tmp155,
.L18:
	mul	sl, r5, r1	@ tmp156, data, true_rvol
	ldrsh	r6, [ip, #2]	@ tmp160,
	mov	r5, sl, asl #8	@ tmp159, tmp156,
	add	r5, r6, r5, asr #16	@, val, tmp160, tmp159,
	cmp	r5, r7	@ val, tmp164
	strh	r8, [ip, #0]	@ movhi	@ D.1355,* ivtmp.119
	ldrgt	r5, .L26+4	@ D.1362,
	bgt	.L21	@,
	cmn	r5, #32768	@ val,
	movge	r5, r5, asl #16	@ tmp163, val,
	movlt	r5, #32768	@ D.1362,
	movge	r5, r5, lsr #16	@ D.1362, tmp163,
.L21:
	add	r4, r4, #2	@ i, i,
	cmp	r3, r4	@ count, i
	strh	r5, [ip, #2]	@ movhi	@ D.1362,
	add	ip, ip, #8	@ ivtmp.119, ivtmp.119,
	bgt	.L23	@,
.L24:
	ldmfd	sp!, {r4, r5, r6, r7, r8, sl}
	bx	lr
.L27:
	.align	2
.L26:
	.word	paintbuffer
	.word	32767
	.size	SND_PaintChannelFrom16, .-SND_PaintChannelFrom16
	.ident	"GCC: (GNU) 4.4.4"
