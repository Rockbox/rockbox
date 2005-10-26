/**
 * @file bitstream.h
 * bitstream api header.
 */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <inttypes.h>

#ifndef BUILD_STANDALONE
  #include <config.h>
  #include <system.h>
#else
  #include <stdio.h>
  #define IBSS_ATTR
  #define ICONST_ATTR

#endif

/* Endian conversion routines for standalone compilation */
#ifdef BUILD_STANDALONE
    #ifdef BUILD_BIGENDIAN
        #define betoh32(x) (x)
        #define letoh32(x) swap32(x)
    #else
        #define letoh32(x) (x)
        #define betoh32(x) swap32(x)
    #endif

    /* Taken from rockbox/firmware/export/system.h */

    static inline unsigned short swap16(unsigned short value)
        /*
          result[15..8] = value[ 7..0];
          result[ 7..0] = value[15..8];
        */
    {
        return (value >> 8) | (value << 8);
    }

    static inline unsigned long swap32(unsigned long value)
        /*
          result[31..24] = value[ 7.. 0];
          result[23..16] = value[15.. 8];
          result[15.. 8] = value[23..16];
          result[ 7.. 0] = value[31..24];
        */
    {
        unsigned long hi = swap16(value >> 16);
        unsigned long lo = swap16(value & 0xffff);
        return (lo << 16) | hi;
    }
#endif

/* FLAC files are big-endian */
#define ALT_BITSTREAM_READER_BE
 
#define NEG_SSR32(a,s) (((int32_t)(a))>>(32-(s)))
#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))

/* bit input */
/* buffer, buffer_end and size_in_bits must be present and used by every reader */
typedef struct GetBitContext {
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
} GetBitContext;

#define VLC_TYPE int16_t

typedef struct VLC {
    int bits;
    VLC_TYPE (*table)[2]; ///< code, bits
    int table_size, table_allocated;
} VLC;

typedef struct RL_VLC_ELEM {
    int16_t level;
    int8_t len;
    uint8_t run;
} RL_VLC_ELEM;

#if defined(ARCH_SPARC) || defined(ARCH_ARMV4L)
#define UNALIGNED_STORES_ARE_BAD
#endif

/* used to avoid missaligned exceptions on some archs (alpha, ...) */
#if defined(ARCH_X86) || defined(ARCH_X86_64)
#    define unaligned32(a) (*(const uint32_t*)(a))
#else
#    ifdef __GNUC__
static inline uint32_t unaligned32(const void *v) {
    struct Unaligned {
	uint32_t i;
    } __attribute__((packed));

    return ((const struct Unaligned *) v)->i;
}
#    elif defined(__DECC)
static inline uint32_t unaligned32(const void *v) {
    return *(const __unaligned uint32_t *) v;
}
#    else
static inline uint32_t unaligned32(const void *v) {
    return *(const uint32_t *) v;
}
#    endif
#endif //!ARCH_X86


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
    will return the next num bits

SHOW_SBITS(name, gb, num)
    will return the next num bits and do sign extension

SKIP_BITS(name, gb, num)
    will skip over the next num bits
    note, this is equivalent to SKIP_CACHE; SKIP_COUNTER

SKIP_CACHE(name, gb, num)
    will remove the next num bits from the cache (note SKIP_COUNTER MUST be called before UPDATE_CACHE / CLOSE_READER)

SKIP_COUNTER(name, gb, num)
    will increment the internal bit counter (see SKIP_CACHE & SKIP_BITS)

LAST_SKIP_CACHE(name, gb, num)
    will remove the next num bits from the cache if it is needed for UPDATE_CACHE otherwise it will do nothing

LAST_SKIP_BITS(name, gb, num)
    is equivalent to SKIP_LAST_CACHE; SKIP_COUNTER

for examples see get_bits, show_bits, skip_bits, get_vlc
*/

static inline int unaligned32_be(const void *v)
{
#ifdef CONFIG_ALIGN
	const uint8_t *p=v;
	return (((p[0]<<8) | p[1])<<16) | (p[2]<<8) | (p[3]);
#else
	return betoh32( unaligned32(v)); //original
#endif
}

static inline int unaligned32_le(const void *v)
{
#ifdef CONFIG_ALIGN
       const uint8_t *p=v;
       return (((p[3]<<8) | p[2])<<16) | (p[1]<<8) | (p[0]);
#else
       return letoh32( unaligned32(v)); //original
#endif
}

#   define MIN_CACHE_BITS 25

#   define OPEN_READER(name, gb)\
        int name##_index= (gb)->index;\
        int name##_cache= 0;\

#   define CLOSE_READER(name, gb)\
        (gb)->index= name##_index;\

# ifdef ALT_BITSTREAM_READER_LE
#   define UPDATE_CACHE(name, gb)\
        name##_cache= unaligned32_le( ((const uint8_t *)(gb)->buffer)+(name##_index>>3) ) >> (name##_index&0x07);\

#   define SKIP_CACHE(name, gb, num)\
        name##_cache >>= (num);
# else
#   define UPDATE_CACHE(name, gb)\
        name##_cache= unaligned32_be( ((const uint8_t *)(gb)->buffer)+(name##_index>>3) ) << (name##_index&0x07);\

#   define SKIP_CACHE(name, gb, num)\
        name##_cache <<= (num);
# endif

// FIXME name?
#   define SKIP_COUNTER(name, gb, num)\
        name##_index += (num);\

#   define SKIP_BITS(name, gb, num)\
        {\
            SKIP_CACHE(name, gb, num)\
            SKIP_COUNTER(name, gb, num)\
        }\

#   define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#   define LAST_SKIP_CACHE(name, gb, num) ;

# ifdef ALT_BITSTREAM_READER_LE
#   define SHOW_UBITS(name, gb, num)\
        ((name##_cache) & (NEG_USR32(0xffffffff,num)))
# else
#   define SHOW_UBITS(name, gb, num)\
        NEG_USR32(name##_cache, num)
# endif

#   define SHOW_SBITS(name, gb, num)\
        NEG_SSR32(name##_cache, num)

#   define GET_CACHE(name, gb)\
        ((uint32_t)name##_cache)

static inline int get_bits_count(GetBitContext *s){
    return s->index;
}

static inline int get_sbits(GetBitContext *s, int n){
    register int tmp;
    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)
    tmp= SHOW_SBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n)
    CLOSE_READER(re, s)
    return tmp;
}

/**
 * reads 0-17 bits.
 * Note, the alt bitstream reader can read up to 25 bits, but the libmpeg2 reader can't
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
 * Note, the alt bitstream reader can read up to 25 bits, but the libmpeg2 reader can't
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
#ifdef ALT_BITSTREAM_READER_LE
    result>>= (index&0x07);
    result&= 1;
#else
    result<<= (index&0x07);
    result>>= 8 - 1;
#endif
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

/**
 * init GetBitContext.
 * @param buffer bitstream buffer, must be FF_INPUT_BUFFER_PADDING_SIZE bytes larger then the actual read bits
 * because some optimized bitstream readers read 32 or 64 bit at once and could read over the end
 * @param bit_size the size of the buffer in bits
 */
static inline void init_get_bits(GetBitContext *s,
                   const uint8_t *buffer, int bit_size)
{
    int buffer_size= (bit_size+7)>>3;
    if(buffer_size < 0 || bit_size < 0) {
        buffer_size = bit_size = 0;
        buffer = 0;
    }

    s->buffer= buffer;
    s->size_in_bits= bit_size;
    s->buffer_end= buffer + buffer_size;
    s->index=0;
    {
        OPEN_READER(re, s)
        UPDATE_CACHE(re, s)
        UPDATE_CACHE(re, s)
        CLOSE_READER(re, s)
    }
}

void align_get_bits(GetBitContext *s);

#endif /* BITSTREAM_H */
