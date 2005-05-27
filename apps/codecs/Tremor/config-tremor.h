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


// #define _LOW_ACCURACY_
