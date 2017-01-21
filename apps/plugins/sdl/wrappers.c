#include "plugin.h"

#include "fixedpoint.h"

#include "SDL.h"

extern bool printf_enabled;

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

/* stolen from doom */
// Here is a hacked up printf command to get the output from the game.
int printf_wrapper(const char *fmt, ...)
{
    static int p_xtpt;
    char p_buf[50];
    rb->yield();
    va_list ap;

    va_start(ap, fmt);
    rb->vsnprintf(p_buf,sizeof(p_buf), fmt, ap);
    va_end(ap);

    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_putsxy(1,p_xtpt, (unsigned char *)p_buf);
    if (printf_enabled)
        rb->lcd_update();

    p_xtpt+=8;
    if(p_xtpt>LCD_HEIGHT-8)
    {
        p_xtpt=0;
        if (printf_enabled)
        {
            rb->lcd_set_backdrop(NULL);
            rb->lcd_clear_display();
        }
    }
    return 1;
}

int sprintf_wrapper(char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = rb->vsnprintf(str, 9999, fmt, ap);
    va_end(ap);
    return ret;
}

char *strcpy_wrapper(char *dest, const char *src)
{
    rb->strlcpy(dest, src, 999);
    return dest;
}

char *strdup_wrapper(const char *s) {
    char *r = malloc(1+strlen(s));
    strcpy(r,s);
    return r;
}

char *strcat_wrapper(char *dest, const char *src)
{
    rb->strlcat(dest, src, 999);
    return dest;
}
