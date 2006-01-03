#include "../codec.h" 
#ifdef ROCKBOX_BIG_ENDIAN
#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#define BIG_ENDIAN 0
#endif

/* As of 28 Dec 2005, the Tremor code is too big for IRAM on the iPod,
   so we temporarily disable ICODE_ATTR - this needs reviewing when it comes
   to optimising Tremor for the iPod */
#ifdef CPU_ARM
#undef ICODE_ATTR
#define ICODE_ATTR
#endif

// #define _LOW_ACCURACY_
