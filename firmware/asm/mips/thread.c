#if (CPU_MIPS == 32) || (CPU_MIPS == 32r2)
  #include "thread-mips32.c"
#else
  #error Missing thread impl
#endif

