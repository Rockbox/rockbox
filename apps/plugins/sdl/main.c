#include "plugin.h"

#include "fixedpoint.h"
#include "lib/helper.h"
#include "lib/pluginlib_exit.h"


#include "SDL.h"
#include "SDL_video.h"

#ifndef COMBINED_SDL
extern int my_main(int argc, char *argv[]);
#else
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

struct prog_t {
    const char *name;
    int (*main)(int argc, char *argv[]);
} programs[] = {
    { "Screensaver",   SDLBlock_main },
    { "Ball Field",    ballfield_main },
    { "Parallax v.3",  parallax3_main },
    { "Parallax v.4",  parallax4_main },
    { "Test Bitmap",   testbitmap_main },
    { "Test Cursor",   testcursor_main },
    { "Test Thread",   testhread_main },
    { "Test Platform", testplatform_main },
    { "Test Sprite",   testsprite_main },
    { "Test Window",   testwin_main }
};
#endif

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
    rb->snprintf(buf, n, "%s", programs[i].name);
    return buf;
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;

#ifdef COMBINED_SDL
    int prog = 0;
    rb->set_int("Choose SDL Program", "", UNIT_INT, &prog, NULL, 1, 0, ARRAYLEN(programs) - 1, formatter);
    int (*main_fn)(int argc, char *argv[]) = programs[prog].main;
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
        rb->splashf(HZ*2, "TLSF failed!");
        return PLUGIN_ERROR;
    }

#if 0
    /* weird crashes */
    void *audiobuf = rb->plugin_get_audio_buffer(&sz);
    if(audiobuf)
    {
        rb->splashf(HZ, "point 1 (sz=%d, addr=0x%08x)", sz, audiobuf);
        add_new_area(audiobuf, sz, pluginbuf);
        rb->splash(HZ, "point 2");
    }
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    backlight_ignore_timeout();
    /* real exit handler */
#undef rb_atexit
    rb_atexit(cleanup);

    char *arg[] = {"blah", NULL };
    int rc = main_fn(1, arg);
    if(rc != 0)
        rb->splash(HZ * 2, SDL_GetError());
    return rc;
}

/* fixed-point wrappers */
static unsigned long lastphase = 0;
static long lastsin = 0, lastcos = 0x7fffffff;

#define PI 3.1415926535897932384626433832795

double sin_wrapper(double rads)
{
    /* we want [0, 2*PI) */
    while(rads >= 2*PI)
        rads -= 2*PI;
    while(rads < 0)
        rads += 2*PI;

    unsigned long phase = rads/(2*PI) * 4294967296.0;

    /* caching */
    if(phase == lastphase)
    {
        return lastsin/(lastsin < 0 ? 2147483648.0 : 2147483647.0);
    }

    lastphase = phase;
    lastsin = fp_sincos(phase, &lastcos);
    return lastsin/(lastsin < 0 ? 2147483648.0 : 2147483647.0);
}

double cos_wrapper(double rads)
{
    /* we want [0, 2*PI) */
    while(rads >= 2*PI)
        rads -= 2*PI;
    while(rads < 0)
        rads += 2*PI;

    unsigned long phase = rads/(2*PI) * 4294967296.0;

    /* caching */
    if(phase == lastphase)
    {
        return lastcos/(lastcos < 0 ? 2147483648.0 : 2147483647.0);
    }

    lastphase = phase;
    lastsin = fp_sincos(phase, &lastcos);
    return lastcos/(lastcos < 0 ? 2147483648.0 : 2147483647.0);
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[128];
    rb->vsnprintf(buf, 128, fmt, ap);
    //rb->splash(HZ, buf);
    va_end(ap);
    return 0;
}
