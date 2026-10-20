/* CPU clock measurement kernel for jz/x1000clk.lua: spins n times through a
 * cached addiu/bnez loop and records the OST2 counter (EXCLK / 4) before and
 * after. Called with hwstub's call. The parameter block address is patched
 * into the first two instructions:
 *   +0: n (in)  +4: OST start  +8: OST end
 * The loop takes 6 cycles per pass. CP0 Count does not count on the X1000.
 * Rebuild with
 *   mipsel-elf-gcc -march=mips32r2 -c x1000_spin.S -o spin.o &&
 *   mipsel-elf-objcopy -O binary -j .text spin.o spin.bin && xxd -p -c 32 spin.bin
 */
    .set    noreorder
    .set    noat
    .text
    lui     $25, 0              /* parameter block, patched */
    ori     $25, $25, 0
    lw      $8, 0($25)
    lui     $9, 0xb200          /* OST */
    lw      $10, 0x20($9)       /* OST_2CNTL */
    .align  5
1:  addiu   $8, $8, -1
    bnez    $8, 1b
    nop
    lw      $12, 0x20($9)
    sw      $10, 4($25)
    sw      $12, 8($25)
    jr      $31
    nop
