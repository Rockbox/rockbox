/*
 * xrick/system/system.h
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net).
 * Copyright (C) 2008-2014 Pierluigi Vicinanza.
 * All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#ifndef _SYSTEM_H
#define _SYSTEM_H

/*
 * If compiling w/gcc, then we can use attributes. UNUSED(x) flags a
 * parameter or a variable as potentially being unused, so that gcc doesn't
 * complain about it.
 *
 * Note: from OpenAL code: Darwin OS cc is based on gcc and has __GNUC__
 * defined, yet does not support attributes. So in theory we should exclude
 * Darwin target here.
 */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute((unused))
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE
#  endif
#else
#  define UNUSED(x) x
#endif

/*
 * Detect Microsoft Visual C
 */
#ifdef _MSC_VER
/*
 * FIXME disable "integral size mismatch in argument; conversion supplied" warning
 * as long as the code has not been cleared -- there are so many of them...
 */
#pragma warning( disable : 4761 )
#endif

/*
 * Detect Microsoft Windows
 */
#if !defined( __WIN32__ ) && ( defined( WIN32 ) || defined( _WIN32 ) )
#define __WIN32__
#endif

#include "xrick/config.h"
#include "xrick/rects.h"
#include "xrick/data/img.h"
#ifdef ENABLE_SOUND
#include "xrick/data/sounds.h"
#endif

#include <stddef.h> /* size_t */
#include <sys/types.h> /* off_t */

/*
 * main section
 */
extern bool sys_init(int, char **);
extern void sys_shutdown(void);
extern void sys_error(const char *, ...);
extern void sys_printf(const char *, ...);
extern void sys_snprintf(char *, size_t, const char *, ...);
extern size_t sys_strlen(const char *);
extern U32 sys_gettime(void);
extern void sys_yield(void);
extern bool sys_cacheData(void);
extern void sys_uncacheData(void);

/*
 * memory section
 */
extern bool sysmem_init(void);
extern void sysmem_shutdown(void);
extern void *sysmem_push(size_t);
extern void sysmem_pop(void *);

/*
 * video section
 */
#define SYSVID_ZOOM 2
#define SYSVID_MAXZOOM 4
#define SYSVID_WIDTH 320
#define SYSVID_HEIGHT 200

extern U8 *sysvid_fb;  /* frame buffer */

extern bool sysvid_init(void);
extern void sysvid_shutdown(void);
extern void sysvid_update(const rect_t *);
extern void sysvid_clear(void);
extern void sysvid_zoom(S8);
extern void sysvid_toggleFullscreen(void);
extern void sysvid_setGamePalette(void);
extern void sysvid_setPalette(img_color_t *, U16);

/*
 * file management section
 */
typedef void *file_t;

extern const char *sysfile_defaultPath;

extern bool sysfile_setRootPath(const char *);
extern void sysfile_clearRootPath(void);

extern file_t sysfile_open(const char *);
extern int sysfile_seek(file_t file, long offset, int origin);
extern int sysfile_tell(file_t);
extern off_t sysfile_size(file_t);
extern int sysfile_read(file_t, void *, size_t, size_t);
extern void sysfile_close(file_t);

/*
 * events section
 */
extern void sysevt_poll(void);
extern void sysevt_wait(void);

/*
 * keyboard section
 */
extern U8 syskbd_up;
extern U8 syskbd_down;
extern U8 syskbd_left;
extern U8 syskbd_right;
extern U8 syskbd_pause;
extern U8 syskbd_end;
extern U8 syskbd_xtra;
extern U8 syskbd_fire;

/*
 * sound section
 */
#ifdef ENABLE_SOUND
extern const U8 syssnd_period; /* time between each sound update, in millisecond */

extern bool syssnd_init(void);
extern void syssnd_shutdown(void);
extern void syssnd_update(void);
extern void syssnd_vol(S8);
extern void syssnd_toggleMute(void);
extern void syssnd_play(sound_t *, S8);
extern void syssnd_pauseAll(bool);
extern void syssnd_stop(sound_t *);
extern void syssnd_stopAll(void);
#endif /* ENABLE_ SOUND */

/*
 * args section
 */
extern int sysarg_args_period;
extern int sysarg_args_map;
extern int sysarg_args_submap;
extern int sysarg_args_fullscreen;
extern int sysarg_args_zoom;
#ifdef ENABLE_SOUND
extern bool sysarg_args_nosound;
extern int sysarg_args_vol;
#endif /* ENABLE_ SOUND */
extern const char *sysarg_args_data;

extern bool sysarg_init(int, char **);

/*
 * joystick section
 */
#ifdef ENABLE_JOYSTICK
extern bool sysjoy_init(void);
extern void sysjoy_shutdown(void);
#endif

#endif /* ndef _SYSTEM_H */

/* eof */


