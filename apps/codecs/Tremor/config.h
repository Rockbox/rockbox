#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0

#ifdef SIMULATOR
  #define BYTE_ORDER LITTLE_ENDIAN
#else
  #define BYTE_ORDER BIG_ENDIAN
#endif
