/* Host mock of the Nano 3G NAND driver. */
#ifndef MOCK_NAND_H
#define MOCK_NAND_H

#include <stdint.h>

/* The backing store, mapped copy-on-write: writes land in RAM and never
 * reach the dump files. Page data is NAND_PAGE_SIZE per page; meta is 16
 * bytes per page - 3 spare words then the int32 nand_read_page() result. */
extern uint8_t *mock_data;

/* What nand_get_geometry() returns: the dumped 4GB unit's chip, which tests
 * may alter before ftl_init() */
struct nand_geometry;
extern struct nand_geometry mock_geo;
extern uint8_t *mock_meta;

/* Counters, for tests to assert on. Reset with mock_reset_stats(). */
extern unsigned long mock_reads;
extern unsigned long mock_read_runs, mock_read_run_pages;
extern unsigned long mock_writes;
extern unsigned long mock_writes_by_type[256]; /* by spare type */
extern unsigned long mock_erases;
/* Violations of real NAND's rules, which the FTL must never commit: */
extern unsigned long mock_reprogram;      /* programmed a non-erased page */
extern unsigned long mock_nonsequential;  /* skipped a page within a block */
extern unsigned long mock_runs, mock_run_pages;          /* nand_write_pages calls */
extern unsigned long mock_twoplane_runs, mock_twoplane_pages;
extern unsigned long mock_bad_twoplane;   /* a pair the chip could not take */

/* Fault injection: make a physical block refuse programs and/or erases, as a
 * worn-out block does. NAND_OP_FAILED is what the real driver returns when
 * the chip reports a failure. */
#define MOCK_FAIL_WRITE 1
#define MOCK_FAIL_ERASE 2
void mock_set_bad(uint32_t bank, uint32_t block, int how);
void mock_clear_bad(void);
extern unsigned long mock_write_fails, mock_erase_fails;

/* Crash injection: after this many more programs and erases, the next one
 * does not happen and longjmp(mock_crash_jmp, 1) is taken instead, as if
 * power were cut just before it. 0 disables. */
#include <setjmp.h>
extern unsigned long mock_crash_after;
extern jmp_buf mock_crash_jmp;
extern void *mock_crash_bt[32];   /* where the last cut happened */
extern int mock_crash_btn;
extern int mock_crash_torn;   /* leave the cut operation half done */

void mock_open(const char *dir);
/* A blank, erased medium with mock_geo's geometry, for ftl_hook_format() */
void mock_open_blank(void);
void mock_reset_stats(void);

/* The dump presented as a chip of another layout would hold it: "halves"
 * (mode 9: 4 banks of 4096 blocks, unit u of VFL block v at v + u * 2048) or
 * "both" (mode 4: 2 banks of 8192 blocks, 4 units, unit u of v at
 * 2v + (u & 1) + ((u & 2) ? 4096 : 0), taking dump banks ce and ce + 2).
 * Every superblock keeps its pages in order, except that on each new bank
 * block 1 and the formula's place for the primary dump bank's block 1 (2048
 * or 4096) are swapped, so the context ring starts within blocks 1..199 as
 * on a chip Apple formatted. The rewritten rings name blocks where they now
 * sit; remap entries name replaced blocks by the formula alone, as the FTL
 * computes them (their contents are never read). Sets mock_geo, and every NAND
 * call, mock_set_bad() and crash injection then use the new numbering. The
 * VFL contexts are rewritten in the copy-on-write mapping to hold the new
 * block numbers; for "both" the newest contexts of the two dump banks are
 * merged into every context page of both. mock_data and mock_meta stay in
 * the dump's layout. */
void mock_open_layout(const char *dir, const char *layout);

/* Block translation of the layout last opened, the identity after
 * mock_open() or mock_open_blank(): a block as the FTL numbers it to the
 * dump's bank and block holding its pages, and back */
void mock_layout_to_dump(uint32_t bank, uint32_t block,
                         uint32_t *dbank, uint32_t *dblock);
void mock_layout_from_dump(uint32_t dbank, uint32_t dblock,
                           uint32_t *bank, uint32_t *block);

/* Write the current state of the mock out, for post-mortem inspection. The
 * files are the backing store, so after mock_open_layout() they are in the
 * dump's layout, not the one the FTL saw. Returns 0, or -1 with errno set. */
int mock_snapshot(const char *dir);

#endif
