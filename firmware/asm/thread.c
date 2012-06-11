  /* First some generic implementations */
#if defined(HAVE_WIN32_FIBER_THREADS)
  #include "thread-win32.c"
#elif defined(HAVE_SIGALTSTACK_THREADS)
  #include "thread-unix.c"

  /* Now the CPU-specific implementations */
#elif defined(CPU_ARM)
  #include "arm/thread.c"
#elif defined(CPU_COLDFIRE)
  #include "m68k/thread.c"
#elif CONFIG_CPU == SH7034
  #include "sh/thread.c"
#elif defined(CPU_MIPS)
  #include "mips/thread.c"
#else
  /* Nothing? OK, give up */
  #error Missing thread impl
#endif
