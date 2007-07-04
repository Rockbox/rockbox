/**
 * @file common.h
 * common internal api header.
 */

#ifndef COMMON_H
#define COMMON_H

#include "ffmpeg_config.h"
#include <inttypes.h>

#define NEG_SSR32(a,s) ((( int32_t)(a))>>(32-(s)))
#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))

/* bit input */

typedef struct GetBitContext {
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
} GetBitContext;

static inline int get_bits_count(GetBitContext *s);

/* used to avoid missaligned exceptions on some archs (alpha, ...) */
static inline uint32_t unaligned32(const void *v) {
    struct Unaligned {
	uint32_t i;
    } __attribute__((packed));

    return ((const struct Unaligned *) v)->i;
}


/* Bitstream reader API docs:
name
    abritary name which is used as prefix for the internal variables

gb
    getbitcontext

OPEN_READER(name, gb)
    loads gb into local variables

CLOSE_READER(name, gb)
    stores local vars in gb

UPDATE_CACHE(name, gb)
    refills the internal cache from the bitstream
    after this call at least MIN_CACHE_BITS will be available,

GET_CACHE(name, gb)
    will output the contents of the internal cache, next bit is MSB of 32 or 64 bit (FIXME 64bit)

SHOW_UBITS(name, gb, num)
    will return the nest num bits

SHOW_SBITS(name, gb, num)
    will return the nest num bits and do sign extension

SKIP_BITS(name, gb, num)
    will skip over the next num bits
    note, this is equinvalent to SKIP_CACHE; SKIP_COUNTER

SKIP_CACHE(name, gb, num)
    will remove the next num bits from the cache (note SKIP_COUNTER MUST be called before UPDATE_CACHE / CLOSE_READER)

SKIP_COUNTER(name, gb, num)
    will increment the internal bit counter (see SKIP_CACHE & SKIP_BITS)

LAST_SKIP_CACHE(name, gb, num)
    will remove the next num bits from the cache if it is needed for UPDATE_CACHE otherwise it will do nothing

LAST_SKIP_BITS(name, gb, num)
    is equinvalent to SKIP_LAST_CACHE; SKIP_COUNTER

for examples see get_bits, show_bits, skip_bits, get_vlc
*/

static inline int unaligned32_be(const void *v)
{
#ifdef CONFIG_ALIGN
	const uint8_t *p=v;
	return (((p[0]<<8) | p[1])<<16) | (p[2]<<8) | (p[3]);
#else
	return be2me_32( unaligned32(v)); //original
#endif
}

#define MIN_CACHE_BITS 25

#define OPEN_READER(name, gb)\
        int name##_index= (gb)->index;\
        int name##_cache= 0;\

#define CLOSE_READER(name, gb)\
        (gb)->index= name##_index;\

#define UPDATE_CACHE(name, gb)\
        name##_cache= unaligned32_be( ((uint8_t *)(gb)->buffer)+(name##_index>>3) ) << (name##_index&0x07);\

#define SKIP_CACHE(name, gb, num)\
        name##_cache <<= (num);\

// FIXME name?
#define SKIP_COUNTER(name, gb, num)\
        name##_index += (num);\

#define SKIP_BITS(name, gb, num)\
        {\
            SKIP_CACHE(name, gb, num)\
            SKIP_COUNTER(name, gb, num)\
        }\

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num) ;

#define SHOW_UBITS(name, gb, num)\
        NEG_USR32(name##_cache, num)

#define SHOW_SBITS(name, gb, num)\
        NEG_SSR32(name##_cache, num)

#define GET_CACHE(name, gb)\
        ((uint32_t)name##_cache)

static inline int get_bits_count(GetBitContext *s){
    return s->index;
}

/**
 * reads 0-17 bits.
 * Note, the alt bitstream reader can read upto 25 bits, but the libmpeg2 reader cant
 */
static inline unsigned int get_bits(GetBitContext *s, int n){
    register int tmp;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    tmp= SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n)
    CLOSE_READER(re, s)
    return tmp;
}

unsigned int get_bits_long(GetBitContext *s, int n);

/**
 * shows 0-17 bits.
 * Note, the alt bitstream reader can read upto 25 bits, but the libmpeg2 reader cant
 */
static inline unsigned int show_bits(GetBitContext *s, int n){
    register int tmp;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    tmp= SHOW_UBITS(re, s, n);
//    CLOSE_READER(re, s)
    return tmp;
}

unsigned int show_bits_long(GetBitContext *s, int n);

static inline void skip_bits(GetBitContext *s, int n){
 //Note gcc seems to optimize this to s->index+=n for the ALT_READER :))
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    LAST_SKIP_BITS(re, s, n)
    CLOSE_READER(re, s)
}

static inline unsigned int get_bits1(GetBitContext *s){
    int index= s->index;
    uint8_t result= s->buffer[ index>>3 ];
    result<<= (index&0x07);
    result>>= 8 - 1;
    index++;
    s->index= index;

    return result;
}

static inline unsigned int show_bits1(GetBitContext *s){
    return show_bits(s, 1);
}

static inline void skip_bits1(GetBitContext *s){
    skip_bits(s, 1);
}

void init_get_bits(GetBitContext *s,
                   const uint8_t *buffer, int buffer_size);

int check_marker(GetBitContext *s, const char *msg);
void align_get_bits(GetBitContext *s);

//#define TRACE

#ifdef TRACE

static inline void print_bin(int bits, int n){
    int i;
    
    for(i=n-1; i>=0; i--){
        printf("%d", (bits>>i)&1);
    }
    for(i=n; i<24; i++)
        printf(" ");
}

static inline int get_bits_trace(GetBitContext *s, int n, char *file, char *func, int line){
    int r= get_bits(s, n);
    
    print_bin(r, n);
    printf("%5d %2d %3d bit @%5d in %s %s:%d\n", r, n, r, get_bits_count(s)-n, file, func, line);
    return r;
}
static inline int get_vlc_trace(GetBitContext *s, VLC_TYPE (*table)[2], int bits, int max_depth, char *file, char *func, int line){
    int show= show_bits(s, 24);
    int pos= get_bits_count(s);
    int r= get_vlc2(s, table, bits, max_depth);
    int len= get_bits_count(s) - pos;
    int bits2= show>>(24-len);
    
    print_bin(bits2, len);
    
    printf("%5d %2d %3d vlc @%5d in %s %s:%d\n", bits2, len, r, pos, file, func, line);
    return r;
}
static inline int get_xbits_trace(GetBitContext *s, int n, char *file, char *func, int line){
    int show= show_bits(s, n);
    int r= get_xbits(s, n);
    
    print_bin(show, n);
    printf("%5d %2d %3d xbt @%5d in %s %s:%d\n", show, n, r, get_bits_count(s)-n, file, func, line);
    return r;
}

#define get_bits(s, n)  get_bits_trace(s, n, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define get_bits1(s)    get_bits_trace(s, 1, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define get_xbits(s, n) get_xbits_trace(s, n, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define get_vlc(s, vlc)            get_vlc_trace(s, (vlc)->table, (vlc)->bits, 3, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define get_vlc2(s, tab, bits, max) get_vlc_trace(s, tab, bits, max, __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define tprintf printf

#else //TRACE
#define tprintf(_arg...) {}
#endif

/* define it to include statistics code (useful only for optimizing
   codec efficiency */
//#define STATS

#ifdef STATS

enum {
    ST_UNKNOWN,
    ST_DC,
    ST_INTRA_AC,
    ST_INTER_AC,
    ST_INTRA_MB,
    ST_INTER_MB,
    ST_MV,
    ST_NB,
};

extern int st_current_index;
extern unsigned int st_bit_counts[ST_NB];
extern unsigned int st_out_bit_counts[ST_NB];

void print_stats(void);
#endif

/* misc math functions */
extern const uint8_t ff_log2_tab[256];

static inline int av_log2(unsigned int v)
{
    int n;

    n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}

static inline int clip(int a, int amin, int amax)
{
    if (a < amin)
        return amin;
    else if (a > amax)
        return amax;
    else
        return a;
}

/* math */
extern const uint8_t ff_sqrt_tab[128];

int64_t ff_gcd(int64_t a, int64_t b);

static inline int ff_sqrt(int a)
{
    int ret=0;
    int s;
    int ret_sq=0;
    
    if(a<128) return ff_sqrt_tab[a];
    
    for(s=15; s>=0; s--){
        int b= ret_sq + (1<<(s*2)) + (ret<<s)*2;
        if(b<=a){
            ret_sq=b;
            ret+= 1<<s;
        }
    }
    return ret;
}

#endif /* COMMON_H */
