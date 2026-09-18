#!/usr/bin/env python3
"""Compare nand_read_pages() register writes with Apple's read program."""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROCKBOX = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
DRIVER = ROCKBOX + "/firmware/target/arm/s5l8702/ipodnano3g/nand-nano3g.c"
HEADER = ROCKBOX + "/firmware/export/s5l87xx.h"
sys.path.insert(0, HERE)
import fmss_emu

FUNCS = ["nand_wait_reg", "nand_wait_stat", "nand_clear_status",
         "nand_send_cmd", "nand_send_addr_page", "nand_read_status",
         "nand_read_ecc_result", "nand_read_loaded_page", "nand_read_pages"]
BASE_CTRL0 = 0x43000


def registers():
    regs = {}
    with open(HEADER) as f:
        src = f.read()
    pattern = (r"#define\s+(FM[A-Z0-9_]+)\s+\(\*\(REG32_PTR_T\)"
               r"\(FMC_BASE\s*\+\s*(0x[0-9A-Fa-f]+)\)\)")
    for m in re.finditer(pattern, src):
        regs[m.group(1)] = int(m.group(2), 16)
    return regs


def extract(src, name):
    m = re.search(r"^(static\s+)?[a-z].*\b%s\(" % name, src, re.M)
    if not m:
        raise SystemExit("no function " + name)
    start = src.index("{", m.start())
    depth = 0
    for pos in range(start, len(src)):
        if src[pos] == "{":
            depth += 1
        elif src[pos] == "}":
            depth -= 1
            if not depth:
                return src[m.start():pos + 1]


def transform(code, regs):
    alt = "|".join(sorted(regs, key=len, reverse=True))

    def reads(expr):
        return re.sub(r"\b(%s)\b" % alt,
                      lambda m: "R(0x%x)" % regs[m.group(1)], expr)

    code = code.replace("volatile uint32_t *reg", "uint32_t reg")
    code = code.replace("*reg & bit", "R(reg) & bit")
    code = code.replace("*reg = bit", "W(reg, bit)")
    code = re.sub(r"&(%s)\b" % alt,
                  lambda m: "0x%x" % regs[m.group(1)], code)
    code = re.sub(r"\b(%s)\s*(\|=|&=)\s*([^;]+);" % alt,
                  lambda m: "W(0x%x, R(0x%x) %s (%s));" %
                  (regs[m.group(1)], regs[m.group(1)], m.group(2)[0],
                   reads(m.group(3))), code)
    code = re.sub(r"\b(%s)\s*=(?!=)\s*([^;]+);" % alt,
                  lambda m: "W(0x%x, %s);" %
                  (regs[m.group(1)], reads(m.group(2))), code)
    return reads(code)


PRELUDE = r"""
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
static uint32_t io[0x1000];
static void W(uint32_t off, uint32_t v)
{ io[off] = v; printf("%03x=%08x\n", off, v); }
static uint32_t R(uint32_t off)
{
    if (off == 0x048 || off == 0x840) return 0xffffffff;
    if (off == 0x04c) return 0x40;
    if (off == 0x07c || off == 0x810) return 0;
    return io[off];
}
#define USEC_TIMER 0UL
#define TIME_BEFORE(a, b) 1
#define FMCTRL0_UNK1 (1 << 11)
#define FMCTRL0_AUTOXFER (1 << 24)
#define FMCTRL1_DOTRANSADDR (1 << 0)
#define FMCSTAT_CMDDONE (1 << 1)
#define FMCSTAT_ADDRDONE (1 << 2)
#define FMCSTAT_TRANSDONE (1 << 3)
#define FMCSTAT_UNK20 (1 << 20)
#define FMCSTAT_STATUSREADY (1 << 23)
#define FMCSTAT_UNK27 (1 << 27)
#define NAND_PAGE_SIZE 2048
#define NAND_UNIT_SIZE 2048
#define NAND_ECC_CLEAN 0
#define NAND_ECC_CORRECTED 1
#define NAND_ECC_FAILED 2
struct nand_read { uint32_t bank, page; void *buf; uint32_t *meta; int ecc;
                  uint32_t result; };
struct { uint16_t blocks, pagesperblock, pagesize; } chip = { 4096, 128 };
static typeof(chip) *nand_chip = &chip;
static bool nand_ready = true;
static unsigned int nand_banks = 4;
static void commit_discard_dcache_range(const void *p, int n)
{ (void)p; (void)n; }
static void discard_dcache_range(const void *p, int n)
{ (void)p; (void)n; }
"""


def harness(regs):
    with open(DRIVER) as f:
        src = f.read()
    defines = "\n".join(line for line in src.splitlines()
                         if re.match(r"#define\s+(NAND_CMD_|NAND_STATUS_|"
                                     r"NAND_TUNK1|NAND_TWP|NAND_POLL_SPINS)",
                                     line))
    protos = "\n".join(transform(re.sub(r"\)\s*$", ");",
                                     extract(src, fn).split("{")[0].strip()),
                                 regs) for fn in FUNCS)
    body = "\n\n".join(transform(extract(src, fn), regs) for fn in FUNCS)
    main = r"""
int main(int argc, char **argv)
{
    static struct nand_read r[64];
    static uint32_t meta[64][3];
    unsigned int n = 0;
    uint32_t bank, page;
    chip.pagesize = atoi(argv[1]);
    while (scanf("%u %u", &bank, &page) == 2)
    {
        r[n].bank = bank; r[n].page = page;
        r[n].buf = (void *)(intptr_t)(0x08000000 + n * chip.pagesize);
        r[n].meta = meta[n];
        n++;
    }
    return nand_read_pages(r, n) ? 1 : 0;
}
"""
    return PRELUDE + defines + "\n" + protos + "\n" + body + main


def scenarios():
    return [("one page", [(0, 128000)]),
            ("four banks", [(i, 128000) for i in range(4)]),
            ("same bank", [(0, 128000 + i) for i in range(3)]),
            ("duplicate split", [(0, 128000), (1, 128000),
                                  (0, 128001), (2, 128000)])]


def main():
    regs = registers()
    build = os.path.join(HERE, "build-read")
    os.makedirs(build, exist_ok=True)
    cfile, exe = os.path.join(build, "h.c"), os.path.join(build, "h")
    with open(cfile, "w") as f:
        f.write(harness(regs))
    cc = subprocess.run(["cc", "-std=gnu99", "-w", "-o", exe, cfile],
                        capture_output=True, text=True)
    if cc.returncode:
        print(cc.stderr[:4000])
        return 1
    failures = 0
    for (name, pages), size in [(s, z) for z in (2048, 4096)
                                for s in scenarios()]:
        name = "%s %dKiB" % (name, size // 1024)
        emu = fmss_emu.Emu(fmss_emu.PROGRAMS["read"])
        fmss_emu.setup_list(emu, pages, units=size // 2048)
        emu.io[0xd4c] = BASE_CTRL0
        emu.run()
        apple = ["%03x=%08x" % item for item in emu.raw]
        inp = "".join("%d %d\n" % item for item in pages)
        ours = subprocess.run([exe, str(size)], input=inp,
                              capture_output=True,
                              text=True).stdout.split()
        if ours == apple:
            print("PASS  %-26s %d register writes identical" %
                  (name, len(apple)))
            continue
        failures += 1
        pos = next((i for i, pair in enumerate(zip(ours, apple))
                    if pair[0] != pair[1]), min(len(ours), len(apple)))
        print("FAIL  %-26s first difference %d (ours %d, Apple %d)" %
              (name, pos, len(ours), len(apple)))
        for i in range(max(0, pos - 5), min(max(len(ours), len(apple)),
                                            pos + 8)):
            print(" %4d ours %-14s apple %s" %
                  (i, ours[i] if i < len(ours) else "-",
                   apple[i] if i < len(apple) else "-"))
    print("ALL IDENTICAL" if not failures else
          "%d SCENARIOS DIFFER" % failures)
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
