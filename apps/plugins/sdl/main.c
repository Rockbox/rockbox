#include "plugin.h"

#include "fixedpoint.h"
#include "lib/helper.h"
#include "lib/pluginlib_exit.h"


#include "SDL.h"
#include "SDL_video.h"

#ifndef COMBINED_SDL
extern int my_main(int argc, char *argv[]);
#else
//#if 0
extern int SDLBlock_main(int argc, char *argv[]);
extern int ballfield_main(int argc, char *argv[]);
extern int parallax3_main(int argc, char *argv[]);
extern int parallax4_main(int argc, char *argv[]);
extern int testbitmap_main(int argc, char *argv[]);
extern int testcursor_main(int argc, char *argv[]);
extern int testhread_main(int argc, char *argv[]);
extern int testplatform_main(int argc, char *argv[]);
extern int testsprite_main(int argc, char *argv[]);
extern int testwin_main(int argc, char *argv[]);
//#endif
extern int abe_main(int argc, char *argv[]);
extern int ballerburg_main(int argc, char **argv);
extern int raytrace_main(int argc, char *argv[]);
extern int wolf3d_main(int argc, char *argv[]);
extern int testsound_main(int argc, char *argv[]);
extern int duke3d_main(int argc,char  **argv);
extern int quake_main (int c, char **v);

char *wolf3d_argv[] = { "wolf3d", "--audiobuffer", "2048"  };
char *duke3d_argv[] = { "duke3d" };
char *quake_argv[] = { "quake", "-basedir", "/.rockbox/quake" };

void *audiobuf = NULL;

struct prog_t {
    const char *name;
    int (*main)(int argc, char *argv[]);
    bool printf_enabled;
    int argc;
    char **argv;
} programs[] = {
#if 0
    {  "Abe's Amazing Adventure",  abe_main,           true   },
    {  "Ballerburg",               ballerburg_main,    false  },
    {  "Screensaver",              SDLBlock_main,      false  },
    {  "Ball Field",               ballfield_main,     false  },
    {  "Parallax v.3",             parallax3_main,     false  },
    {  "Parallax v.4",             parallax4_main,     false  },
    {  "Raytrace",                 raytrace_main,      false  },
    {  "Test Bitmap",              testbitmap_main,    false  },
    {  "Test Cursor",              testcursor_main,    false  },
    {  "Test Thread",              testhread_main,     true   },
    {  "Test Platform",            testplatform_main,  true   },
    {  "Test Sprite",              testsprite_main,    false  },
    {  "Test Window",              testwin_main,       false  },
#endif
    {  "Wolf3D!!!!!",              wolf3d_main,        false, ARRAYLEN(wolf3d_argv), wolf3d_argv },
    {  "Duke3D",                   duke3d_main,        true, ARRAYLEN(duke3d_argv), duke3d_argv },
    {  "Quake",                    quake_main,         true, ARRAYLEN(quake_argv), quake_argv },
    {  "Test sound",               testsound_main,     true, 0, NULL },
};
#endif

bool printf_enabled = false;

static void (*exit_cb)(void) = NULL;

void rbsdl_atexit(void (*cb)(void))
{
    if(exit_cb)
    {
        rb->splash(HZ, "WARNING: multiple exit handlers!");
    }
    exit_cb = cb;
}

void cleanup(void)
{
    if(exit_cb)
        exit_cb();
    if(audiobuf)
        memset(audiobuf, 0, 4); /* clear */
    backlight_use_settings();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

const char *formatter(char *buf, size_t n, int i, const char *unit)
{
    snprintf(buf, n, "%s", programs[i].name);
    return buf;
}

#ifdef SIMULATOR
#include <signal.h>

void segv(int s, siginfo_t *si, void *ptr)
{
    LOGF("SEGV: at address %llx (%d)", si->si_addr, si->si_code);
    exit(1);
}

void install_segv(void)
{
    struct sigaction act;
    act.sa_handler = NULL;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = segv;
    sigaction(SIGSEGV, &act, NULL);
}
#endif

static long main_stack[1024 * 1024 / 4];
int (*main_fn)(int argc, char *argv[]);
int prog_idx;
static void main_thread(void)
{
    /* these settings provide a nice balance of efficiency, quality, and latency */
    main_fn = programs[prog_idx].main;

    char *fallback[] = { "/blah", NULL };
    char **argv = programs[prog_idx].argv ? programs[prog_idx].argv : fallback;
    int argc = programs[prog_idx].argv ? programs[prog_idx].argc : 1;

    int rc = main_fn(argc, argv);
    if(rc != 0)
        rb->splash(HZ * 2, SDL_GetError());

    rb->thread_exit();
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;

    /* we always use the audio buffer */
    size_t sz;
    audiobuf = rb->plugin_get_audio_buffer(&sz);
    /* wipe sig */
    rb->memset(audiobuf, 0, 4);
    rb->memset(audiobuf, 0, sz);
    if(init_memory_pool(sz, audiobuf) == (size_t) -1)
    {
        rb->splashf(HZ*2, "TLSF init failed!");
        return PLUGIN_ERROR;
    }

    rb->splash(0, "plugin start");

    rb->splash(0, "plugin start (0)");

    rb->splash(0, "plugin start (1)");

#ifdef SIMULATOR
    install_segv();
#endif

#ifdef COMBINED_SDL
    int prog = 0;
    rb->splash(0, "plugin start 1");

    rb->set_int("Choose SDL Program", "", UNIT_INT, &prog, NULL, 1, 0, ARRAYLEN(programs) - 1, formatter);
    prog_idx = prog;
    rb->splash(0, "plugin start 2");

    main_fn = programs[prog].main;
    printf_enabled = programs[prog].printf_enabled;
#else
    main_fn = my_main;
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    backlight_ignore_timeout();
    /* real exit handler */
#undef rb_atexit
    rb_atexit(cleanup);

    /* make a new thread to use a bigger stack */
    unsigned int id = rb->create_thread(main_thread, main_stack, sizeof(main_stack), 0, "sdl_main" IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    rb->thread_wait(id);
    //main_thread();
    rb->sleep(HZ); /* wait a second... */

    return PLUGIN_OK;
}
