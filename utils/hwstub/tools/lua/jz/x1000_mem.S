/* Memory throughput kernel for jz/x1000mem.lua: runs passes over a buffer,
 * reading, writing or copying 32 bytes (a cache line) per loop, and records
 * the OST2 counter (EXCLK / 4) before and after. Called with hwstub's call.
 * The parameter block address is patched into the first two instructions:
 *   +0: op (0: read, 1: write, 2: copy)  +4: buffer  +8: copy destination
 *   +12: size in bytes, a multiple of 32  +16: passes
 *   +20: OST start (out)  +24: OST end (out)
 * Rebuild with
 *   mipsel-elf-gcc -march=mips32r2 -c x1000_mem.S -o mem.o &&
 *   mipsel-elf-objcopy -O binary -j .text mem.o mem.bin && xxd -p -c 32 mem.bin
 */
    .set    noreorder
    .set    noat
    .text
    lui     $25, 0              /* parameter block, patched */
    ori     $25, $25, 0
    lw      $8, 0($25)          /* op */
    lw      $4, 4($25)          /* buffer */
    lw      $5, 8($25)          /* destination */
    lw      $6, 12($25)         /* size */
    lw      $7, 16($25)         /* passes */
    lui     $24, 0xb200         /* OST */
    lw      $2, 0x20($24)       /* OST_2CNTL */
pass:
    move    $9, $4
    move    $10, $5
    addu    $11, $4, $6         /* end */
    beqz    $8, rd
    addiu   $1, $8, -1
    beqz    $1, wr
    nop
    .align  5
cp: lw      $12, 0($9)
    lw      $13, 4($9)
    lw      $14, 8($9)
    lw      $15, 12($9)
    sw      $12, 0($10)
    sw      $13, 4($10)
    sw      $14, 8($10)
    sw      $15, 12($10)
    lw      $12, 16($9)
    lw      $13, 20($9)
    lw      $14, 24($9)
    lw      $15, 28($9)
    sw      $12, 16($10)
    sw      $13, 20($10)
    sw      $14, 24($10)
    sw      $15, 28($10)
    addiu   $9, $9, 32
    bne     $9, $11, cp
    addiu   $10, $10, 32
    b       next
    nop
    .align  5
rd: lw      $12, 0($9)
    lw      $13, 4($9)
    lw      $14, 8($9)
    lw      $15, 12($9)
    lw      $12, 16($9)
    lw      $13, 20($9)
    lw      $14, 24($9)
    lw      $15, 28($9)
    addiu   $9, $9, 32
    bne     $9, $11, rd
    nop
    b       next
    nop
    .align  5
wr: sw      $9, 0($9)
    sw      $9, 4($9)
    sw      $9, 8($9)
    sw      $9, 12($9)
    sw      $9, 16($9)
    sw      $9, 20($9)
    sw      $9, 24($9)
    sw      $9, 28($9)
    addiu   $9, $9, 32
    bne     $9, $11, wr
    nop
next:
    addiu   $7, $7, -1
    bnez    $7, pass
    nop
    lw      $3, 0x20($24)
    sw      $2, 20($25)
    sw      $3, 24($25)
    jr      $31
    nop
