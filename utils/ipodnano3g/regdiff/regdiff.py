#!/usr/bin/env python3
"""regdiff.py -- check nand_write_pages() in nand-nano3g.c against Apple's
FMSS write programs, register write for register write.

The driver's write functions are extracted from the source, every FMC
register access is rewritten into a logging call, and the result is compiled
for the host with a mock controller that is always ready and never fails -
the same mock fmss_emu.py runs Apple's programs against. Both are given the
same run of pages and their register writes are compared.

  regdiff.py            all scenarios
"""
import os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROCKBOX = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
DRIVER = ROCKBOX + "/firmware/target/arm/s5l8702/ipodnano3g/nand-nano3g.c"
HEADER = ROCKBOX + "/firmware/export/s5l87xx.h"
sys.path.insert(0, HERE)
import fmss_emu

FUNCS = ["nand_set_fmctrl0", "nand_wait_reg", "nand_wait_stat",
         "nand_clear_status", "nand_send_cmd", "nand_send_addr_page",
         "nand_read_status", "nand_finish", "nand_load_spare", "nand_encode",
         "nand_run_status", "nand_write_pages"]

# The base FMCTRL0 value nand_set_fmctrl0() uses without a chip enable
BASE_CTRL0 = 0x43000


def registers():
    regs = {}
    for m in re.finditer(r"#define\s+(FM[A-Z0-9_]+)\s+\(\*\(REG32_PTR_T\)"
                         r"\(FMC_BASE\s*\+\s*(0x[0-9A-Fa-f]+)\)\)",
                         open(HEADER).read()):
        regs[m.group(1)] = int(m.group(2), 16)
    return regs


def extract(src, name):
    m = re.search(r"^(static\s+)?[a-z].*\b%s\(" % name, src, re.M)
    if not m:
        raise SystemExit("no function " + name)
    i = src.index("{", m.start())
    depth = 0
    for j in range(i, len(src)):
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
            if depth == 0:
                return src[m.start():j + 1]


def transform(code, regs):
    names = sorted(regs, key=len, reverse=True)
    alt = "|".join(names)

    def reads(expr):
        return re.sub(r"\b(%s)\b" % alt, lambda m: "R(0x%x)" % regs[m.group(1)],
                      expr)

    # nand_wait_reg takes a register offset on the host
    code = code.replace("volatile uint32_t *reg", "uint32_t reg")
    code = code.replace("*reg & bit", "R(reg) & bit")
    code = code.replace("*reg = bit", "W(reg, bit)")
    code = re.sub(r"&(%s)\b" % alt, lambda m: "0x%x" % regs[m.group(1)], code)
    code = re.sub(r"\b(%s)\s*(\|=|&=)\s*([^;]+);" % alt,
                  lambda m: "W(0x%x, R(0x%x) %s (%s));" % (
                      regs[m.group(1)], regs[m.group(1)], m.group(2)[0],
                      reads(m.group(3))), code)
    code = re.sub(r"\b(%s)\s*=(?!=)\s*([^;]+);" % alt,
                  lambda m: "W(0x%x, %s);" % (regs[m.group(1)],
                                              reads(m.group(2))), code)
    return reads(code)


PRELUDE = r"""
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
static uint32_t io[0x1000];
static void W(uint32_t off, uint32_t v) { io[off] = v; printf("%03x=%08x\n", off, v); }
static uint32_t R(uint32_t off)
{
    if (off == 0x048 || off == 0x840) return 0xffffffff;   /* every event done */
    if (off == 0x04c) return 0x40;                          /* ready, pass */
    if (off == 0x07c) return 0;                             /* encoder done */
    return io[off];
}
#define USEC_TIMER 0UL
#define TIME_BEFORE(a, b) 1
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))
#define FMCTRL0_UNK1 (1 << 11)
#define FMCTRL0_AUTOXFER (1 << 24)
#define FMCTRL1_DOTRANSADDR (1 << 0)
#define FMCSTAT_CMDDONE (1 << 1)
#define FMCSTAT_ADDRDONE (1 << 2)
#define FMCSTAT_TRANSDONE (1 << 3)
#define FMCSTAT_UNK20 (1 << 20)
#define FMCSTAT_STATUSREADY (1 << 23)
#define NAND_PAGE_SIZE 2048
#define NAND_CHUNK_SIZE 512
#define NAND_OP_FAILED (-2)
struct nand_write { uint32_t bank, page; const void *buf; const uint32_t *meta; };
struct { uint16_t blocks, pagesperblock, pagesize; } chip = { 4096, 128 },
    *nand_chip = &chip;
static bool nand_ready = true;
static unsigned int nand_banks = 4;
static void commit_dcache_range(const void *p, int n) { (void)p; (void)n; }
"""


def driver_harness(regs):
    src = open(DRIVER).read()
    defines = "\n".join(l for l in src.splitlines()
                        if re.match(r"#define\s+(NAND_CMD_|NAND_STATUS_|NAND_TUNK1|"
                                    r"NAND_TWP|NAND_POLL_SPINS|NAND_MAX_BANKS)", l))
    protos = "\n".join(transform(re.sub(r"\)\s*$", ");",
                                         extract(src, f).split("{")[0].strip()), regs)
                       for f in FUNCS)
    body = "\n\n".join(transform(extract(src, f), regs) for f in FUNCS)
    main = r"""
int main(int argc, char **argv)
{
    static struct nand_write w[64];
    static uint32_t meta[64][3];
    unsigned int n = 0, i;
    int two = argv[1][0] == '2';
    chip.pagesize = atoi(argv[2]);
    uint32_t bank, page, fb;
    while (scanf("%u %u", &bank, &page) == 2)
    {
        w[n].bank = bank; w[n].page = page;
        w[n].buf = (const void *)(intptr_t)(0x08000000 + n * chip.pagesize);
        for (i = 0; i < 3; i++) meta[n][i] = 0x11110000 | (n << 4) | i;
        w[n].meta = meta[n];
        n++;
    }
    return nand_write_pages(w, n, two, &fb) ? 1 : 0;
}
"""
    return PRELUDE + defines + "\n" + protos + "\n" + body + main


def scenarios():
    def rows(nrows, pairs):
        out = []
        for r in range(nrows):
            if pairs:
                for b in range(4):
                    out += [(b, 1000 * 128 + r), (b, 1001 * 128 + r)]
            else:
                for p in range(2):
                    for b in range(4):
                        out.append((b, (1000 + p) * 128 + r))
        return out
    return [("one page", "1", [(0, 1000 * 128)]),
            ("one page bank 2", "1", [(2, 1000 * 128 + 5)]),
            ("single-plane row", "1", rows(1, False)),
            ("single-plane two rows", "1", rows(2, False)),
            ("three pages one bank", "1", [(1, 1000 * 128), (1, 1000 * 128 + 1),
                                           (1, 1000 * 128 + 2)]),
            ("two-plane row", "2", rows(1, True)),
            ("two-plane two rows", "2", rows(2, True)),
            ("two-plane three rows", "2", rows(3, True))]


def main():
    regs = registers()
    tmp = os.path.join(HERE, "build")
    os.makedirs(tmp, exist_ok=True)
    c = os.path.join(tmp, "h.c")
    exe = os.path.join(tmp, "h")
    open(c, "w").write(driver_harness(regs))
    r = subprocess.run(["cc", "-std=gnu99", "-w", "-o", exe, c],
                       capture_output=True, text=True)
    if r.returncode:
        print(r.stderr[:3000])
        print("harness source:", c)
        return 1
    fails = 0
    for (name, mode, pages), size in [(s, z) for z in (2048, 4096)
                                      for s in scenarios()]:
        name = "%s %dKiB" % (name, size // 1024)
        e = fmss_emu.Emu(fmss_emu.PROGRAMS["program2" if mode == "2" else "program1"])
        fmss_emu.setup_list(e, pages, BASE_CTRL0 | 0x801, units=size // 2048)
        e.run()
        apple = ["%03x=%08x" % w for w in e.raw]
        inp = "".join("%d %d\n" % p for p in pages)
        out = subprocess.run([exe, mode, str(size)], input=inp, capture_output=True,
                             text=True).stdout.split()
        # The driver clears FMCSTAT after every operation (nand_clear_status),
        # which Apple's FIL does outside the program; that one trailing write
        # is expected
        if out[:-1] == apple and out[-1:] == ["048=ffffffff"]:
            print("PASS  %-30s %d register writes identical, + status clear"
                  % (name, len(apple)))
            continue
        fails += 1
        k = next((i for i, (a, b) in enumerate(zip(out, apple)) if a != b),
                 min(len(out), len(apple)))
        print("FAIL  %-30s first difference at write %d (ours %d, Apple %d)"
              % (name, k, len(out), len(apple)))
        for i in range(max(0, k - 6), min(max(len(out), len(apple)), k + 6)):
            print("   %4d  ours %-14s apple %s" % (
                i, out[i] if i < len(out) else "-", apple[i] if i < len(apple) else "-"))
    print("ALL IDENTICAL" if not fails else "%d SCENARIOS DIFFER" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
