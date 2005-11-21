/*
 * sh2d
 * Bart Trzynadlowski, July 24, 2000
 * Public domain
 *
 * Some changes by Björn Stenberg <bjorn@haxx.se>
 * $Id$
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.2"

#define ZERO_F  0       /* 0 format */
#define N_F     1       /* n format */
#define M_F     2       /* m format */
#define NM_F    3       /* nm format */
#define MD_F    4       /* md format */
#define ND4_F   5       /* nd4 format */
#define NMD_F   6       /* nmd format */
#define D_F     7       /* d format */
#define D12_F   8       /* d12 format */
#define ND8_F   9       /* nd8 format */
#define I_F     10      /* i format */
#define NI_F    11      /* ni format */

typedef struct
{
        int             format;
        const char   *mnem;
        unsigned short  mask;   /* mask used to obtain opcode bits */
        unsigned short  bits;   /* opcode bits */
        int             dat;    /* specific data for situation */
        int             sh2;    /* SH-2 specific */
} i_descr;

/* register name lookup added by bjorn@haxx.se 2001-12-09 */
char* regname[] =
{
 /* 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
   "","","","","","","","","","","","","","","","", /* 0 */
   "","","","","","","","","","","","","","","","", /* 10 */
   "","","","","","","","","","","","","","","","", /* 20 */
   "","","","","","","","","","","","","","","","", /* 30 */
   "","","","","","","","","","","","","","","","", /* 40 */
   "","","","","","","","","","","","","","","","", /* 50 */
   "","","","","","","","","","","","","","","","", /* 60 */
   "","","","","","","","","","","","","","","","", /* 70 */
   "","","","","","","","","","","","","","","","", /* 80 */
   "","","","","","","","","","","","","","","","", /* 90 */
   "","","","","","","","","","","","","","","","", /* a0 */
   "","","","","","","","","","","","","","","","", /* b0 */

   "SMR0","BRR0","SCR0","TDR0","SSR0","RDR0","","",    /* c0 */
   "SMR1","BRR1","SCR1","TDR1","SSR1","RDR1","","",    /* c8 */
   "","","","","","","","","","","","","","","","",    /* d0 */
   "ADDRAH","ADDRAL","ADDRBH","ADDRBL",                /* e0 */
   "ADDRCH","ADDRCL","ADDRDH","ADDRDL",                /* e4 */
   "ADCSR","ADCR","","","","","","",                   /* e8 */
   "","","","","","","","","","","","","","","","",    /* f0 */
   "TSTR","TSNC","TMDR","TFCR","TCR0","TIOR0","TIER0","TSR0", /* 100 */
   "TCNT0","!","GRA0","!","GRB0","!","TCR1","TIORL",      /* 108 */
   "TIERI","TSR1","TCNT1","!","GRA1","!","GRB1","!",      /* 110 */
   "TCR2","TIOR2","TIER2","TSR2","TCNT2","!","GRA2","!",  /* 118 */
   "GRB2","!","TCR3","TIOR3","TIER3","TSR3","TCNT3","!",  /* 120 */
   "GRA3","!","GRB3","!","BRA3","!","BRB3","!",           /* 128 */
   "","TOCR","TCR4","TIOR4","TIER4","TSR4","TCNT4","!",   /* 130 */
   "GRA4","!","GRB4","!","BRA4","!","BRB4","!",           /* 138 */
   "SAR0","!","!","!","DAR0","!","!","!",                 /* 140 */
   "DMAOR","!","TCR0","!","","","CHCR0","!",              /* 148 */
   "SAR1","!","!","!","DAR1","!","!","!",                 /* 150 */
   "","","TCR1","!","","","CHCR1","!",                    /* 158 */
   "SAR2","!","!","!","DAR2","!","!","!",                 /* 160 */
   "","","TCR2","!","","","CHCR2","!",                    /* 168 */
   "SAR3","!","!","!","DAR3","!","!","!",                 /* 170 */
   "","","TCR3","!","","","CHCR3","!",                    /* 178 */
   "","","","","IPRA","!","IPRB","!",                     /* 180 */
   "IPRC","!","IPRD","!","IPRE","!","ICR","!",            /* 188 */
   "BARH","!","BARL","!","BAMRH","!","BAMRL","!",         /* 190 */
   "BBR","!","","","","","","",                           /* 198 */
   "BCR","!","WCR1","!","WCR2","!","WCR3","!",            /* 1a0 */
   "DCR","!","PCR","!","RCR","!","RTCSR","!",             /* 1a8 */
   "RTCNT","!","RTCOR","!","","","","",                   /* 1b0 */
   "TCSR","TCNT","","RSTCSR","SBYCR","","","",            /* 1b8 */
   "PADR","!","PBDR","!","PAIOR","!","PBIOR","!",         /* 1c0 */
   "PACR1","!","PACR2","!","PBCR1","!","PBCR2","!",       /* 1c8 */
   "PCDR","!","","","","","","","","","","","","","","",  /* 1d0 */
   "","","","","","","","","","","","","","","CASCR","!", /* 1e0 */
   "TPMR","TPCR","NDERB","NDERA","NDRB","NDRA","NDRB","NDRA", /* 1f0 */
   "","","","","","","",""
};

i_descr tab[] =
{
        { ZERO_F,       "clrt",                 0xffff, 0x8,    0,      0 },
        { ZERO_F,       "clrmac",               0xffff, 0x28,   0,      0 },
        { ZERO_F,       "div0u",                0xffff, 0x19,   0,      0 },
        { ZERO_F,       "nop",                  0xffff, 0x9,    0,      0 },
        { ZERO_F,       "rte",                  0xffff, 0x2b,   0,      0 },
        { ZERO_F,       "rts",                  0xffff, 0xb,    0,      0 },
        { ZERO_F,       "sett",                 0xffff, 0x18,   0,      0 },
        { ZERO_F,       "sleep",                0xffff, 0x1b,   0,      0 },
        { N_F,          "cmp/pl\tr%d",          0xf0ff, 0x4015, 0,      0 },
        { N_F,          "cmp/pz\tr%d",          0xf0ff, 0x4011, 0,      0 },
        { N_F,          "dt\tr%d",              0xf0ff, 0x4010, 0,      1 },
        { N_F,          "movt\tr%d",            0xf0ff, 0x0029, 0,      0 },
        { N_F,          "rotl\tr%d",            0xf0ff, 0x4004, 0,      0 },
        { N_F,          "rotr\tr%d",            0xf0ff, 0x4005, 0,      0 },
        { N_F,          "rotcl\tr%d",           0xf0ff, 0x4024, 0,      0 },
        { N_F,          "rotcr\tr%d",           0xf0ff, 0x4025, 0,      0 },
        { N_F,          "shal\tr%d",            0xf0ff, 0x4020, 0,      0 },
        { N_F,          "shar\tr%d",            0xf0ff, 0x4021, 0,      0 },
        { N_F,          "shll\tr%d",            0xf0ff, 0x4000, 0,      0 },
        { N_F,          "shlr\tr%d",            0xf0ff, 0x4001, 0,      0 },
        { N_F,          "shll2\tr%d",           0xf0ff, 0x4008, 0,      0 },
        { N_F,          "shlr2\tr%d",           0xf0ff, 0x4009, 0,      0 },
        { N_F,          "shll8\tr%d",           0xf0ff, 0x4018, 0,      0 },
        { N_F,          "shlr8\tr%d",           0xf0ff, 0x4019, 0,      0 },
        { N_F,          "shll16\tr%d",          0xf0ff, 0x4028, 0,      0 },
        { N_F,          "shlr16\tr%d",          0xf0ff, 0x4029, 0,      0 },
        { N_F,          "stc\tsr,r%d",          0xf0ff, 0x0002, 0,      0 },
        { N_F,          "stc\tgbr,r%d",         0xf0ff, 0x0012, 0,      0 },
        { N_F,          "stc\tvbr,r%d",         0xf0ff, 0x0022, 0,      0 },
        { N_F,          "sts\tmach,r%d",        0xf0ff, 0x000a, 0,      0 },
        { N_F,          "sts\tmacl,r%d",        0xf0ff, 0x001a, 0,      0 },
        { N_F,          "sts\tpr,r%d",          0xf0ff, 0x002a, 0,      0 },
        { N_F,          "tas.b\t@r%d",          0xf0ff, 0x401b, 0,      0 },
        { N_F,          "stc.l\tsr,@-r%d",      0xf0ff, 0x4003, 0,      0 },
        { N_F,          "stc.l\tgbr,@-r%d",     0xf0ff, 0x4013, 0,      0 },
        { N_F,          "stc.l\tvbr,@-r%d",     0xf0ff, 0x4023, 0,      0 },
        { N_F,          "sts.l\tmach,@-r%d",    0xf0ff, 0x4002, 0,      0 },
        { N_F,          "sts.l\tmacl,@-r%d",    0xf0ff, 0x4012, 0,      0 },
        { N_F,          "sts.l\tpr,@-r%d",      0xf0ff, 0x4022, 0,      0 },
        { M_F,          "ldc\tr%d,sr",          0xf0ff, 0x400e, 0,      0 },
        { M_F,          "ldc\tr%d,gbr",         0xf0ff, 0x401e, 0,      0 },
        { M_F,          "ldc\tr%d,vbr",         0xf0ff, 0x402e, 0,      0 },
        { M_F,          "lds\tr%d,mach",        0xf0ff, 0x400a, 0,      0 },
        { M_F,          "lds\tr%d,macl",        0xf0ff, 0x401a, 0,      0 },
        { M_F,          "lds\tr%d,pr",          0xf0ff, 0x402a, 0,      0 },
        { M_F,          "jmp\t@r%d",            0xf0ff, 0x402b, 0,      0 },
        { M_F,          "jsr\t@r%d",            0xf0ff, 0x400b, 0,      0 },
        { M_F,          "ldc.l\t@r%d+,sr",      0xf0ff, 0x4007, 0,      0 },
        { M_F,          "ldc.l\t@r%d+,gbr",     0xf0ff, 0x4017, 0,      0 },
        { M_F,          "ldc.l\t@r%d+,vbr",     0xf0ff, 0x4027, 0,      0 },
        { M_F,          "lds.l\t@r%d+,mach",    0xf0ff, 0x4006, 0,      0 },
        { M_F,          "lds.l\t@r%d+,macl",    0xf0ff, 0x4016, 0,      0 },
        { M_F,          "lds.l\t@r%d+,pr",      0xf0ff, 0x4026, 0,      0 },
        { M_F,          "braf\tr%d",            0xf0ff, 0x0023, 0,      1 },
        { M_F,          "bsrf\tr%d",            0xf0ff, 0x0003, 0,      1 },
        { NM_F,         "add\tr%d,r%d",        0xf00f, 0x300c, 0,      0 },
        { NM_F,         "addc\tr%d,r%d",       0xf00f, 0x300e, 0,      0 },
        { NM_F,         "addv\tr%d,r%d",       0xf00f, 0x300f, 0,      0 },
        { NM_F,         "and\tr%d,r%d",        0xf00f, 0x2009, 0,      0 },
        { NM_F,         "cmp/eq\tr%d,r%d",     0xf00f, 0x3000, 0,      0 },
        { NM_F,         "cmp/hs\tr%d,r%d",     0xf00f, 0x3002, 0,      0 },
        { NM_F,         "cmp/ge\tr%d,r%d",     0xf00f, 0x3003, 0,      0 },
        { NM_F,         "cmp/hi\tr%d,r%d",     0xf00f, 0x3006, 0,      0 },
        { NM_F,         "cmp/gt\tr%d,r%d",     0xf00f, 0x3007, 0,      0 },
        { NM_F,         "cmp/str\tr%d,r%d",    0xf00f, 0x200c, 0,      0 },
        { NM_F,         "div1\tr%d,r%d",       0xf00f, 0x3004, 0,      0 },
        { NM_F,         "div0s\tr%d,r%d",      0xf00f, 0x2007, 0,      0 },
        { NM_F,         "dmuls.l\tr%d,r%d",    0xf00f, 0x300d, 0,      1 },
        { NM_F,         "dmulu.l\tr%d,r%d",    0xf00f, 0x3005, 0,      1 },
        { NM_F,         "exts.b\tr%d,r%d",     0xf00f, 0x600e, 0,      0 },
        { NM_F,         "exts.w\tr%d,r%d",     0xf00f, 0x600f, 0,      0 },
        { NM_F,         "extu.b\tr%d,r%d",     0xf00f, 0x600c, 0,      0 },
        { NM_F,         "extu.w\tr%d,r%d",     0xf00f, 0x600d, 0,      0 },
        { NM_F,         "mov\tr%d,r%d",        0xf00f, 0x6003, 0,      0 },
        { NM_F,         "mul.l\tr%d,r%d",      0xf00f, 0x0007, 0,      1 },
        { NM_F,         "muls.w\tr%d,r%d",     0xf00f, 0x200f, 0,      0 },
        { NM_F,         "mulu.w\tr%d,r%d",     0xf00f, 0x200e, 0,      0 },
        { NM_F,         "neg\tr%d,r%d",        0xf00f, 0x600b, 0,      0 },
        { NM_F,         "negc\tr%d,r%d",       0xf00f, 0x600a, 0,      0 },
        { NM_F,         "not\tr%d,r%d",        0xf00f, 0x6007, 0,      0 },
        { NM_F,         "or\tr%d,r%d",         0xf00f, 0x200b, 0,      0 },
        { NM_F,         "sub\tr%d,r%d",        0xf00f, 0x3008, 0,      0 },
        { NM_F,         "subc\tr%d,r%d",       0xf00f, 0x300a, 0,      0 },
        { NM_F,         "subv\tr%d,r%d",       0xf00f, 0x300b, 0,      0 },
        { NM_F,         "swap.b\tr%d,r%d",     0xf00f, 0x6008, 0,      0 },
        { NM_F,         "swap.w\tr%d,r%d",     0xf00f, 0x6009, 0,      0 },
        { NM_F,         "tst\tr%d,r%d",        0xf00f, 0x2008, 0,      0 },
        { NM_F,         "xor\tr%d,r%d",        0xf00f, 0x200a, 0,      0 },
        { NM_F,         "xtrct\tr%d,r%d",      0xf00f, 0x200d, 0,      0 },
        { NM_F,         "mov.b\tr%d,@r%d",     0xf00f, 0x2000, 0,      0 },
        { NM_F,         "mov.w\tr%d,@r%d",     0xf00f, 0x2001, 0,      0 },
        { NM_F,         "mov.l\tr%d,@r%d",     0xf00f, 0x2002, 0,      0 },
        { NM_F,         "mov.b\t@r%d,r%d",     0xf00f, 0x6000, 0,      0 },
        { NM_F,         "mov.w\t@r%d,r%d",     0xf00f, 0x6001, 0,      0 },
        { NM_F,         "mov.l\t@r%d,r%d",     0xf00f, 0x6002, 0,      0 },
        { NM_F,         "mac.l\t@r%d+,@r%d+",  0xf00f, 0x000f, 0,      1 },
        { NM_F,         "mac.w\t@r%d+,@r%d+",  0xf00f, 0x400f, 0,      0 },
        { NM_F,         "mov.b\t@r%d+,r%d",    0xf00f, 0x6004, 0,      0 },
        { NM_F,         "mov.w\t@r%d+,r%d",    0xf00f, 0x6005, 0,      0 },
        { NM_F,         "mov.l\t@r%d+,r%d",    0xf00f, 0x6006, 0,      0 },
        { NM_F,         "mov.b\tr%d,@-r%d",    0xf00f, 0x2004, 0,      0 },
        { NM_F,         "mov.w\tr%d,@-r%d",    0xf00f, 0x2005, 0,      0 },
        { NM_F,         "mov.l\tr%d,@-r%d",    0xf00f, 0x2006, 0,      0 },
        { NM_F,         "mov.b\tr%d,@(r0,r%d)", 0xf00f, 0x0004, 0,    0 },
        { NM_F,         "mov.w\tr%d,@(r0,r%d)", 0xf00f, 0x0005, 0,    0 },
        { NM_F,         "mov.l\tr%d,@(r0,r%d)", 0xf00f, 0x0006, 0,    0 },
        { NM_F,         "mov.b\t@(r0,r%d),r%d", 0xf00f, 0x000c, 0,    0 },
        { NM_F,         "mov.w\t@(r0,r%d),r%d", 0xf00f, 0x000d, 0,    0 },
        { NM_F,         "mov.l\t@(r0,r%d),r%d", 0xf00f, 0x000e, 0,    0 },
        { MD_F,         "mov.b\t@(0x%03X,r%d), r0", 0xff00, 0x8400, 0, 0 },
        { MD_F,         "mov.w\t@(0x%03X,r%d), r0", 0xff00, 0x8500, 0, 0 },
        { ND4_F,        "mov.b\tr0,@(0x%03X,r%d)", 0xff00, 0x8000, 0, 0 },
        { ND4_F,        "mov.w\tr0,@(0x%03X,r%d)", 0xff00, 0x8100, 0, 0 },
        { NMD_F,        "mov.l\tr%d,@(0x%03X,r%d)", 0xf000, 0x1000, 0,0 },
        { NMD_F,        "mov.l\t@(0x%03X,r%d),r%d", 0xf000, 0x5000, 0,0 },
        { D_F,          "mov.b\tr0,@(0x%03X,gbr)", 0xff00, 0xc000, 1, 0 },
        { D_F,          "mov.w\tr0,@(0x%03X,gbr)", 0xff00, 0xc100, 2, 0 },
        { D_F,          "mov.l\tr0,@(0x%03X,gbr)", 0xff00, 0xc200, 4, 0 },
        { D_F,          "mov.b\t@(0x%03X,gbr),r0", 0xff00, 0xc400, 1, 0 },
        { D_F,          "mov.w\t@(0x%03X,gbr),r0", 0xff00, 0xc500, 2, 0 },
        { D_F,          "mov.l\t@(0x%03X,gbr),r0", 0xff00, 0xc600, 4, 0 },
        { D_F,          "mova\t@(0x%03X,pc),r0", 0xff00, 0xc700, 4,   0 },
        { D_F,          "bf\t0x%08X",           0xff00, 0x8b00, 5,      0 },
        { D_F,          "bf/s\t0x%08X",         0xff00, 0x8f00, 5,      1 },
        { D_F,          "bt\t0x%08X",           0xff00, 0x8900, 5,      0 },
        { D_F,          "bt/s\t0x%08X",         0xff00, 0x8d00, 5,      1 },
        { D12_F,        "bra\t0x%08X",          0xf000, 0xa000, 0,      0 },
        { D12_F,        "bsr\t0x%08X",          0xf000, 0xb000, 0,      0 },
        { ND8_F,        "mov.w\t@(0x%03X,pc),r%d", 0xf000, 0x9000, 2, 0 },
        { ND8_F,        "mov.l\t@(0x%03X,pc),r%d", 0xf000, 0xd000, 4, 0 },
        { I_F,          "and.b\t#0x%02X,@(r0,gbr)", 0xff00, 0xcd00, 0,0 },
        { I_F,          "or.b\t#0x%02X,@(r0,gbr)",  0xff00, 0xcf00, 0,0 },
        { I_F,          "tst.b\t#0x%02X,@(r0,gbr)", 0xff00, 0xcc00, 0,0 },
        { I_F,          "xor.b\t#0x%02X,@(r0,gbr)", 0xff00, 0xce00, 0,0 },
        { I_F,          "and\t#0x%02X,r0",     0xff00, 0xc900, 0,      0 },
        { I_F,          "cmp/eq\t#0x%02X,r0",  0xff00, 0x8800, 0,      0 },
        { I_F,          "or\t#0x%02X,r0",      0xff00, 0xcb00, 0,      0 },
        { I_F,          "tst\t#0x%02X,r0",     0xff00, 0xc800, 0,      0 },
        { I_F,          "xor\t#0x%02X,r0",     0xff00, 0xca00, 0,      0 },
        { I_F,          "trapa\t#0x%X",         0xff00, 0xc300, 0,      0 },
        { NI_F,         "add\t#0x%02X,r%d",    0xf000, 0x7000, 0,      0 },
        { NI_F,         "mov\t#0x%02X,r%d",    0xf000, 0xe000, 0,      0 },
        { 0,            NULL,                   0,      0,      0,      0 }
};


int FindOption(char *option, int p, int h, int u, int argc, char **argv)
{
        static int      t[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0 };
        int             i;
        char            *c;
      
        if (argc > 128)
                argc = 128;     /* maximum this function can handle is 128 */

        /*
         * if p = 1 and h = 0 will find option and return decimal value of
         * argv[i+1], if h = 1 will read it as hex.
         * if p = 0 then it will return index of option in argv[], 0 not found
         * if u = 1 will return index of first occurance of untouched option
         */

        if (u)  /* find first untouched element */
        {
                for (i = 1; i < argc; i++)
                {
                        if (!t[i])      /* 0 indicates untouched */
                                return i;
                }
                return 0;
        }

        if (p)  /* find option and return integer value following it */
        {
                for (i = 1; i < argc; i++)
                {                
                        if (strcmp(argv[i], option) == 0)       /* found */
                        {
                                if (i >= argc)                  /* bounds! */
                                        return 0;
                                t[i + 1] = t[i] = 1;            /* touched */
                                if (!h)
                                        return atoi(argv[i + 1]);
                                else
                                        return strtoul(argv[i + 1], &c, 16);
                        }
                }
                return 0;                                       /* no match */
        }
        else    /* find option and return position */        
        {
                for (i = 1; i < argc; i++)
                {                
                        if (strcmp(argv[i], option) == 0)
                        {
                                t[i] = 1;
                                return i;       /* found! return position */
                        }
                }                                                          
                return 0;
        }
}

/*
 * SH2Disasm(): SH-1/SH-2 disassembler routine. If mode = 0 then SH-2 mode,
 *              otherwise SH-1 mode
 */
 
void SH2Disasm(unsigned v_addr, unsigned char *p_addr, int mode, char *m_addr)
  {
    int            i;
    unsigned short op;

    op =  (unsigned short) (*p_addr << 8) | *(p_addr + 1);
    printf("0x%08X: 0x%04X\t", v_addr, op);

    if (m_addr[0]==ND8_F)
      {
        if (m_addr[2]==-1)
          {
            unsigned int tmp = (op << 16) | ((unsigned int) (p_addr [2] << 8) | p_addr[3]);
            printf(".long\t0x%08X\t; 0x%08X",tmp,v_addr - (unsigned)m_addr[1]);
          }
        else
          printf(".short\t0x%08X\t; 0x%08X",op,v_addr - (unsigned)m_addr[1]);
      }
    else if (m_addr[0] != -1)
      {
        for (i = 0; tab[i].mnem != NULL; i++)   /* 0 format */
          {
            if ((op & tab[i].mask) == tab[i].bits)
              {
                if (tab[i].sh2 && mode) /* if SH-1 mode, no SH-2 */
                  printf("???");
                else if (tab[i].format == ZERO_F)
                  printf("%s", tab[i].mnem);
                else if (tab[i].format == N_F)
                  printf(tab[i].mnem, (op >> 8) & 0xf);
                else if (tab[i].format == M_F)
                  printf(tab[i].mnem, (op >> 8) & 0xf);
                else if (tab[i].format == NM_F)
                  printf(tab[i].mnem, (op >> 4) & 0xf,
                  (op >> 8) & 0xf);
                else if (tab[i].format == MD_F)
                  {
                    if (op & 0x100)
                      printf(tab[i].mnem, (op & 0xf) * 2,
                      (op >> 4) & 0xf);
                    else
                      printf(tab[i].mnem, op & 0xf,
                      (op >> 4) & 0xf);
                  }
                else if (tab[i].format == ND4_F)
                  {
                    if (op & 0x100)
                      printf(tab[i].mnem, (op & 0xf) * 2,
                      (op >> 4) & 0xf);
                    else
                      printf(tab[i].mnem, (op & 0xf),
                      (op >> 4) & 0xf);
                  }
                else if (tab[i].format == NMD_F)
                  {
                    if ((op & 0xf000) == 0x1000)
                      printf(tab[i].mnem, (op >> 4) & 0xf,
                      (op & 0xf) * 4,
                      (op >> 8) & 0xf);
                    else
                      printf(tab[i].mnem, (op & 0xf) * 4,
                      (op >> 4) & 0xf,
                      (op >> 8) & 0xf);
                  }
                else if (tab[i].format == D_F)
                  {
                    if (tab[i].dat <= 4)
                      {
                      if ((op & 0xff00) == 0xc700)
                        {
                          printf(tab[i].mnem,
                            (op & 0xff) *
                            tab[i].dat + 4);
                          printf("\t; 0x%08X",
                            (op & 0xff) *
                            tab[i].dat + 4 +
                            v_addr);
                        }
                      else
                        printf(tab[i].mnem,
                        (op & 0xff) *
                        tab[i].dat);
                      }
                    else
                      {
                        if (op & 0x80)  /* sign extend */
                          printf(tab[i].mnem,
                          (((op & 0xff) +
                          0xffffff00) * 2) +

                          v_addr + 4);
                        else
                          printf(tab[i].mnem,
                          ((op & 0xff) * 2) +
                          v_addr + 4);
                      }        
                  }
                else if (tab[i].format == D12_F)
                  {
                    if (op & 0x800)         /* sign extend */
                      printf(tab[i].mnem,
                      ((op & 0xfff) + 0xfffff000) * 2
                      + v_addr + 4);
                    else
                      printf(tab[i].mnem, (op & 0xfff) * 2 +
                      v_addr + 4);
                  }
                else if (tab[i].format == ND8_F)
                  {
                    int imm = (op & 0xff) * tab[i].dat + 4; 
                    if ((op & 0xf000) == 0x9000)    /* .W */
                      {
                        int dat =  (unsigned short) (*(imm + p_addr) << 8) | *(imm + p_addr + 1);
                        m_addr[imm+0] = ND8_F; /* this couldn't be an instruction so mark it ! */
                        m_addr[imm+1] = imm;
                        printf(tab[i].mnem,
                          imm,
                          (op >> 8) & 0xf);
                        printf("\t; 0x%08X (0x%04X)",
                          imm + v_addr, dat);
                      }
                    else                            /* .L */
                      {
                        unsigned char *b_addr = (unsigned char *)((int)p_addr & 0xfffffffc);
                        int dat =  (unsigned int) (*(imm + b_addr) << 24) | (*(imm + b_addr + 1) << 16)
                          | (*(imm + b_addr + 2) << 8) | *(imm + b_addr + 3) ;
			/* SH-1 register name lookup */
			char* str = "";
			if ( (dat & 0xfffffe00) == 0x05fffe00 )
			   str = regname[dat & 0x1ff];
                        m_addr[imm+(b_addr-p_addr)+0] = ND8_F; /* this couldn't be an instruction so mark it ! */
                        m_addr[imm+(b_addr-p_addr)+1] = imm;
                        m_addr[imm+(b_addr-p_addr)+2] = -1;
                        printf(tab[i].mnem,
                          imm,
                          (op >> 8) & 0xf);
                        printf("\t; 0x%08X (0x%08X) %s",
                          imm + (v_addr & 0xfffffffc), dat, str);
                      }
                  }
                else if (tab[i].format == I_F)
                  printf(tab[i].mnem, op & 0xff);
                else if (tab[i].format == NI_F)
                  printf(tab[i].mnem, op & 0xff, (op >> 8) &
                  0xf);
                else
                  printf("???");
                printf("\n");
                return;
              }
          }
     
        printf("???");

      }
    printf("\n");
  }

void ShowHelp()
{
        printf("sh2d Version %s by Bart Trzynadlowski: A Free SH-1/SH-2 "
               "Disassembler\n", VERSION);
        printf("Usage:     sh2d <file> [options]\n");
        printf("Options:   -?,-h        Show this help text\n");
        printf("           -s #         Start offset (hexadecimal)\n");
        printf("           -l #         Number of bytes (decimal)\n");
        printf("           -o #         Set origin (hexadecimal)\n");
        printf("           -sh1         SH-1 disassembly only\n");
        printf("           -sh2         SH-2 disassembly (default)\n");
        exit(0);
}

int main(int argc, char **argv)
{
        FILE            *fp;
        long            fsize, file, mode;
        unsigned        start, len, calc_len = 0, org, do_org, i, j = 0;
        char            *buffer;
        char            *mark;

        if (argc == 1)                                  /* show help */
                ShowHelp();
        if (FindOption("-?", 0, 0, 0, argc, argv) ||
            FindOption("-h", 0, 0, 0, argc, argv))
                ShowHelp();

        if (FindOption("-sh1", 0, 0, 0, argc, argv))
                mode = 1;       /* SH-1 mode */
        else
                mode = 0;       /* SH-2 mode */
        if (FindOption("-sh2", 0, 0, 0, argc, argv))
                mode = 0;       /* SH-2 mode */

        start = FindOption("-s", 1, 1, 0, argc, argv);
        org = FindOption("-o", 1, 1, 0, argc, argv);
        if (!(len = FindOption("-l", 1, 0, 0, argc, argv)))
        {
                if (FindOption("-l", 0, 0, 0, argc, argv))
                        return 0;       /* -l was actually specified w/ 0 */
                calc_len = 1;           /* no -l, calculate length */
        }

        if (FindOption("-o", 0, 0, 0, argc, argv))
                do_org = 1;     /* -o was actually 0 */
        else
                do_org = 0;     /* there was no -o, ignore the org variable */

        if (!(file = FindOption(NULL, 0, 0, 1, argc, argv)))
        {
                fprintf(stderr, "sh2d: No input file specified. Try "
                                "\"sh2d -h\" for usage instructions\n");
                exit(1);
        }

        if ((fp = fopen(argv[file], "rb")) == NULL)
        {
                fprintf(stderr, "sh2d: Failed to open file: %s\n",
                        argv[file]);
                exit(1);
        }
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
        if ((buffer = (char *) calloc(fsize * 2, sizeof(unsigned short)))
            == NULL)
        {                      
                fprintf(stderr, "sh2d: Not enough memory to load input "
                                "file: %s, %lu bytes\n", argv[file], fsize);
                exit(1);
        }
        fread(buffer, sizeof(unsigned char), fsize, fp);
        fclose(fp);

        if (calc_len)
                len = fsize - start;

        mark = buffer + fsize;

        for (i = start; i < (unsigned) fsize && j < len; i += 2)
        {
                if (do_org)
                {
                        SH2Disasm(org, (unsigned char*)&buffer[i], mode, &mark[i]);
                        org += 2;
                }
                else
                        SH2Disasm(i, (unsigned char *)&buffer[i], mode, &mark[i]);
                j += 2;
        }

        return 0;
}
