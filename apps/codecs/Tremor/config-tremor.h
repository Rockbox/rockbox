#include "../codec.h"

#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0
#define _LOW_ACCURACY_

#ifdef SIMULATOR
  #define BYTE_ORDER LITTLE_ENDIAN
#else
  #define BYTE_ORDER BIG_ENDIAN
#endif
