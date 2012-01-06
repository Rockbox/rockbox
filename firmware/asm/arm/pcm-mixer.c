#if ARM_ARCH >= 6
  #include "pcm-mixer-armv6.c"
#elif ARM_ARCH >= 5
  #include "pcm-mixer-armv5.c"
#elif ARM_ARCH >= 4
  #include "pcm-mixer-armv4.c"
#endif
