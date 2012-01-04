#if CPU_MIPS == 32
  #include "thread-mips32.c"
#else
  #error Missing thread impl
#endif

