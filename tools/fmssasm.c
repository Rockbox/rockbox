/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Viktor Krapivin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* fmssasm: assembler for the S5L8720 flash sequencer (FMSS) microcode used
 * by the iPod nano 4G NAND driver.
 *
 *   fmssasm [-p prefix] [-c out.c] [-h out.h] [-s out.h] [-b out.bin] prog.s ...
 *
 * Each prog.s becomes an array <prefix><name> (name = file name without
 * directory and ".s"; default prefix "fmss_prog_") of 32-bit words, two per
 * instruction. -c writes the definitions, -h the matching extern
 * declarations, -s a header with static definitions (for a single user),
 * -b the raw bytes of the (single) program.
 *
 * Instruction encoding: 8 bytes little endian, u16 arg | u8 reg | u8 opcode,
 * then u32 immediate. Source syntax, one instruction per line, ';' starts a
 * comment, "name:" defines a label (its value is the byte offset):
 *   wr   [REG], #imm|#label    01  [REG] = imm
 *   st   [REG], rR             02  [REG] = rR
 *   ldi  rR, [rA]              03  rR = mem32[rA]
 *   ld   rR, [REG][, mask]     04  rR = [REG] & mask (default 0xffffffff)
 *   movi rR, #imm              05  rR = imm
 *   mov  rR, rA                06  rR = rA
 *   wait N[, arg=V, mask=M]    07  wait for event N
 *   and/or/add/sub/shl/shr rR, rA, #imm   0a/0b/0c/0d/13/14  rR = rA op imm
 *   and/or/add/sub/shl/shr rR, rA         same ops, rR = rR op rA
 *   bnz  rR, label             0e  branch if rR != 0
 *   bz   rR, label             17  branch if rR == 0
 *   sti  [rA], rR              11  mem32[rA] = rR
 *   end                        00  stop, r6 is the result
 * Opcodes no original program uses, as measured on the S5L8720:
 *   waitc [N]                  08  wait for condition N (default 0): 0..7 pass at once,
 *                                  N = 8..255 never pass (the timeout fault,
 *                                  or a hold without timeout)
 *   nop                        ff  no operation (also an all-ones word)
 *   waitirq N                  09  wait for CPU interrupt request line N
 *   jr   rT[, rC]              0f  jump to the byte offset in rT if rC != 0
 *                                  (rC defaults to r0)
 *   sig                        10  set CS_IRQ bit 1 and continue
 *   pause                      12  stop; the CPU resumes with PC and a start
 *   tmwait                     15  wait start-word bits 15:8 timeout steps
 *                                  (2^14 cycles each), then continue
 *   evjmp                      16  wait for a command, address, data, DMA,
 *                                  ECC or status latch flag, then jump to
 *                                  the vector in FMSS_C4C without a fault
 *                                  (the flag stays set); at the timeout
 *                                  continue at the next instruction
 *   pld  rR, [rA]              18  rR = window word at byte offset rA
 *   pst  [rA], rR              19  window word at byte offset rA = rR
 * end, nop, sig, pause, tmwait and evjmp take no operands or the raw fields
 * R[, arg[, imm]] of a measured form; waitc takes [N[, arg[, imm]]] and
 * waitirq N[, arg[, imm]].
 * Without them the fields are zero.
 * REG is a name from the table below or a number (offset from the FMC base).
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INSNS   4096
#define MAX_LABELS  1024
#define MAX_LINE    512
#define MAX_FILES   16

struct name_value
{
    const char *name;
    unsigned value;
};

static const struct name_value regs[] =
{
    { "FMC_CTRL0", 0x000 }, { "FMCTRL1", 0x004 }, { "FMC_CMD", 0x008 },
    { "FMC_ADDR0", 0x00c }, { "FMC_ADDR1", 0x010 }, { "FMC_DATA_SEL", 0x014 },
    { "FMC_18", 0x018 }, { "FMC_1C", 0x01c }, { "FMC_ANUM", 0x02c },
    { "FMC_DNUM", 0x030 }, { "FMC_DMA_ADDR", 0x034 }, { "FMC_40", 0x040 },
    { "FMC_STATUS", 0x048 }, { "FMC_58", 0x058 }, { "FMC_5C", 0x05c },
    { "FMC_DATA0?", 0x060 }, { "FMC_64", 0x064 }, { "FMC_68", 0x068 },
    { "ECC_CTRL", 0x804 }, { "ECC_RESULT", 0x80c }, { "ECC_STATUS", 0x810 },
    { "CS_IRQ", 0xc0c }, { "FMSS_C10", 0xc10 }, { "FMSS_C3C", 0xc3c },
    { "FMSS_C4C", 0xc4c }, { "FMSS_C50", 0xc50 }, { "FMSS_C58", 0xc58 },
    { "P_IDBUF", 0xd08 }, { "P_ROWS", 0xd0c }, { "P_CEMASKS", 0xd10 },
    { "P_ECCBUF", 0xd14 }, { "P_COUNT", 0xd18 }, { "P_META", 0xd1c },
    { "P_DMALIST", 0xd20 }, { "P_STATS", 0xd24 }, { "P_CHUNKS", 0xd28 },
    { "P_ECCMODE", 0xd2c }, { "P_ECCCFG", 0xd30 }, { "P_SPARELEN", 0xd34 },
    { "P_HDRLEN", 0xd34 }, { "P_PARLEN", 0xd38 }, { "V_D3C", 0xd3c },
    { "P_TIMING", 0xd4c },
};

enum { OP_END = 0x00, OP_WR = 0x01, OP_ST = 0x02, OP_LDI = 0x03, OP_LD = 0x04,
       OP_MOVI = 0x05, OP_MOV = 0x06, OP_WAIT = 0x07, OP_AND = 0x0a,
       OP_OR = 0x0b, OP_ADD = 0x0c, OP_SUB = 0x0d, OP_BNZ = 0x0e,
       OP_STI = 0x11, OP_SHL = 0x13, OP_SHR = 0x14, OP_BZ = 0x17,
       OP_WAITC = 0x08, OP_WAIT2 = 0x09, OP_JR = 0x0f, OP_SIG = 0x10,
       OP_PAUSE = 0x12, OP_TMWAIT = 0x15, OP_EVJMP = 0x16, OP_PLD = 0x18,
       OP_PST = 0x19, OP_NOP = 0xff };

static const struct name_value opcodes[] =
{
    { "end", OP_END }, { "wr", OP_WR }, { "st", OP_ST }, { "ldi", OP_LDI },
    { "ld", OP_LD }, { "movi", OP_MOVI }, { "mov", OP_MOV },
    { "wait", OP_WAIT }, { "and", OP_AND }, { "or", OP_OR },
    { "add", OP_ADD }, { "sub", OP_SUB }, { "bnz", OP_BNZ },
    { "sti", OP_STI }, { "shl", OP_SHL }, { "shr", OP_SHR }, { "bz", OP_BZ },
    { "waitc", OP_WAITC }, { "nop", OP_NOP },
    { "waitirq", OP_WAIT2 },
    { "jr", OP_JR }, { "sig", OP_SIG }, { "pause", OP_PAUSE },
    { "tmwait", OP_TMWAIT }, { "evjmp", OP_EVJMP }, { "pld", OP_PLD },
    { "pst", OP_PST },
};

struct program
{
    char name[128];
    const char *path;
    uint32_t words[2 * MAX_INSNS];
    unsigned ninsns;
};

static struct
{
    char name[64];
    unsigned value;
} labels[MAX_LABELS];
static unsigned nlabels;
static const char *cur_path;
static int cur_line;

static void fail(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "%s:%d: ", cur_path, cur_line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static char *trim(char *s)
{
    char *e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

/* Strip the comment and split off any "label:" prefixes. In pass 1 labels
 * are defined at byte offset pc. Returns the remaining statement or NULL. */
static char *statement(char *line, unsigned pc, int define)
{
    char *s = trim(line), *colon;
    char *semi = strchr(s, ';');
    if (semi)
        *semi = '\0';
    s = trim(s);
    while ((colon = strchr(s, ':')) != NULL)
    {
        char *p;
        for (p = s; p < colon; p++)
            if (!(isalnum((unsigned char)*p) || *p == '_' || *p == '.'))
                break;
        if (p != colon || colon == s)
            break;
        *colon = '\0';
        if (define)
        {
            unsigned i;
            for (i = 0; i < nlabels; i++)
                if (!strcmp(labels[i].name, s))
                    fail("label '%s' defined twice", s);
            if (nlabels == MAX_LABELS || strlen(s) >= sizeof(labels[0].name))
                fail("too many labels or label too long");
            strcpy(labels[nlabels].name, s);
            labels[nlabels++].value = pc;
        }
        s = trim(colon + 1);
    }
    return *s ? s : NULL;
}

static unsigned value(const char *text)
{
    char buf[MAX_LINE], *t, *end;
    unsigned long v;
    unsigned i;
    snprintf(buf, sizeof(buf), "%s", text);
    t = trim(buf);
    if (*t == '#')
        t = trim(t + 1);
    for (i = 0; i < nlabels; i++)
        if (!strcmp(labels[i].name, t))
            return labels[i].value;
    v = strtoul(t, &end, 0);
    if (!*t || *end)
        fail("bad value '%s'", t);
    return (unsigned)v;
}

static unsigned reg(const char *text)
{
    char buf[MAX_LINE], *t;
    snprintf(buf, sizeof(buf), "%s", text);
    t = trim(buf);
    if (t[0] == '[')
    {
        size_t n = strlen(t);
        if (t[n - 1] != ']')
            fail("bad register '%s'", text);
        t[n - 1] = '\0';
        t = trim(t + 1);
    }
    if (t[0] != 'r' || t[1] < '0' || t[1] > '7' || t[2])
        fail("bad register '%s'", text);
    return t[1] - '0';
}

static unsigned mmio(const char *text)
{
    char buf[MAX_LINE], *t, *end;
    size_t n;
    unsigned i;
    unsigned long v;
    snprintf(buf, sizeof(buf), "%s", text);
    t = trim(buf);
    n = strlen(t);
    if (n < 2 || t[0] != '[' || t[n - 1] != ']')
        fail("expected [REGISTER], got '%s'", text);
    t[n - 1] = '\0';
    t = trim(t + 1);
    for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
        if (!strcmp(regs[i].name, t))
            return regs[i].value;
    v = strtoul(t, &end, 0);
    if (!*t || *end)
        fail("unknown register '%s'", t);
    return (unsigned)v;
}

/* split "a, b, c" into at most 4 operands */
static int operands(char *s, char *ops[4])
{
    int n = 0;
    char *p;
    if (!*s)
        return 0;
    while (n < 4)
    {
        ops[n++] = s;
        p = strchr(s, ',');
        if (!p)
            break;
        *p = '\0';
        s = p + 1;
    }
    return n;
}

static void need(int n, int want)
{
    if (n < want)
        fail("missing operand");
}

static void encode(struct program *prog, char *stmt)
{
    char *ops[4] = { 0 }, *rest = stmt, mnem[16];
    unsigned i, arg = 0, r = 0, op, imm = 0;
    int n, len = 0;

    while (*rest && !isspace((unsigned char)*rest) && len < 15)
        mnem[len++] = tolower((unsigned char)*rest++);
    mnem[len] = '\0';
    for (i = 0; i < sizeof(opcodes) / sizeof(opcodes[0]); i++)
        if (!strcmp(opcodes[i].name, mnem))
            break;
    if (i == sizeof(opcodes) / sizeof(opcodes[0]))
        fail("unknown instruction '%s'", mnem);
    op = opcodes[i].value;
    n = operands(trim(rest), ops);

    switch (op)
    {
        case OP_END:
        case OP_WAITC:
        case OP_NOP:
        case OP_SIG:
        case OP_PAUSE:
        case OP_TMWAIT:
        case OP_EVJMP:
        case OP_WAIT2:
            /* no operands, or the raw fields R, arg, imm of a measured form;
             * waitirq requires its line N, waitc defaults to condition 0 */
            if (n > 3)
                fail("'%s' takes at most R, arg, imm", mnem);
            if (op == OP_WAIT2)
                need(n, 1);
            if (n > 0) r = value(ops[0]);
            if (n > 1) arg = value(ops[1]);
            if (n > 2) imm = value(ops[2]);
            break;
        case OP_JR:
            need(n, 1); arg = reg(ops[0]);
            if (n > 1)
                r = reg(ops[1]);
            break;
        case OP_WR:
            need(n, 2); arg = mmio(ops[0]); imm = value(ops[1]);
            break;
        case OP_ST:
            need(n, 2); arg = mmio(ops[0]); r = reg(ops[1]);
            break;
        case OP_LD:
            need(n, 2); r = reg(ops[0]); arg = mmio(ops[1]);
            imm = n > 2 ? value(ops[2]) : 0xffffffff;
            break;
        case OP_LDI:
        case OP_PLD:
            need(n, 2); r = reg(ops[0]); arg = reg(ops[1]);
            break;
        case OP_STI:
        case OP_PST:
            need(n, 2); arg = reg(ops[0]); r = reg(ops[1]);
            break;
        case OP_MOVI:
            need(n, 2); r = reg(ops[0]); imm = value(ops[1]);
            break;
        case OP_MOV:
            need(n, 2); r = reg(ops[0]); arg = reg(ops[1]);
            break;
        case OP_WAIT:
            need(n, 1); r = value(ops[0]);
            for (i = 1; i < (unsigned)n; i++)
            {
                char *eq = strchr(ops[i], '='), *key;
                if (!eq)
                    fail("bad wait option '%s'", ops[i]);
                *eq = '\0';
                key = trim(ops[i]);
                if (!strcmp(key, "arg"))
                    arg = value(eq + 1);
                else if (!strcmp(key, "mask"))
                    imm = value(eq + 1);
                else
                    fail("bad wait option '%s'", key);
            }
            break;
        case OP_BNZ:
        case OP_BZ:
            need(n, 2); r = reg(ops[0]); imm = value(ops[1]);
            break;
        default: /* ALU */
            need(n, 2); r = reg(ops[0]); arg = reg(ops[1]);
            if (n > 2)
            {
                imm = value(ops[2]);
                if (imm == 0)
                    fail("immediate 0 selects the register form");
            }
            break;
    }
    if (arg > 0xffff || r > 0xff)
        fail("operand out of range");
    if (prog->ninsns == MAX_INSNS)
        fail("program too long");
    prog->words[2 * prog->ninsns] = arg | (r << 16) | (op << 24);
    prog->words[2 * prog->ninsns + 1] = imm;
    prog->ninsns++;
}

static void assemble(struct program *prog)
{
    char line[MAX_LINE], copy[MAX_LINE];
    int pass;
    FILE *f;

    cur_path = prog->path;
    nlabels = 0;
    for (pass = 1; pass <= 2; pass++)
    {
        unsigned pc = 0;
        f = fopen(prog->path, "r");
        if (!f)
        {
            perror(prog->path);
            exit(1);
        }
        cur_line = 0;
        prog->ninsns = 0;
        while (fgets(line, sizeof(line), f))
        {
            char *s;
            cur_line++;
            if (!strchr(line, '\n') && !feof(f))
                fail("line too long");
            strcpy(copy, line);
            s = statement(copy, pc, pass == 1);
            if (!s)
                continue;
            if (pass == 2)
                encode(prog, s);
            pc += 8;
        }
        fclose(f);
    }
}

static void set_name(struct program *prog, const char *prefix)
{
    const char *base = strrchr(prog->path, '/');
    size_t n;
    char *p;
    base = base ? base + 1 : prog->path;
    n = strlen(base);
    if (n > 2 && !strcmp(base + n - 2, ".s"))
        n -= 2;
    if (strlen(prefix) + n >= sizeof(prog->name))
    {
        fprintf(stderr, "%s: name too long\n", prog->path);
        exit(1);
    }
    snprintf(prog->name, sizeof(prog->name), "%s%.*s", prefix, (int)n, base);
    for (p = prog->name; *p; p++)
        if (!isalnum((unsigned char)*p))
            *p = '_';
}

static FILE *open_out(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
    {
        perror(path);
        exit(1);
    }
    return f;
}

static void write_defs(FILE *f, struct program *progs, int nprogs,
                       const char *storage)
{
    int i;
    unsigned k;
    for (i = 0; i < nprogs; i++)
    {
        const char *base = strrchr(progs[i].path, '/');
        fprintf(f, "\n/* %s, %u instructions */\n",
                base ? base + 1 : progs[i].path, progs[i].ninsns);
        fprintf(f, "%sconst uint32_t %s[%u] __attribute__((aligned(32))) =\n{\n",
                storage, progs[i].name, 2 * progs[i].ninsns);
        for (k = 0; k < 2 * progs[i].ninsns; k++)
            fprintf(f, "%s0x%08lx,%s", k % 4 ? " " : "    ",
                    (unsigned long)progs[i].words[k],
                    k % 4 == 3 || k + 1 == 2 * progs[i].ninsns ? "\n" : "");
        fprintf(f, "};\n");
    }
}

static void usage(void)
{
    fprintf(stderr, "usage: fmssasm [-p prefix] [-c out.c] [-h out.h] "
                    "[-b out.bin] prog.s ...\n");
    exit(1);
}

int main(int argc, char **argv)
{
    static struct program progs[MAX_FILES];
    const char *prefix = "fmss_prog_", *cout = NULL, *hout = NULL, *bout = NULL;
    const char *sout = NULL;
    int i, nprogs = 0;
    unsigned k;
    FILE *f;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-p") && i + 1 < argc)
            prefix = argv[++i];
        else if (!strcmp(argv[i], "-c") && i + 1 < argc)
            cout = argv[++i];
        else if (!strcmp(argv[i], "-h") && i + 1 < argc)
            hout = argv[++i];
        else if (!strcmp(argv[i], "-s") && i + 1 < argc)
            sout = argv[++i];
        else if (!strcmp(argv[i], "-b") && i + 1 < argc)
            bout = argv[++i];
        else if (argv[i][0] == '-')
            usage();
        else if (nprogs == MAX_FILES)
            usage();
        else
            progs[nprogs++].path = argv[i];
    }
    if (!nprogs || (!cout && !hout && !sout && !bout) || (bout && nprogs != 1))
        usage();

    for (i = 0; i < nprogs; i++)
    {
        set_name(&progs[i], prefix);
        assemble(&progs[i]);
    }

    if (cout)
    {
        f = open_out(cout);
        fprintf(f, "/* Generated by tools/fmssasm. Do not edit: change the "
                   ".s sources. */\n\n#include <stdint.h>\n");
        write_defs(f, progs, nprogs, "");
        fclose(f);
    }
    if (hout)
    {
        f = open_out(hout);
        fprintf(f, "/* Generated by tools/fmssasm. Do not edit: change the "
                   ".s sources. */\n\n#ifndef FMSSASM_GENERATED_H\n"
                   "#define FMSSASM_GENERATED_H\n\n#include <stdint.h>\n\n");
        for (i = 0; i < nprogs; i++)
            fprintf(f, "extern const uint32_t %s[%u];\n", progs[i].name,
                    2 * progs[i].ninsns);
        fprintf(f, "\n#endif\n");
        fclose(f);
    }
    if (sout)
    {
        f = open_out(sout);
        fprintf(f, "/* Generated by tools/fmssasm. Do not edit: change the "
                   ".s sources. */\n\n#ifndef FMSSASM_GENERATED_H\n"
                   "#define FMSSASM_GENERATED_H\n\n#include <stdint.h>\n");
        write_defs(f, progs, nprogs, "static ");
        fprintf(f, "\n#endif\n");
        fclose(f);
    }
    if (bout)
    {
        f = fopen(bout, "wb");
        if (!f)
        {
            perror(bout);
            return 1;
        }
        for (k = 0; k < 2 * progs[0].ninsns; k++)
        {
            uint32_t w = progs[0].words[k];
            unsigned char b[4] = { w & 0xff, (w >> 8) & 0xff,
                                   (w >> 16) & 0xff, w >> 24 };
            fwrite(b, 1, 4, f);
        }
        fclose(f);
    }
    return 0;
}
