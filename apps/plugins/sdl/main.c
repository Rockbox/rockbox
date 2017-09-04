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

struct prog_t {
    const char *name;
    int (*main)(int argc, char *argv[]);
    bool printf_enabled;
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
    {  "Wolf3D!!!!!",              wolf3d_main,        false  },
    {  "Test sound",               testsound_main,     true   },
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

enum plugin_status plugin_start(const void *param)
{
    (void) param;

#ifdef COMBINED_SDL
    int prog = 0;
    rb->set_int("Choose SDL Program", "", UNIT_INT, &prog, NULL, 1, 0, ARRAYLEN(programs) - 1, formatter);
    int (*main_fn)(int argc, char *argv[]) = programs[prog].main;
    printf_enabled = programs[prog].printf_enabled;
#else
    int (*main_fn)(int argc, char *argv[]) = my_main;
#endif

    /* we always use the audio buffer */
    size_t sz;
    void *audiobuf = rb->plugin_get_audio_buffer(&sz);
    /* wipe sig */
    rb->memset(audiobuf, 0, 4);
    if(init_memory_pool(sz, audiobuf) == (size_t) -1)
    {
        rb->splashf(HZ*2, "TLSF init failed!");
        return PLUGIN_ERROR;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    backlight_ignore_timeout();
    /* real exit handler */
#undef rb_atexit
    rb_atexit(cleanup);

    /* these settings provide a nice balance of efficiency, quality, and latency */
    char *arg[] = {"blah", "--audiobuffer", "2048", "--samplerate", "16000"};
    int rc = main_fn(ARRAYLEN(arg), arg);
    if(rc != 0)
        rb->splash(HZ * 2, SDL_GetError());

    rb->sleep(HZ * 2);

    return rc;
}
