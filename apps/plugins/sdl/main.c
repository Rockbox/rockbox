#include "plugin.h"

#include "fixedpoint.h"
#include "lib/helper.h"
#include "lib/pluginlib_exit.h"


#include "SDL.h"
#include "SDL_video.h"

#ifdef COMBINED_SDL
#error deprecated
#endif

/* SDL program */
extern int my_main(int argc, char *argv[]);

void *audiobuf = NULL;

unsigned int sdl_thread_id, main_thread_id;

bool printf_enabled = true;

static void (*exit_cb)(void) = NULL;

/* sets the exit handler presented to the program */
void rbsdl_atexit(void (*cb)(void))
{
    if(exit_cb)
    {
        rb->splash(HZ, "WARNING: multiple exit handlers!");
    }
    exit_cb = cb;
}

/* called by program */
void rb_exit(int rc)
{
    if(rb->thread_self() == main_thread_id) /* rockbox main thread */
        exit(rc);
    else
        rb->thread_exit();
}

/* exit handler, called by rockbox */
void cleanup(void)
{
    if(exit_cb)
        exit_cb();

    if(exit_cb != SDL_Quit)
        SDL_Quit();

#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR) && !defined(RB_PROFILE)
#define USE_TIMER
#endif

#ifdef USE_TIMER
    /* stop timer callback if there is one, since the memory it
     * resides in could be overwritten */
    rb->timer_unregister();
#endif

    if(audiobuf)
        memset(audiobuf, 0, 4); /* clear */

#ifdef HAVE_BACKLIGHT
    backlight_use_settings();
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

static long main_stack[1024 * 1024 / 4]; /* ie 1 MB */

int (*main_fn)(int argc, char *argv[]);
int prog_idx;
static void main_thread(void)
{
    char *fallback[] = { "/blah", NULL };

#ifdef COMBINED_SDL
    char **argv = programs[prog_idx].argv ? programs[prog_idx].argv : fallback;
    int argc = programs[prog_idx].argv ? programs[prog_idx].argc : 1;
#else
    char **argv = fallback;
    int argc = 1;
#endif

    int rc = main_fn(argc, argv);
    if(rc != 0)
        rb->splash(HZ * 2, SDL_GetError());

    rb->thread_exit();
}

#if defined(CPU_ARM) && !defined(SIMULATOR)
#define CR_M    (1 << 0)        /* MMU enable                           */
#define CR_A    (1 << 1)        /* Alignment abort enable               */
#define CR_C    (1 << 2)        /* Dcache enable                        */
#define CR_W    (1 << 3)        /* Write buffer enable                  */
#define CR_P    (1 << 4)        /* 32-bit exception handler             */
#define CR_D    (1 << 5)        /* 32-bit data address range            */
#define CR_L    (1 << 6)        /* Implementation defined               */
#define CR_B    (1 << 7)        /* Big endian                           */
#define CR_S    (1 << 8)        /* System MMU protection                */
#define CR_R    (1 << 9)        /* ROM MMU protection                   */
#define CR_F    (1 << 10)       /* Implementation defined               */
#define CR_Z    (1 << 11)       /* Implementation defined               */
#define CR_I    (1 << 12)       /* Icache enable                        */
#define CR_V    (1 << 13)       /* Vectors relocated to 0xffff0000      */
#define CR_RR   (1 << 14)       /* Round Robin cache replacement        */
#define CR_L4   (1 << 15)       /* LDR pc can set T bit                 */
#define CR_DT   (1 << 16)
#ifdef CONFIG_MMU
#define CR_HA   (1 << 17)       /* Hardware management of Access Flag   */
#else
#define CR_BR   (1 << 17)       /* MPU Background region enable (PMSA)  */
#endif
#define CR_IT   (1 << 18)
#define CR_ST   (1 << 19)
#define CR_FI   (1 << 21)       /* Fast interrupt (lower latency mode)  */
#define CR_U    (1 << 22)       /* Unaligned access operation           */
#define CR_XP   (1 << 23)       /* Extended page tables                 */
#define CR_VE   (1 << 24)       /* Vectored interrupts                  */
#define CR_EE   (1 << 25)       /* Exception (Big) Endian               */
#define CR_TRE  (1 << 28)       /* TEX remap enable                     */
#define CR_AFE  (1 << 29)       /* Access flag enable                   */
#define CR_TE   (1 << 30)       /* Thumb exception enable               */

#define __LINUX_ARM_ARCH__ ARM_ARCH

#if __LINUX_ARM_ARCH__ >= 7
#define isb(option) __asm__ __volatile__ ("isb " #option : : : "memory")
#define dsb(option) __asm__ __volatile__ ("dsb " #option : : : "memory")
#define dmb(option) __asm__ __volatile__ ("dmb " #option : : : "memory")
#elif defined(CONFIG_CPU_XSC3) || __LINUX_ARM_ARCH__ == 6
#define isb(x) __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
                                    : : "r" (0) : "memory")
#define dsb(x) __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
                                    : : "r" (0) : "memory")
#define dmb(x) __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" \
                                    : : "r" (0) : "memory")
#elif defined(CONFIG_CPU_FA526)
#define isb(x) __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
                                    : : "r" (0) : "memory")
#define dsb(x) __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
                                    : : "r" (0) : "memory")
#define dmb(x) __asm__ __volatile__ ("" : : : "memory")
#else
#define isb(x) __asm__ __volatile__ ("" : : : "memory")
#define dsb(x) __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
                                    : : "r" (0) : "memory")
#define dmb(x) __asm__ __volatile__ ("" : : : "memory")
#endif

static inline unsigned long get_cr(void)
{
        unsigned long val;
        asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
	return val;
}

static inline void set_cr(unsigned long val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}
#endif

enum plugin_status plugin_start(const void *param)
{
    (void) param;

#if defined(CPU_ARM) && !defined(SIMULATOR) && (ARM_ARCH >= 5)
    /* (don't) set alignment trap. Will generate a data abort
     * exception on ARM. */
//    set_cr(get_cr() | CR_A);
#endif

#if 0
    char c = *((char*)NULL);

    /* test alignment trap */
    unsigned int x = 0x12345678;
    char *p = ((char*)&x) + 1;
    unsigned short *p2 = (unsigned short*)p;
    rb->splashf(HZ, "%04x, %02x%02x", *p2, *(p+1), *p);
#endif

    /* don't confuse this with the main SDL thread! */
    main_thread_id = rb->thread_self();

    /* we always use the audio buffer */
    size_t sz;
    audiobuf = rb->plugin_get_audio_buffer(&sz);
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    /* We are loaded as an overlay towards the end of the audio
     * buffer. We must take care not to overwrite ourselves. */
    if ((uintptr_t)audiobuf < (uintptr_t)plugin_start_addr)
    {
        uint32_t tmp_size = (uintptr_t)plugin_start_addr - (uintptr_t)audiobuf;
        sz = MIN(sz, tmp_size);
    }
#endif

    /* wipe sig */
    rb->memset(audiobuf, 0, sz);
    if(init_memory_pool(sz, audiobuf) == (size_t) -1)
    {
        rb->splashf(HZ*2, "TLSF init failed!");
        return PLUGIN_ERROR;
    }

    main_fn = my_main;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
#ifdef HAVE_BACKLIGHT
    backlight_ignore_timeout();
#endif
    /* set the real exit handler */
#undef rb_atexit
    rb_atexit(cleanup);

    /* make a new thread to use a bigger stack and higher priority than otherwise accessible */
    sdl_thread_id = rb->create_thread(main_thread, main_stack,
                                      sizeof(main_stack), 0, "sdl_main"
                                      IF_PRIO(, PRIORITY_USER_INTERFACE) // we need other threads still
                                      IF_COP(, CPU));
    rb->thread_wait(sdl_thread_id);

    rb->sleep(HZ); /* wait a second in case there's an error message on screen */

    return PLUGIN_OK;
}
