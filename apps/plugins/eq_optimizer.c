/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael Giacomelli
 *
 * EQ optimizer: finds an EQ with fewer bands that approximates the
 * magnitude response of the current EQ.
 *
 * Method:
 *  - The target is the summed dB response of the enabled EQ bands, sampled
 *    on a log-spaced frequency grid.
 *  - Backward elimination: starting from the current bands, repeatedly try
 *    removing each band, give each candidate a short Levenberg-Marquardt
 *    refit, then refit the best two fully and keep the better one.  This
 *    produces a fit for every band count from N-1 down to 1 in one run.
 *  - Each fit is rounded to the units Rockbox stores (1 Hz, 0.1 Q, 0.1 dB)
 *    and polished with an integer coordinate search.
 *
 * Everything is fixed point.  The EQ's filters are bilinear transforms of
 * analog prototypes, prewarped at the band frequency, so a band's response
 * depends on frequency only through v = (tan(w/2) / tan(w0/2))^(+-2) <= 1.
 * With l = 2*log2(tan(w/2)) precomputed per grid point, v = 2^-|l - l0|,
 * and each band's |H|^2 is a ratio of two quadratics in v whose
 * coefficients are bounded: one table-driven exp2 and two log2s per point.
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

#ifndef EQOPT_HOST_TEST
#include "plugin.h"
#include "lib/pluginlib_actions.h"
#endif

#include <stdint.h>

/* Number formats: QN means a signed or unsigned integer scaled by 2^N.
 * Frequencies, gains and Q are fitted as log2(fc), dB and log2(Q), all
 * Q24, and dB curves are Q24. */

#define NGRID         128
#define MAXB          EQ_NUM_BANDS
#define MAXP          (3 * MAXB)
#define ONE_Q24       (1 << 24)

/* Q24 constants for a 44.1 kHz grid */
#define L2_20        72509921   /* log2(20) */
#define L2_30        82323963   /* log2(30) */
#define L2_12000     227343806  /* log2(12000) */
#define L2_16000     234306980  /* log2(16000) */
#define L2_20000     239708037  /* log2(20000) */
#define L2_1000      167198116  /* log2(1000) */
#define L2_PI_FS     -231139619 /* log2(pi / 44100) */
#define L2_10        55732705   /* log2(10) */
#define L2_QMIN      -29141447  /* log2(0.3) */
#define L2_QMAX      44930591   /* log2(6.4) */
#define GAIN_MAX     (24 * ONE_Q24)

#define C_L2A_Q30    89172328   /* log2(10)/40: dB -> log2(A) */
#define K_DB_Q28     808071242  /* 10*log10(2): log2(|H|^2) -> dB */
#define LN2_Q31      1488522236u
#define INV_LN2_Q30  1549082005u

/* Penalty on band gain (0.15 per dB, compared with weighted dB residuals
 * at NGRID points).  Without it, low band counts drift into pairs of very
 * broad, very large peaks that cancel each other. */
#define REG_Q16      9830

/* 2^(i/256), Q31 */
static const uint32_t exp2_tab[256] =
{
    0x80000000, 0x8058d7d3, 0x80b1ed50, 0x810b40a2, 0x8164d1f4, 0x81bea171,
    0x8218af43, 0x8272fb98, 0x82cd8699, 0x83285072, 0x8383594f, 0x83dea15c,
    0x843a28c4, 0x8495efb3, 0x84f1f656, 0x854e3cd9, 0x85aac368, 0x86078a2f,
    0x8664915c, 0x86c1d91a, 0x871f6197, 0x877d2aff, 0x87db3580, 0x88398147,
    0x88980e81, 0x88f6dd5b, 0x8955ee03, 0x89b540a8, 0x8a14d575, 0x8a74ac9a,
    0x8ad4c645, 0x8b3522a4, 0x8b95c1e4, 0x8bf6a435, 0x8c57c9c4, 0x8cb932c2,
    0x8d1adf5b, 0x8d7ccfc1, 0x8ddf0420, 0x8e417ca9, 0x8ea4398b, 0x8f073af6,
    0x8f6a8118, 0x8fce0c22, 0x9031dc43, 0x9095f1ac, 0x90fa4c8c, 0x915eed14,
    0x91c3d374, 0x9228ffdc, 0x928e727e, 0x92f42b89, 0x935a2b2f, 0x93c071a1,
    0x9426ff10, 0x948dd3ad, 0x94f4efa9, 0x955c5337, 0x95c3fe87, 0x962bf1cc,
    0x96942d37, 0x96fcb0fb, 0x97657d4a, 0x97ce9256, 0x9837f052, 0x98a1976f,
    0x990b87e2, 0x9975c1dd, 0x99e04593, 0x9a4b1337, 0x9ab62afd, 0x9b218d17,
    0x9b8d39ba, 0x9bf93119, 0x9c657368, 0x9cd200dc, 0x9d3ed9a7, 0x9dabfdff,
    0x9e196e19, 0x9e872a27, 0x9ef53261, 0x9f6386f9, 0x9fd22825, 0xa041161b,
    0xa0b05110, 0xa11fd938, 0xa18faecb, 0xa1ffd1fc, 0xa2704303, 0xa2e10215,
    0xa3520f69, 0xa3c36b34, 0xa43515ae, 0xa4a70f0d, 0xa5195787, 0xa58bef53,
    0xa5fed6aa, 0xa6720dc1, 0xa6e594d0, 0xa7596c0f, 0xa7cd93b5, 0xa8420bfa,
    0xa8b6d516, 0xa92bef42, 0xa9a15ab5, 0xaa1717a8, 0xaa8d2653, 0xab0386ef,
    0xab7a39b6, 0xabf13edf, 0xac6896a5, 0xace04140, 0xad583eea, 0xadd08fdd,
    0xae493453, 0xaec22c85, 0xaf3b78ad, 0xafb51907, 0xb02f0dcc, 0xb0a95736,
    0xb123f582, 0xb19ee8e9, 0xb21a31a6, 0xb295cff6, 0xb311c413, 0xb38e0e38,
    0xb40aaea2, 0xb487a58d, 0xb504f334, 0xb58297d4, 0xb60093a8, 0xb67ee6ef,
    0xb6fd91e3, 0xb77c94c3, 0xb7fbefcb, 0xb87ba338, 0xb8fbaf47, 0xb97c1437,
    0xb9fcd245, 0xba7de9af, 0xbaff5ab2, 0xbb81258d, 0xbc034a7f, 0xbc85c9c5,
    0xbd08a39f, 0xbd8bd84c, 0xbe0f680a, 0xbe935318, 0xbf1799b6, 0xbf9c3c25,
    0xc0213aa2, 0xc0a6956f, 0xc12c4cca, 0xc1b260f6, 0xc238d231, 0xc2bfa0bd,
    0xc346ccda, 0xc3ce56ca, 0xc4563ecc, 0xc4de8524, 0xc5672a11, 0xc5f02dd7,
    0xc67990b6, 0xc70352f0, 0xc78d74c9, 0xc817f681, 0xc8a2d85d, 0xc92e1a9d,
    0xc9b9bd86, 0xca45c15b, 0xcad2265e, 0xcb5eecd4, 0xcbec14ff, 0xcc799f24,
    0xcd078b86, 0xcd95da6b, 0xce248c15, 0xceb3a0ca, 0xcf4318cf, 0xcfd2f468,
    0xd06333db, 0xd0f3d76c, 0xd184df62, 0xd2164c02, 0xd2a81d92, 0xd33a5458,
    0xd3ccf09a, 0xd45ff29e, 0xd4f35aac, 0xd587290a, 0xd61b5dff, 0xd6aff9d2,
    0xd744fccb, 0xd7da6731, 0xd870394c, 0xd9067365, 0xd99d15c2, 0xda3420ae,
    0xdacb946f, 0xdb637150, 0xdbfbb798, 0xdc946791, 0xdd2d8185, 0xddc705bd,
    0xde60f482, 0xdefb4e20, 0xdf9612df, 0xe031430a, 0xe0ccdeec, 0xe168e6d0,
    0xe2055b00, 0xe2a23bc8, 0xe33f8973, 0xe3dd444c, 0xe47b6ca0, 0xe51a02bb,
    0xe5b906e7, 0xe6587973, 0xe6f85aab, 0xe798aadb, 0xe8396a50, 0xe8da9958,
    0xe97c3840, 0xea1e4756, 0xeac0c6e8, 0xeb63b743, 0xec0718b6, 0xecaaeb90,
    0xed4f301f, 0xedf3e6b2, 0xee990f98, 0xef3eab21, 0xefe4b99c, 0xf08b3b59,
    0xf13230a8, 0xf1d999d9, 0xf281773c, 0xf329c923, 0xf3d28fde, 0xf47bcbbe,
    0xf5257d15, 0xf5cfa434, 0xf67a416c, 0xf7255511, 0xf7d0df73, 0xf87ce0e6,
    0xf92959bb, 0xf9d64a47, 0xfa83b2db, 0xfb3193cc, 0xfbdfed6d, 0xfc8ec011,
    0xfd3e0c0d, 0xfdedd1b5, 0xfe9e115c, 0xff4ecb59,
};
/* log2(1 + i/256), Q24 */
static const uint32_t log2_tab[256] =
{
    0x00000000, 0x0001709c, 0x0002dfca, 0x00044d8c, 0x0005b9e6, 0x000724d9,
    0x00088e69, 0x0009f698, 0x000b5d6a, 0x000cc2e0, 0x000e26fd, 0x000f89c5,
    0x0010eb39, 0x00124b5b, 0x0013aa30, 0x001507b8, 0x001663f7, 0x0017beef,
    0x001918a1, 0x001a7112, 0x001bc842, 0x001d1e35, 0x001e72ec, 0x001fc66a,
    0x002118b1, 0x002269c3, 0x0023b9a3, 0x00250853, 0x002655d4, 0x0027a229,
    0x0028ed54, 0x002a3757, 0x002b8034, 0x002cc7ee, 0x002e0e86, 0x002f53fe,
    0x00309858, 0x0031db96, 0x00331dba, 0x00345ec6, 0x00359ebc, 0x0036dd9e,
    0x00381b6e, 0x0039582c, 0x003a93dd, 0x003bce80, 0x003d0818, 0x003e40a6,
    0x003f782d, 0x0040aeaf, 0x0041e42b, 0x004318a6, 0x00444c1f, 0x00457e9a,
    0x0046b017, 0x0047e098, 0x0049101f, 0x004a3ead, 0x004b6c44, 0x004c98e6,
    0x004dc493, 0x004eef4f, 0x00501919, 0x005141f4, 0x005269e1, 0x005390e2,
    0x0054b6f8, 0x0055dc24, 0x00570069, 0x005823c7, 0x00594640, 0x005a67d5,
    0x005b8887, 0x005ca859, 0x005dc74b, 0x005ee55f, 0x00600296, 0x00611ef1,
    0x00623a72, 0x0063551a, 0x00646eea, 0x006587e4, 0x0066a009, 0x0067b75a,
    0x0068cdd8, 0x0069e385, 0x006af862, 0x006c0c70, 0x006d1fb0, 0x006e3223,
    0x006f43cc, 0x007054aa, 0x007164bf, 0x0072740c, 0x00738292, 0x00749053,
    0x00759d50, 0x0076a989, 0x0077b4ff, 0x0078bfb5, 0x0079c9ab, 0x007ad2e1,
    0x007bdb5a, 0x007ce316, 0x007dea16, 0x007ef05b, 0x007ff5e6, 0x0080fab9,
    0x0081fed4, 0x00830239, 0x008404e8, 0x008506e2, 0x00860828, 0x008708bc,
    0x0088089e, 0x008907cf, 0x008a0650, 0x008b0422, 0x008c0146, 0x008cfdbe,
    0x008df989, 0x008ef4a9, 0x008fef1f, 0x0090e8eb, 0x0091e20f, 0x0092da8b,
    0x0093d260, 0x0094c990, 0x0095c01a, 0x0096b601, 0x0097ab44, 0x00989fe4,
    0x009993e3, 0x009a8742, 0x009b7a00, 0x009c6c1f, 0x009d5da0, 0x009e4e83,
    0x009f3eca, 0x00a02e74, 0x00a11d84, 0x00a20bf9, 0x00a2f9d5, 0x00a3e718,
    0x00a4d3c2, 0x00a5bfd6, 0x00a6ab53, 0x00a7963a, 0x00a8808c, 0x00a96a4a,
    0x00aa5374, 0x00ab3c0c, 0x00ac2411, 0x00ad0b85, 0x00adf268, 0x00aed8bc,
    0x00afbe80, 0x00b0a3b5, 0x00b1885c, 0x00b26c77, 0x00b35004, 0x00b43306,
    0x00b5157d, 0x00b5f769, 0x00b6d8cb, 0x00b7b9a4, 0x00b899f5, 0x00b979bd,
    0x00ba58ff, 0x00bb37b9, 0x00bc15ee, 0x00bcf39d, 0x00bdd0c8, 0x00bead6e,
    0x00bf8991, 0x00c06531, 0x00c1404f, 0x00c21aeb, 0x00c2f506, 0x00c3cea0,
    0x00c4a7ba, 0x00c58055, 0x00c65872, 0x00c73010, 0x00c80731, 0x00c8ddd4,
    0x00c9b3fb, 0x00ca89a7, 0x00cb5ed7, 0x00cc338c, 0x00cd07c7, 0x00cddb88,
    0x00ceaed0, 0x00cf819f, 0x00d053f7, 0x00d125d7, 0x00d1f740, 0x00d2c832,
    0x00d398af, 0x00d468b6, 0x00d53848, 0x00d60765, 0x00d6d60f, 0x00d7a446,
    0x00d87209, 0x00d93f5a, 0x00da0c3a, 0x00dad8a8, 0x00dba4a4, 0x00dc7031,
    0x00dd3b4e, 0x00de05fb, 0x00ded039, 0x00df9a09, 0x00e0636a, 0x00e12c5e,
    0x00e1f4e5, 0x00e2bcff, 0x00e384ad, 0x00e44bf0, 0x00e512c7, 0x00e5d933,
    0x00e69f35, 0x00e764cd, 0x00e829fb, 0x00e8eec1, 0x00e9b31e, 0x00ea7712,
    0x00eb3a9f, 0x00ebfdc5, 0x00ecc083, 0x00ed82db, 0x00ee44cd, 0x00ef065a,
    0x00efc781, 0x00f08843, 0x00f148a1, 0x00f2089b, 0x00f2c832, 0x00f38765,
    0x00f44636, 0x00f504a4, 0x00f5c2b0, 0x00f6805a, 0x00f73da4, 0x00f7fa8c,
    0x00f8b714, 0x00f9733c, 0x00fa2f04, 0x00faea6d, 0x00fba578, 0x00fc6023,
    0x00fd1a71, 0x00fdd460, 0x00fe8df2, 0x00ff4728,
};
/* 1 / (1 + i/256), Q32; [0] saturated */
static uint32_t rcp_tab[256];

/* Continuous band used during fitting: log2(fc), gain in dB, log2(Q) */
struct band
{
    int slot;   /* EQ band index this came from */
    int type;   /* enum eq_filter_type */
    int32_t p[3];
};

/* Quantised band in Rockbox setting units */
struct qband
{
    int slot, type;
    int fc;     /* Hz */
    int q;      /* tenths */
    int g;      /* tenths of a dB */
};

struct fit
{
    int n;
    struct qband b[MAXB];
    int rms, maxerr;  /* hundredths of a dB, unweighted over 20 Hz - 16 kHz */
    int32_t peak;     /* highest point of the curve, Q24 dB */
};

/* Per-band constants for evaluating a response */
struct bcoef
{
    int type;
    int32_t l0;      /* 2*log2(tan(w0/2)), Q24 */
    int32_t off;     /* constant dB term, Q24 */
    int32_t A;       /* shelves: A = 10^(gain/40), Q28 */
    uint32_t am, dm; /* peak: A^2/Q^2 and 1/(A^2 Q^2); shelf: A/Q^2 in am.
                        Q31 mantissas in [1, 2) ... */
    int ash, dsh;    /* ... and the right shifts taking mantissa * v (Q62)
                        to Q56 */
};

/** Fixed-point math **/

static void init_tables(void)
{
    rcp_tab[0] = 0xffffffffu;
    for (int i = 1; i < 256; i++)
        rcp_tab[i] = (uint32_t)((1ULL << 40) / (256 + i));
}

/* count leading zeros of x != 0.  ARMv4 has no CLZ instruction, and
 * libgcc's version is an out-of-line call. */
static inline __attribute__((always_inline)) int clz32(uint32_t x)
{
#if !defined(__arm__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 5)
    return __builtin_clz(x);
#else
    int n = 0;
    if (x < 0x10000)    { n = 16; x <<= 16; }
    if (x < 0x1000000)  { n += 8; x <<= 8; }
    if (x < 0x10000000) { n += 4; x <<= 4; }
    if (x < 0x40000000) { n += 2; x <<= 2; }
    if (x < 0x80000000) n += 1;
    return n;
#endif
}

static inline int clz64(uint64_t x)
{
    uint32_t hi = (uint32_t)(x >> 32);
    return hi ? clz32(hi) : 32 + clz32((uint32_t)x);
}

/* log2(x) for x > 0, Q24 */
static int32_t log2_u64(uint64_t x)
{
    if (x == 0)
        return -(64 << 24);
    int lz = clz64(x);
    uint32_t m = (uint32_t)((x << lz) >> 32);       /* [1, 2), Q31 */
    unsigned idx = (m >> 23) & 0xff;
    /* r = m / (1 + idx/256) - 1 < 2^-8, then log2(1 + r) ~ (r - r^2/2)/ln2 */
    uint32_t r = (uint32_t)(((uint64_t)(m & 0x7fffff) * rcp_tab[idx]) >> 32);
    uint32_t u = r - (uint32_t)(((uint64_t)r * r) >> 32);
    int32_t frac = (int32_t)(((uint64_t)u * INV_LN2_Q30) >> 37);
    return ((63 - lz) << 24) + (int32_t)log2_tab[idx] + frac;
}

/* 2^(f / 2^24) for 0 <= f < 2^24: Q31, in [1, 2) */
static uint32_t exp2_frac(uint32_t f)
{
    uint32_t rem = f & 0xffff;
    /* 2^x ~ 1 + x ln2 + (x ln2)^2 / 2 for x < 2^-8 */
    uint32_t t = (uint32_t)(((uint64_t)rem * LN2_Q31) >> 24);
    uint32_t e = 0x80000000u + t + (uint32_t)(((uint64_t)t * t) >> 32);
    return (uint32_t)(((uint64_t)exp2_tab[f >> 16] * e) >> 31);
}

/* 2^x for Q24 x, as a Q31 mantissa in [1, 2) and an exponent */
static uint32_t exp2_me(int32_t x, int *ex)
{
    *ex = x >> 24;  /* floor */
    return exp2_frac((uint32_t)x & 0xffffff);
}

/* 2^-d for Q24 d >= 0, Q31 */
static uint32_t exp2_neg(uint32_t d)
{
    uint32_t n = d >> 24, f = d & 0xffffff;
    if (f == 0)
        return n >= 32 ? 0 : 0x80000000u >> n;
    if (n >= 31)
        return 0;
    return exp2_frac(ONE_Q24 - f) >> (n + 1);
}

/* round(2^x) for Q24 x in [0, 30) */
static int exp2_round(int32_t x)
{
    int ex;
    uint32_t m = exp2_me(x, &ex);
    return (int)(((uint64_t)m + (1ULL << (30 - ex))) >> (31 - ex));
}

static uint32_t isqrt64(uint64_t x)
{
    uint64_t r = 0, b = 1ULL << 62;
    while (b > x)
        b >>= 2;
    while (b)
    {
        if (x >= r + b)
        {
            x -= r + b;
            r = (r >> 1) + b;
        }
        else
            r >>= 1;
        b >>= 2;
    }
    return (uint32_t)r;
}

/* round(x / 2^24) */
static int round_q24(int64_t x)
{
    return x >= 0 ? (int)((x + (ONE_Q24 / 2)) >> 24)
                  : -(int)((-x + (ONE_Q24 / 2)) >> 24);
}

/* z * l / 2^30 for Q30 l, without overflowing for |z| < 2^61 */
static int64_t mul_q30(int64_t z, int32_t l)
{
    int32_t hi = (int32_t)(z >> 30);
    uint32_t lo = (uint32_t)z & 0x3fffffff;
    return (int64_t)hi * l + (((int64_t)lo * l) >> 30);
}

/* (z * m) >> s for |z| < 2^62, 0 <= s < 64, without a 128-bit product */
static int64_t mul_shr(int64_t z, uint32_t m, int s)
{
    int64_t hi = (int64_t)(int32_t)(z >> 32) * m;
    uint64_t lo = (uint64_t)(uint32_t)z * m;
    if (s >= 32)
        return (hi >> (s - 32)) + (int64_t)(lo >> s);
    return hi * ((int64_t)1 << (32 - s)) + (int64_t)(lo >> s);
}

/* Reciprocal of d > 0 for use with mul_shr: z / d = mul_shr(z, inv, sh) / 2^k
 * where 2^62 / (d << n) = inv and the returned shift is 62 - n - k.  Here
 * k is chosen by the caller, so return n. */
static uint32_t recip(uint32_t d, int *n)
{
    *n = clz32(d);
    return (uint32_t)((1ULL << 62) / (d << *n));
}

static int64_t clamp64(int64_t x, int64_t lim)
{
    return x > lim ? lim : (x < -lim ? -lim : x);
}

/** Filter response **/

/* 2*log2(tan(pi f / fs)) for p = log2(f), Q24 */
static int32_t tan_l(int32_t p)
{
    /* Horner coefficients, Q30: sin(t)/t and cos(t) in powers of t^2 */
    static const int32_t sinc_c[6] =
        { 1073741824, -178956971, 8947849, -213044, 2959, -27 };
    static const int32_t cos_c[7] =
        { 1073741824, -536870912, 44739243, -1491308, 26631, -296, 2 };
    int32_t lt = p + L2_PI_FS;                  /* log2(theta) */
    int ex;
    uint32_t m = exp2_me(lt, &ex);
    uint32_t th = m >> (1 - ex);                /* theta < 1.43, Q30 */
    uint32_t th2 = (uint32_t)(((uint64_t)th * th) >> 30);
    int32_t s = sinc_c[5], c = cos_c[6];
    for (int k = 4; k >= 0; k--)
        s = sinc_c[k] + (int32_t)(((int64_t)s * th2) >> 30);
    for (int k = 5; k >= 0; k--)
        c = cos_c[k] + (int32_t)(((int64_t)c * th2) >> 30);
    /* log2(tan) = log2(theta) + log2(sin(theta)/theta) - log2(cos(theta)) */
    return 2 * (lt + log2_u64((uint32_t)s) - log2_u64((uint32_t)c));
}

/* m * v (Q31 * Q31) shifted to Q56 */
static inline uint64_t mv_q56(uint32_t m, uint32_t v, int sh)
{
    uint64_t p = (uint64_t)m * v;
    return sh >= 0 ? p >> sh : p << 1;
}

static int coef_shift(int ex)
{
    int sh = 6 - ex;
    return sh > 63 ? 63 : sh;
}

static void band_coef(int type, const int32_t p[3], struct bcoef *c)
{
    int32_t l2a = (int32_t)(((int64_t)p[1] * C_L2A_Q30) >> 30); /* log2 A */
    int32_t l2q2 = 2 * p[2];                                     /* log2 Q^2 */
    int ex;

    c->type = type;
    c->l0 = tan_l(p[0]);
    if (type == EQ_FILTER_LOW_SHELF || type == EQ_FILTER_HIGH_SHELF)
    {
        c->am = exp2_me(l2a - l2q2, &ex);
        c->ash = coef_shift(ex);
        uint32_t a = exp2_me(l2a, &ex);         /* ex in [-2, 1] */
        c->A = (int32_t)(a >> (3 - ex));
        c->off = p[1] / 2;
    }
    else
    {
        c->am = exp2_me(2 * l2a - l2q2, &ex);
        c->ash = coef_shift(ex);
        c->dm = exp2_me(-2 * l2a - l2q2, &ex);
        c->dsh = coef_shift(ex);
        c->off = 0;
    }
}

/* Response in dB (Q24) at n points given by their tan_l() values.
 *
 * Analog prototypes, with u = (tan(w/2)/tan(w0/2))^2:
 *   peak:  |H|^2 = ((1-u)^2 + u A^2/Q^2) / ((1-u)^2 + u/(A^2 Q^2))
 *   low shelf: A^2 ((A-u)^2 + u A/Q^2) / ((1-Au)^2 + u A/Q^2)
 *   high shelf: A^2 ((1-Au)^2 + u A/Q^2) / ((A-u)^2 + u A/Q^2)
 * Above w0, dividing through by u^2 gives the same forms in v = 1/u (the
 * shelves swap numerator and denominator), so v is always <= 1. */
static void coef_curve(const struct bcoef *c, const int32_t *l, int n,
                       int32_t *out)
{
    for (int i = 0; i < n; i++)
    {
        int32_t dl = l[i] - c->l0;
        uint32_t v = exp2_neg(dl < 0 ? -dl : dl);    /* Q31 */
        uint64_t num, den;

        if (c->type == EQ_FILTER_LOW_SHELF || c->type == EQ_FILTER_HIGH_SHELF)
        {
            int32_t amv = c->A - (int32_t)(v >> 3);                /* Q28 */
            int32_t omav = (1 << 28) - (int32_t)(((int64_t)c->A * v) >> 31);
            uint64_t x = (uint64_t)((int64_t)amv * amv);           /* Q56 */
            uint64_t y = (uint64_t)((int64_t)omav * omav);
            uint64_t sv = mv_q56(c->am, v, c->ash);
            int below = dl < 0;
            if (c->type == EQ_FILTER_HIGH_SHELF)
                below = !below;
            num = (below ? x : y) + sv;
            den = (below ? y : x) + sv;
        }
        else
        {
            uint32_t om = 0x80000000u - v;
            uint64_t t = ((uint64_t)om * om) >> 6;                 /* Q56 */
            num = t + mv_q56(c->am, v, c->ash);
            den = t + mv_q56(c->dm, v, c->dsh);
        }

        int32_t d = log2_u64(num) - log2_u64(den);
        out[i] = c->off + (int32_t)(((int64_t)d * K_DB_Q28) >> 28);
    }
}

static void band_curve(const struct band *b, const int32_t *l, int n,
                       int32_t *out)
{
    struct bcoef c;
    band_coef(b->type, b->p, &c);
    coef_curve(&c, l, n, out);
}

static void band_from_q(const struct qband *q, struct band *b)
{
    b->slot = q->slot;
    b->type = q->type;
    b->p[0] = log2_u64(q->fc > 0 ? q->fc : 1);
    b->p[1] = (int32_t)(((int64_t)q->g * ONE_Q24) / 10);
    b->p[2] = log2_u64(q->q > 0 ? q->q : 1) - L2_10;
}

static void qband_curve(const struct qband *q, const int32_t *l, int n,
                        int32_t *out)
{
    struct band b;
    band_from_q(q, &b);
    band_curve(&b, l, n, out);
}

/** Optimiser state **/

static int32_t grid_p[NGRID], grid_l[NGRID];
static int32_t grid_w[NGRID];        /* weights, Q15 */
static int32_t target[NGRID];
static int32_t curves[MAXB][NGRID];  /* per-band curves of current fit */
static int32_t tcurves[MAXB][NGRID]; /* trial curves */
static int32_t resid[NGRID];         /* weighted residuals, Q24 */
static int32_t jac[MAXP][NGRID];     /* weighted Jacobian, Q16, then
                                        normalised columns, Q30 */
static int32_t tmpc[NGRID];

static struct qband orig[MAXB];      /* enabled bands of the user's EQ */
static int norig;
static struct fit fits[MAXB + 1];    /* fits[n] for n = 1 .. norig */

static void (*progress_cb)(int done, int total);
static int progress_done, progress_total;

static void bump_progress(void)
{
    progress_done++;
    if (progress_cb)
        progress_cb(progress_done, progress_total);
}

static void setup_grid(void)
{
    for (int i = 0; i < NGRID; i++)
    {
        int32_t p = L2_20 + (int32_t)(((int64_t)L2_1000 * i) / (NGRID - 1));
        grid_p[i] = p;
        grid_l[i] = tan_l(p);
        /* de-emphasise the extremes, where narrow details matter least */
        grid_w[i] = p < L2_30 ? 16384 : (p > L2_12000 ? 13107 : 32768);
    }
}

/* Sum of n curves at point i, saturated to the Q24 range */
static int32_t sum_at(int32_t (*cv)[NGRID], int n, int i)
{
    int64_t t = 0;
    for (int b = 0; b < n; b++)
        t += cv[b][i];
    return (int32_t)clamp64(t, INT32_MAX);
}

static void compute_target(void)
{
    for (int b = 0; b < norig; b++)
        qband_curve(&orig[b], grid_l, NGRID, curves[b]);
    for (int i = 0; i < NGRID; i++)
        target[i] = sum_at(curves, norig, i);
}

/* Weighted residuals (Q24) and their sum of squares (Q32) */
static uint64_t eval_cost(int32_t (*cv)[NGRID], int nb, int32_t *res)
{
    uint64_t cost = 0;
    for (int i = 0; i < NGRID; i++)
    {
        int64_t t = (int64_t)sum_at(cv, nb, i) - target[i];
        int32_t r = (int32_t)clamp64((t * grid_w[i]) >> 15, INT32_MAX);
        if (res)
            res[i] = r;
        int32_t s = r >> 8;
        cost += (uint64_t)((int64_t)s * s);
    }
    return cost;
}

static uint64_t gain_pen(int32_t gain)
{
    int32_t s = (int32_t)(((int64_t)REG_Q16 * gain) >> 24);       /* Q16 */
    return (uint64_t)((int64_t)s * s);
}

static uint64_t gain_penalty(const struct band *bs, int nb)
{
    uint64_t s = 0;
    for (int b = 0; b < nb; b++)
        s += gain_pen(bs[b].p[1]);
    return s;
}

static void clamp_band(struct band *b)
{
    static const int32_t lo[3] = { L2_20, -GAIN_MAX, L2_QMIN };
    static const int32_t hi[3] = { L2_20000, GAIN_MAX, L2_QMAX };
    for (int j = 0; j < 3; j++)
    {
        if (b->p[j] < lo[j]) b->p[j] = lo[j];
        if (b->p[j] > hi[j]) b->p[j] = hi[j];
    }
}

/* Solve M y = c for symmetric positive definite M with unit diagonal and
 * |entries| <= 1, Q30 (row stride MAXP).  c and y are Q24.  Returns 0 if a
 * pivot is too small. */
static int cholesky_solve(int32_t (*M)[MAXP], const int64_t *c, int64_t *y,
                          int n)
{
    static int32_t L[MAXP][MAXP];
    static uint32_t inv[MAXP];
    static int ish[MAXP];
    const int64_t lim = 1LL << 52;

    for (int j = 0; j < n; j++)
    {
        int64_t s = (int64_t)M[j][j] << 30;                         /* Q60 */
        for (int k = 0; k < j; k++)
            s -= (int64_t)L[j][k] * L[j][k];
        if (s < (1LL << 40))   /* pivot below 2^-20 */
            return 0;
        int32_t ljj = (int32_t)isqrt64((uint64_t)s);                 /* Q30 */
        L[j][j] = ljj;
        /* x * 2^30 / ljj = mul_shr(x, inv, 32 - n) */
        inv[j] = recip((uint32_t)ljj, &ish[j]);
        ish[j] = 32 - ish[j];
        for (int i = j + 1; i < n; i++)
        {
            int64_t t = (int64_t)M[i][j] << 30;
            for (int k = 0; k < j; k++)
                t -= (int64_t)L[i][k] * L[j][k];
            t = mul_shr(clamp64(t >> 30, INT32_MAX), inv[j], ish[j]);
            L[i][j] = (int32_t)clamp64(t, 1 << 30);
        }
    }

    for (int i = 0; i < n; i++)
    {
        int64_t s = c[i];
        for (int k = 0; k < i; k++)
            s -= mul_q30(y[k], L[i][k]);
        y[i] = clamp64(mul_shr(clamp64(s, lim), inv[i], ish[i]), lim);
    }
    for (int i = n - 1; i >= 0; i--)
    {
        int64_t s = y[i];
        for (int k = i + 1; k < n; k++)
            s -= mul_q30(y[k], L[k][i]);
        y[i] = clamp64(mul_shr(clamp64(s, lim), inv[i], ish[i]), lim);
    }
    return 1;
}

static void yield_cpu(void)
{
#ifndef EQOPT_HOST_TEST
    rb->yield();
#endif
}

/* 2^30 / (1 + 2^lam) */
static int32_t damping_mu(int lam)
{
    if (lam >= 0)
        return (int32_t)((1ULL << 60) / ((1ULL << 30) + (1ULL << (30 + lam))));
    return (int32_t)((1ULL << (30 - lam)) / ((1ULL << -lam) + 1));
}

/* Levenberg-Marquardt refit of bs[0..nb-1] in place; returns the cost.
 *
 * With column norms D (penalty included), the scaled system
 * (B + lambda I) y = c has B = D^-1 (J'J + P) D^-1 with a unit diagonal and
 * y = D delta.  Dividing by 1 + lambda keeps every entry within +-1:
 * (mu B_offdiag + I) y = mu c, mu = 1 / (1 + lambda). */
static uint64_t lm_fit(struct band *bs, int nb, int maxit)
{
    static int32_t B[MAXP][MAXP], M[MAXP][MAXP];
    static int16_t jh[MAXP][NGRID];     /* normalised columns, Q14 */
    static uint32_t dinv[MAXP];         /* reciprocals of column norms */
    static int dsh[MAXP];
    static int64_t cv[MAXP], rhs[MAXP], y[MAXP];
    static const int hs[3] = { 10, 7, 10 };   /* steps 2^-hs */
    static const int32_t maxstep[3] =
        { 2 * ONE_Q24, 12 * ONE_Q24, 2 * ONE_Q24 };
    struct band trial[MAXB];
    int np = 3 * nb;
    int lam = -7;

    for (int b = 0; b < nb; b++)
        band_curve(&bs[b], grid_l, NGRID, curves[b]);
    uint64_t cost = eval_cost(curves, nb, resid) + gain_penalty(bs, nb);

    for (int it = 0; it < maxit; it++)
    {
        /* forward-difference Jacobian: only the perturbed band changes */
        for (int b = 0; b < nb; b++)
        {
            for (int j = 0; j < 3; j++)
            {
                struct band t = bs[b];
                t.p[j] += 1 << (24 - hs[j]);
                band_curve(&t, grid_l, NGRID, tmpc);
                int32_t *col = jac[3 * b + j];
                int sh = hs[j] - 8;
                for (int i = 0; i < NGRID; i++)
                {
                    int64_t d = (int64_t)(tmpc[i] - curves[b][i]);
                    d = sh >= 0 ? d * (1 << sh) : d >> -sh;
                    col[i] = (int32_t)clamp64((d * grid_w[i]) >> 15,
                                              INT32_MAX);
                }
            }
        }

        /* normalise the columns; build the gradient */
        for (int r = 0; r < np; r++)
        {
            int32_t *col = jac[r];
            uint64_t s = 0;
            for (int i = 0; i < NGRID; i++)
                s += (uint64_t)((int64_t)col[i] * col[i]);
            if (r % 3 == 1)
                s += (uint64_t)REG_Q16 * REG_Q16;
            uint32_t d = isqrt64(s);
            if (d == 0)
                d = 1;
            int sh;
            uint32_t inv = recip(d, &sh);
            dinv[r] = inv;
            dsh[r] = sh;

            /* col * 2^30 / d (Q30) for the gradient, and a Q14 copy for
             * the Hessian estimate */
            int64_t g = 0;
            for (int i = 0; i < NGRID; i++)
            {
                col[i] = (int32_t)(((int64_t)col[i] * inv) >> (32 - sh));
                g += (int64_t)col[i] * (resid[i] >> 4);             /* Q50 */
                jh[r][i] = (int16_t)(col[i] >> 16);
            }
            g >>= 26;                                               /* Q24 */
            if (r % 3 == 1)
            {
                /* penalty gradient REG^2 gain / d */
                int64_t pg = (int64_t)REG_Q16 * REG_Q16 * bs[r / 3].p[1];
                g += mul_shr(pg, inv, 62 - sh) >> 16;
            }
            cv[r] = -g;
        }

        for (int r = 0; r < np; r++)
        {
            /* only the step depends on B, not where the fit converges, so
             * Q14 is plenty; |B| <= 1 keeps the 32-bit sum in range */
            for (int c = 0; c < r; c++)
            {
                int32_t s = 0;
                for (int i = 0; i < NGRID; i++)
                    s += jh[r][i] * jh[c][i];
                B[r][c] = B[c][r] = s * 4;
            }
        }

        uint64_t old = cost;
        int improved = 0;
        for (int tries = 0; tries < 10 && !improved; tries++)
        {
            int32_t mu = damping_mu(lam);
            for (int r = 0; r < np; r++)
            {
                for (int c = 0; c < np; c++)
                    M[r][c] = (int32_t)(((int64_t)B[r][c] * mu) >> 30);
                M[r][r] = 1 << 30;
                rhs[r] = mul_q30(cv[r], mu);
            }

            if (!cholesky_solve(M, rhs, y, np))
            {
                lam = lam + 3 > 30 ? 30 : lam + 3;
                continue;
            }

            for (int b = 0; b < nb; b++)
            {
                trial[b] = bs[b];
                for (int j = 0; j < 3; j++)
                {
                    int r = 3 * b + j;
                    /* y * 2^16 / d */
                    int64_t dlt = mul_shr(clamp64(y[r], 1LL << 46), dinv[r],
                                          46 - dsh[r]);
                    trial[b].p[j] += (int32_t)clamp64(dlt, maxstep[j]);
                }
                clamp_band(&trial[b]);
                band_curve(&trial[b], grid_l, NGRID, tcurves[b]);
            }

            uint64_t tc = eval_cost(tcurves, nb, NULL) +
                          gain_penalty(trial, nb);
            if (tc < cost)
            {
                for (int b = 0; b < nb; b++)
                {
                    bs[b] = trial[b];
                    for (int i = 0; i < NGRID; i++)
                        curves[b][i] = tcurves[b][i];
                }
                cost = eval_cost(curves, nb, resid) + gain_penalty(bs, nb);
                lam = lam - 2 < -24 ? -24 : lam - 2;
                improved = 1;
            }
            else
            {
                lam = lam + 2 > 30 ? 30 : lam + 2;
            }
        }

        yield_cpu();

        /* stop when an iteration gains less than 2^-13 of the cost */
        if (!improved || old - cost < (old >> 13) + 1)
            break;
    }
    return cost;
}

/** Quantisation and integer polish **/

static void quantise(const struct band *b, struct qband *q)
{
    q->slot = b->slot;
    q->type = b->type;
    q->fc = exp2_round(b->p[0]);
    q->q = exp2_round(b->p[2] + L2_10);
    q->g = round_q24((int64_t)b->p[1] * 10);
    if (q->q < 1)
        q->q = 1;
    if (q->q > 64)
        q->q = 64;
    if (q->g == 0) /* zero gain would disable the band */
        q->g = b->p[1] < 0 ? -1 : 1;
}

static int qband_valid(const struct qband *q)
{
    return q->fc >= 20 && q->fc <= 20000 && q->q >= 1 && q->q <= 64 &&
           q->g != 0 && q->g >= -240 && q->g <= 240;
}

static uint64_t qband_pen(const struct qband *q)
{
    return gain_pen((int32_t)(((int64_t)q->g * ONE_Q24) / 10));
}

static void finish_fit(struct fit *f, int nb)
{
    uint64_t ss = 0;
    int cnt = 0;
    int32_t mx = 0, pk = INT32_MIN;

    f->n = nb;
    for (int b = 0; b < nb; b++)
        qband_curve(&f->b[b], grid_l, NGRID, curves[b]);

    for (int i = 0; i < NGRID; i++)
    {
        int32_t t = sum_at(curves, nb, i);
        if (t > pk)
            pk = t;
        if (grid_p[i] <= L2_16000)
        {
            int32_t e = t - target[i];
            int32_t s = e >> 8;
            ss += (uint64_t)((int64_t)s * s);
            cnt++;
            if (e < 0) e = -e;
            if (e > mx) mx = e;
        }
    }

    f->peak = pk;
    f->maxerr = round_q24((int64_t)mx * 100);
    f->rms = cnt ? (int)(((uint64_t)isqrt64(ss / cnt) * 100 + 32768) >> 16)
                 : 0;
}

/* Round to setting units, then try +-1 step changes to each setting while
 * the weighted error goes down. */
static void quantise_polish(const struct band *bs, int nb, struct fit *f)
{
    static int32_t total[NGRID];
    uint64_t pen = 0;

    for (int b = 0; b < nb; b++)
    {
        quantise(&bs[b], &f->b[b]);
        qband_curve(&f->b[b], grid_l, NGRID, curves[b]);
        pen += qband_pen(&f->b[b]);
    }
    for (int i = 0; i < NGRID; i++)
        total[i] = sum_at(curves, nb, i);
    uint64_t cost = eval_cost(curves, nb, NULL) + pen;

    for (int pass = 0; pass < 25; pass++)
    {
        int changed = 0;
        for (int b = 0; b < nb; b++)
        {
            for (int j = 0; j < 3; j++)
            {
                for (int dir = -1; dir <= 1; dir += 2)
                {
                    struct qband t = f->b[b];
                    if (j == 0)
                    {
                        int step = t.fc / 200;
                        t.fc += dir * (step > 0 ? step : 1);
                    }
                    else if (j == 1)
                        t.q += dir;
                    else
                        t.g += dir;
                    if (!qband_valid(&t))
                        continue;

                    qband_curve(&t, grid_l, NGRID, tmpc);
                    uint64_t tpen = pen - qband_pen(&f->b[b]) +
                                    qband_pen(&t);
                    uint64_t c = tpen;
                    for (int i = 0; i < NGRID; i++)
                    {
                        int64_t e = (int64_t)total[i] - curves[b][i] +
                                    tmpc[i] - target[i];
                        int32_t s = (int32_t)clamp64(
                            (e * grid_w[i]) >> 23, INT32_MAX);      /* Q16 */
                        c += (uint64_t)((int64_t)s * s);
                    }
                    if (c < cost)
                    {
                        cost = c;
                        pen = tpen;
                        f->b[b] = t;
                        for (int i = 0; i < NGRID; i++)
                        {
                            total[i] += tmpc[i] - curves[b][i];
                            curves[b][i] = tmpc[i];
                        }
                        changed = 1;
                    }
                }
            }
        }
        yield_cpu();
        if (!changed)
            break;
    }

    finish_fit(f, nb);
}

/* Fill orig[] from the EQ settings; returns the number of enabled bands */
static int load_bands(const struct eq_band_setting *set)
{
    norig = 0;
    for (int i = 0; i < EQ_NUM_BANDS; i++)
    {
        if (set[i].gain == 0)
            continue;
        struct qband *q = &orig[norig++];
        q->slot = i;
        q->type = set[i].type;
        q->fc = set[i].cutoff;
        q->q = set[i].q;
        q->g = set[i].gain;
    }
    return norig;
}

static void optimise(void)
{
    struct band cur[MAXB], trial[MAXB], best[2][MAXB];

    init_tables();
    setup_grid();
    compute_target();

    /* the original itself, for reference */
    for (int b = 0; b < norig; b++)
        fits[norig].b[b] = orig[b];
    finish_fit(&fits[norig], norig);

    for (int b = 0; b < norig; b++)
    {
        band_from_q(&orig[b], &cur[b]);
        clamp_band(&cur[b]);
    }

    progress_done = 0;
    progress_total = 0;
    for (int k = norig; k > 1; k--)
        progress_total += k + 1;

    for (int k = norig; k > 1; k--)
    {
        uint64_t bestcost[2] = { UINT64_MAX, UINT64_MAX };
        int ncur = k - 1;
        for (int c = 0; c < k; c++)
        {
            int n = 0;
            for (int b = 0; b < k; b++)
                if (b != c)
                    trial[n++] = cur[b];
            uint64_t cost = lm_fit(trial, ncur, 4);
            if (cost < bestcost[1])
            {
                int slot = 1;
                if (cost < bestcost[0])
                {
                    bestcost[1] = bestcost[0];
                    for (int b = 0; b < ncur; b++)
                        best[1][b] = best[0][b];
                    slot = 0;
                }
                bestcost[slot] = cost;
                for (int b = 0; b < ncur; b++)
                    best[slot][b] = trial[b];
            }
            bump_progress();
        }

        /* the short refit only ranks candidates roughly: finish the top two */
        int pick = 0;
        uint64_t c0 = lm_fit(best[0], ncur, 60);
        if (bestcost[1] != UINT64_MAX && lm_fit(best[1], ncur, 60) < c0)
            pick = 1;
        for (int b = 0; b < ncur; b++)
            cur[b] = best[pick][b];
        quantise_polish(cur, ncur, &fits[ncur]);
        bump_progress();
    }
}

/* Precut that keeps the same headroom the user had for the original */
static int fit_precut(const struct fit *f, int orig_precut)
{
    int32_t extra = f->peak - fits[norig].peak;
    int p = orig_precut;
    if (extra > 0)
        p += (int)(((int64_t)extra * 10 + ONE_Q24 - 1) >> 24);
    if (p < 0) p = 0;
    if (p > 240) p = 240;
    return p;
}

#ifndef EQOPT_HOST_TEST

/** Plugin UI **/

static struct eq_band_setting saved_eq[EQ_NUM_BANDS];
static int saved_precut;

static int32_t disp_l[LCD_WIDTH], disp_orig[LCD_WIDTH];
static int32_t disp_fit[LCD_WIDTH], disp_tmp[LCD_WIDTH];
static int disp_n = -1;

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

static const char *type_name(int type)
{
    return type == EQ_FILTER_LOW_SHELF ? "LS" :
           type == EQ_FILTER_HIGH_SHELF ? "HS" : "PK";
}

static void progress(int done, int total)
{
    rb->splash_progress(done, total, "Optimising EQ...");
}

static void qbands_disp(const struct qband *b, int n, int32_t *out)
{
    for (int x = 0; x < LCD_WIDTH; x++)
        out[x] = 0;
    for (int i = 0; i < n; i++)
    {
        qband_curve(&b[i], disp_l, LCD_WIDTH, disp_tmp);
        for (int x = 0; x < LCD_WIDTH; x++)
            out[x] += disp_tmp[x];
    }
}

static void setup_display(void)
{
    for (int x = 0; x < LCD_WIDTH; x++)
        disp_l[x] = tan_l(L2_20 + (int32_t)(((int64_t)L2_1000 * x) /
                                            (LCD_WIDTH - 1)));
    qbands_disp(orig, norig, disp_orig);
}

static int freq_x(int f)
{
    return (int)(((int64_t)(log2_u64(f) - L2_20) * (LCD_WIDTH - 1) +
                  L2_1000 / 2) / L2_1000);
}

/* tenths, formatted as "-1.2" */
static void fmt_tenths(char *buf, size_t len, int v)
{
    rb->snprintf(buf, len, "%s%d.%d", v < 0 ? "-" : "", (v < 0 ? -v : v) / 10,
                 (v < 0 ? -v : v) % 10);
}

static void draw(int n)
{
    const struct fit *f = &fits[n];
    int fh, fw;
    char buf[48];

    if (disp_n != n)
    {
        qbands_disp(f->b, f->n, disp_fit);
        disp_n = n;
    }

    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_getstringsize("0", &fw, &fh);

    /* header: the longest form that fits the screen */
    {
        int r1 = f->rms / 100, r2 = f->rms % 100;
        int m1 = f->maxerr / 100, m2 = f->maxerr % 100;
        const char *bn = n == 1 ? "band" : "bands";
        int maxch = LCD_WIDTH / fw;
        rb->snprintf(buf, sizeof buf, "%d %s  rms %d.%02d dB  max %d.%02d dB",
                     n, bn, r1, r2, m1, m2);
        if ((int)rb->strlen(buf) > maxch)
            rb->snprintf(buf, sizeof buf, "%d %s rms %d.%02d max %d.%02d dB",
                         n, bn, r1, r2, m1, m2);
        if ((int)rb->strlen(buf) > maxch)
            rb->snprintf(buf, sizeof buf, "%d %s rms%d.%02d max%d.%02ddB",
                         n, bn, r1, r2, m1, m2);
        if ((int)rb->strlen(buf) > maxch)
            rb->snprintf(buf, sizeof buf, "%d %s rms%d.%02d max%d.%02d",
                         n, bn, r1, r2, m1, m2);
        if ((int)rb->strlen(buf) > maxch)
            rb->snprintf(buf, sizeof buf, "%d %s %d.%02d/%d.%02ddB",
                         n, bn, r1, r2, m1, m2);
        rb->lcd_putsxy(0, 0, buf);
    }
    int hdr_h = fh;
#if defined(CPU_ARM) && LCD_HEIGHT >= 96
    /* estimated EQ CPU load per band, in tenths of a MHz */
    {
#if ARM_ARCH >= 5
        const int per_band = 25;
#else
        const int per_band = 40;
#endif
        int cost = n * per_band, save = (norig - n) * per_band;
        rb->snprintf(buf, sizeof buf, "EQ %d.%d MHz, saves %d.%d MHz",
                     cost / 10, cost % 10, save / 10, save % 10);
        if ((int)rb->strlen(buf) * fw > LCD_WIDTH)
            rb->snprintf(buf, sizeof buf, "%d.%d MHz (-%d.%d)",
                         cost / 10, cost % 10, save / 10, save % 10);
        if ((int)rb->strlen(buf) * fw <= LCD_WIDTH)
        {
            rb->lcd_putsxy(0, fh, buf);
            hdr_h += fh;
        }
    }
#endif

    /* error strip (fit - original) along the bottom */
    int sh = LCD_HEIGHT / 6;
    if (sh < 11)
        sh = 11;
    if (!(sh & 1))
        sh++;
    int sy1 = LCD_HEIGHT - 1, sy0 = sy1 - sh + 1, sc = (sy0 + sy1) / 2;
    int32_t emax = 0;
    for (int x = 0; x < LCD_WIDTH; x++)
    {
        int32_t e = disp_fit[x] - disp_orig[x];
        if (e < 0)
            e = -e;
        if (e > emax)
            emax = e;
    }
    /* full scale: 0.5, 1, 2, 4 or 8 dB, in tenths */
    int escale = 5;
    while (escale < 80 && (int64_t)emax * 10 > (int64_t)escale * ONE_Q24)
        escale *= 2;

    int y0 = hdr_h + 1, y1 = sy0 - 2;
    int32_t lo = 0, hi = 0;
    for (int x = 0; x < LCD_WIDTH; x++)
    {
        if (disp_orig[x] < lo) lo = disp_orig[x];
        if (disp_orig[x] > hi) hi = disp_orig[x];
        if (disp_fit[x] < lo) lo = disp_fit[x];
        if (disp_fit[x] > hi) hi = disp_fit[x];
    }
    int ilo = -3, ihi = 3;
    while ((int64_t)ilo * ONE_Q24 > lo)
        ilo -= 3;
    while ((int64_t)ihi * ONE_Q24 < hi)
        ihi += 3;
    int gstep = (ihi - ilo) > 18 ? 6 : 3;
    int64_t span = (int64_t)(ihi - ilo) * ONE_Q24;
#define DB_Y(v) (y0 + (int)((((int64_t)ihi * ONE_Q24 - (v)) * (y1 - y0) + \
                             span / 2) / span))

#if LCD_DEPTH > 1
    unsigned oldfg = rb->lcd_get_foreground();
    rb->lcd_set_foreground(LCD_DARKGRAY);
#endif
    /* grid: decades and dB lines, dotted */
    static const int decades[] = { 100, 1000, 10000 };
    for (unsigned d = 0; d < ARRAYLEN(decades); d++)
    {
        int x = freq_x(decades[d]);
        for (int y = y0; y <= y1; y += 2)
            rb->lcd_drawpixel(x, y);
    }
    for (int v = ilo; v <= ihi; v += gstep)
    {
        int y = DB_Y((int64_t)v * ONE_Q24);
        if (v == 0)
            rb->lcd_hline(0, LCD_WIDTH - 1, y);
        else
            for (int x = 0; x < LCD_WIDTH; x += 2)
                rb->lcd_drawpixel(x, y);
    }
    rb->snprintf(buf, sizeof buf, "%+d", ihi);
    rb->lcd_putsxy(1, y0 + 1, buf);
    rb->snprintf(buf, sizeof buf, "%+d", ilo);
    rb->lcd_putsxy(1, y1 - fh, buf);
    if (LCD_HEIGHT >= 96)
    {
        rb->lcd_putsxy(freq_x(100) + 2, y1 - fh, "100");
        rb->lcd_putsxy(freq_x(1000) + 2, y1 - fh, "1k");
        rb->lcd_putsxy(freq_x(10000) + 2, y1 - fh, "10k");
    }

    /* original: solid */
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(oldfg);
#endif
    for (int x = 1; x < LCD_WIDTH; x++)
        rb->lcd_drawline(x - 1, DB_Y(disp_orig[x - 1]), x, DB_Y(disp_orig[x]));

    /* fit: coloured where possible, otherwise dashed */
#if LCD_DEPTH >= 16
    rb->lcd_set_foreground(LCD_RGBPACK(255, 120, 0));
    for (int x = 1; x < LCD_WIDTH; x++)
        rb->lcd_drawline(x - 1, DB_Y(disp_fit[x - 1]), x, DB_Y(disp_fit[x]));
#else
    for (int x = 1; x < LCD_WIDTH; x++)
        if ((x / 3) & 1)
            rb->lcd_drawline(x - 1, DB_Y(disp_fit[x - 1]),
                             x, DB_Y(disp_fit[x]));
#endif
#undef DB_Y

    /* error strip: separator, zero line, bars from zero */
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DARKGRAY);
#endif
    for (int x = 0; x < LCD_WIDTH; x += 2)
        rb->lcd_drawpixel(x, sc);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(oldfg);
#endif
    rb->lcd_hline(0, LCD_WIDTH - 1, sy0 - 1);
#if LCD_DEPTH >= 16
    rb->lcd_set_foreground(LCD_RGBPACK(255, 120, 0));
#endif
    int half = sc - sy0;
    int64_t fs = (int64_t)escale * ONE_Q24 / 10;
    /* one bar per column on colour screens; on greyscale and mono,
       one 2-pixel bar every 4 columns showing the largest error there */
#if LCD_DEPTH >= 16
    const int bstep = 1, bw = 1;
#else
    const int bstep = 4, bw = 2;
#endif
    for (int x = 0; x < LCD_WIDTH; x += bstep)
    {
        int64_t e = 0;
        for (int i = x; i < x + bstep && i < LCD_WIDTH; i++)
        {
            int64_t ei = disp_fit[i] - disp_orig[i];
            if ((ei < 0 ? -ei : ei) > (e < 0 ? -e : e))
                e = ei;
        }
        int d = (int)((e * half + (e < 0 ? -fs / 2 : fs / 2)) / fs);
        if (d > half)
            d = half;
        if (d < -half)
            d = -half;
        if (d)
            for (int i = x; i < x + bw && i < LCD_WIDTH; i++)
                rb->lcd_vline(i, sc, sc - d);
    }
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(oldfg);
#endif
    /* scale label (full scale in dB), left edge, on a cleared box */
    if (escale % 10)
        rb->snprintf(buf, sizeof buf, "%d.%d", escale / 10, escale % 10);
    else
        rb->snprintf(buf, sizeof buf, "%d", escale / 10);
    {
        int lw = rb->strlen(buf) * fw;
        int lx = 1, ly = sc - fh / 2;
        if (ly < sy0)
            ly = sy0;
        rb->lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
        rb->lcd_fillrect(lx - 1, ly, lw + 2, fh);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(lx, ly, buf);
    }

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(oldfg);
#endif
    rb->lcd_setfont(FONT_UI);
    rb->lcd_update();
}

static void show_bands(int n)
{
    const struct fit *f = &fits[n];
    int fh, fw;
    char g[8], q[8], buf[48];

    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_getstringsize("0", &fw, &fh);
    for (int i = 0; i < f->n; i++)
    {
        const struct qband *b = &f->b[i];
        fmt_tenths(g, sizeof g, b->g);
        fmt_tenths(q, sizeof q, b->q);
        rb->snprintf(buf, sizeof buf, "%d %s %5dHz %5sdB Q%s",
                     b->slot, type_name(b->type), b->fc, g, q);
        rb->lcd_putsxy(0, i * fh, buf);
    }
    fmt_tenths(g, sizeof g, -fit_precut(f, saved_precut));
    rb->snprintf(buf, sizeof buf, "precut %sdB", g);
    rb->lcd_putsxy(0, f->n * fh, buf);
    rb->lcd_setfont(FONT_UI);
    rb->lcd_update();
    rb->button_clear_queue();
    rb->button_get(true);
    rb->button_clear_queue();
}

static void apply_settings(const struct eq_band_setting *set, int precut)
{
    rb->memcpy(rb->global_settings->eq_band_settings, set,
               sizeof(struct eq_band_setting) * EQ_NUM_BANDS);
    rb->global_settings->eq_precut = precut;
    rb->settings_apply(false);
    rb->settings_save();
}

static void apply_fit(int n)
{
    struct eq_band_setting set[EQ_NUM_BANDS];
    const struct fit *f = &fits[n];

    /* unused slots keep their type, cutoff and Q but are switched off */
    rb->memcpy(set, saved_eq, sizeof set);
    for (int i = 0; i < EQ_NUM_BANDS; i++)
        set[i].gain = 0;
    for (int i = 0; i < f->n; i++)
    {
        struct eq_band_setting *s = &set[f->b[i].slot];
        s->type = f->b[i].type;
        s->cutoff = f->b[i].fc;
        s->q = f->b[i].q;
        s->gain = f->b[i].g;
    }
    apply_settings(set, fit_precut(f, saved_precut));
    rb->splashf(HZ, "Applied %d-band EQ", n);
}

/* returns true to quit */
static bool menu(int n)
{
    MENUITEM_STRINGLIST(m, "EQ Optimizer", NULL,
                        "Apply This Fit", "Show Bands",
                        "Revert to Original", "Quit");
    int sel = 0;
    switch (rb->do_menu(&m, &sel, NULL, false))
    {
        case 0:
            apply_fit(n);
            break;
        case 1:
            show_bands(n);
            break;
        case 2:
            apply_settings(saved_eq, saved_precut);
            rb->splash(HZ, "Original EQ restored");
            break;
        case 3:
            return true;
    }
    return false;
}

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;

    rb->memcpy(saved_eq, rb->global_settings->eq_band_settings,
               sizeof saved_eq);
    saved_precut = rb->global_settings->eq_precut;

    if (load_bands(saved_eq) < 2)
    {
        rb->splash(HZ * 2, "Need at least 2 enabled EQ bands");
        return PLUGIN_OK;
    }

    progress_cb = progress;
    rb->splash_progress_set_delay(HZ / 2);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    long t0 = *rb->current_tick;
    optimise();
    long dt = *rb->current_tick - t0;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    rb->splashf(HZ, "Done in %ld.%02lds", dt / HZ, (dt % HZ) * 100 / HZ);

    setup_display();

    /* default: fewest bands with a close fit */
    int n = norig - 1;
    for (int i = 1; i < norig; i++)
    {
        if (fits[i].rms <= 25 && fits[i].maxerr <= 100)
        {
            n = i;
            break;
        }
    }

    while (1)
    {
        draw(n);
        int action = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                                         ARRAYLEN(plugin_contexts));
        switch (action)
        {
            case PLA_LEFT:
            case PLA_LEFT_REPEAT:
#ifdef HAVE_SCROLLWHEEL
            case PLA_SCROLL_BACK:
            case PLA_SCROLL_BACK_REPEAT:
#endif
                if (n > 1)
                    n--;
                break;
            case PLA_RIGHT:
            case PLA_RIGHT_REPEAT:
#ifdef HAVE_SCROLLWHEEL
            case PLA_SCROLL_FWD:
            case PLA_SCROLL_FWD_REPEAT:
#endif
                if (n < norig)
                    n++;
                break;
            case PLA_SELECT:
                if (menu(n))
                    return PLUGIN_OK;
                break;
            case PLA_CANCEL:
            case PLA_EXIT:
                return PLUGIN_OK;
            default:
                if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

#endif /* !EQOPT_HOST_TEST */
