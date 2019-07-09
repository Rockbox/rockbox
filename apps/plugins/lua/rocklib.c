/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
 * Copyright (C) 2009 Maurus Cuelenaere
 * Copyright (C) 2018 William Wilgus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#define lrocklib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "rocklib.h"
#include "lib/helper.h"
#include "lib/pluginlib_actions.h"

/*
 * http://www.lua.org/manual/5.1/manual.html#lua_CFunction
 *
 * In order to communicate properly with Lua, a C function must use the following protocol,
 * which defines the way parameters and results are passed: a C function receives its arguments
 * from Lua in its stack in direct order (the first argument is pushed first). To return values to Lua,
 * a C function just pushes them onto the stack, in direct order (the first result is pushed first),
 * and returns the number of results. Any other value in the stack below the results will be properly
 * discarded by Lua. Like a Lua function, a C function called by Lua can also return many results. 
 *
 * When porting new functions, don't forget to check rocklib_aux.pl whether it automatically creates
 * wrappers for the function and if so, add the function names to @forbidden_functions. This is to
 * prevent namespace collisions and adding duplicate wrappers.
 */

/*
 * -----------------------------
 *
 *  Rockbox wrappers start here!
 *
 * -----------------------------
 */

#define RB_WRAP(func) static int rock_##func(lua_State UNUSED_ATTR *L)
#define SIMPLE_VOID_WRAPPER(func) RB_WRAP(func) { (void)L; func(); return 0; }

RB_WRAP(current_tick)
{
    lua_pushinteger(L, *rb->current_tick);
    return 1;
}

#ifdef HAVE_SCHEDULER_BOOSTCTRL
RB_WRAP(schedule_cpu_boost)
{
    bool boost = luaL_checkboolean(L, 1);

    if(boost)
        rb->trigger_cpu_boost();
    else
        rb->cancel_cpu_boost();

    return 0;
}
#endif

RB_WRAP(current_path)
{
    return get_current_path(L, 1);
}


/* DEVICE INPUT CONTROL */

RB_WRAP(get_plugin_action)
{
    static const struct button_mapping *m1[] = { pla_main_ctx };
    int timeout = luaL_checkint(L, 1);
    int btn;

#ifdef HAVE_REMOTE_LCD
    static const struct button_mapping *m2[] = { pla_main_ctx, pla_remote_ctx };
    bool with_remote = luaL_optint(L, 2, 0);
    if (with_remote)
        btn = pluginlib_getaction(timeout, m2, 2);
    else
#endif
        btn = pluginlib_getaction(timeout, m1, 1);

    lua_pushinteger(L, btn);
    return 1;
}

#ifdef HAVE_TOUCHSCREEN
RB_WRAP(action_get_touchscreen_press)
{
    short x, y;
    int result = rb->action_get_touchscreen_press(&x, &y);

    lua_pushinteger(L, result);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 3;
}

RB_WRAP(touchscreen_mode)
{
    int origmode = rb->touchscreen_get_mode();
    if(!lua_isnoneornil(L, 1))
    {
        enum touchscreen_mode mode = luaL_checkint(L, 1);
        rb->touchscreen_set_mode(mode);
    }
    lua_pushinteger(L, origmode);
    return 1;
}

#endif

RB_WRAP(kbd_input)
{
    luaL_Buffer b;
    luaL_buffinit(L, &b);

    const char *input = lua_tostring(L, 1);
    char *buffer = luaL_prepbuffer(&b);

    if(input != NULL)
        rb->strlcpy(buffer, input, LUAL_BUFFERSIZE);
    else
        buffer[0] = '\0';

    if(!rb->kbd_input(buffer, LUAL_BUFFERSIZE))
    {
        luaL_addstring(&b, buffer);
        luaL_pushresult(&b);
    }
    else
        return 0;

    return 1;
}

static const char ** get_table_items(lua_State *L, int pos, int *count)
{
    int i;
    luaL_checktype(L, pos, LUA_TTABLE);
    *count = lua_objlen(L, pos);
    int n = *count;

    /* newuserdata will be pushed onto stack after args*/
    const char **items = (const char**) lua_newuserdata(L, n * sizeof(const char*));

    for(i=1; i<= n; i++)
    {
        lua_rawgeti(L, pos, i); /* Push item on the stack */
        items[i-1] = lua_tostring(L, -1);
        lua_pop(L, 1); /* Pop it */
    }

    return items;
}

static inline void fill_text_message(lua_State *L, struct text_message * message,
                              int pos)
{
    int n;
    /* newuserdata will be pushed onto stack after args*/
    message->message_lines = get_table_items(L, pos, &n);
    message->nb_lines = n;
}

RB_WRAP(gui_syncyesno_run)
{
    struct text_message main_message, yes_message, no_message;
    struct text_message *yes = NULL, *no = NULL;

    lua_settop(L, 3); /* newuserdata will be pushed onto stack after args*/
    fill_text_message(L, &main_message, 1);
    if(!lua_isnoneornil(L, 2))
        fill_text_message(L, (yes = &yes_message), 2);
    if(!lua_isnoneornil(L, 3))
        fill_text_message(L, (no = &no_message), 3);

    enum yesno_res result = rb->gui_syncyesno_run(&main_message, yes, no);

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(do_menu)
{
    struct menu_callback_with_desc menu_desc = {NULL, NULL, Icon_NOICON};
    struct menu_item_ex menu = {MT_RETURN_ID | MENU_HAS_DESC, {.strings = NULL},
                                {.callback_and_desc = &menu_desc}};
    int n, start_selected;
    const char **items, *title;

    title = luaL_checkstring(L, 1);

    start_selected = lua_tointeger(L, 3);

    /* newuserdata will be pushed onto stack after args*/
    items = get_table_items(L, 2, &n);

    menu.strings = items;
    menu.flags |= MENU_ITEM_COUNT(n);
    menu_desc.desc = (unsigned char*) title;

    int result = rb->do_menu(&menu, &start_selected, NULL, false);

    lua_pushinteger(L, result);
    return 1;
}


/* DEVICE AUDIO / PLAYLIST CONTROL */

RB_WRAP(playlist)
{
    /* just passes NULL to work with the current playlist */
    enum e_playlist {PLAYL_AMOUNT = 0, PLAYL_ADD, PLAYL_CREATE,
                     PLAYL_START, PLAYL_RESUMETRACK, PLAYL_RESUME,
                     PLAYL_SHUFFLE, PLAYL_SYNC, PLAYL_REMOVEALLTRACKS,
                     PLAYL_INSERTTRACK, PLAYL_INSERTDIRECTORY, PLAYL_ECOUNT};

    const char *playlist_option[] = {"amount", "add", "create", "start", "resume_track",
                                     "resume", "shuffle", "sync", "remove_all_tracks",
                                     "insert_track", "insert_directory", NULL};

    const char *filename, *dir;
    int result = 0;
    bool queue, recurse, sync;
    int pos, crc, index, random_seed;
    unsigned long elapsed, offset;

    int option = luaL_checkoption (L, 1, NULL, playlist_option);
    switch(option)
    {
        case PLAYL_AMOUNT:
            result = rb->playlist_amount();
            break;
        case PLAYL_ADD:
            filename = luaL_checkstring(L, 2);
            result = rb->playlist_add(filename);
            break;
        case PLAYL_CREATE:
            dir = luaL_checkstring(L, 2);
            filename = luaL_checkstring(L, 3);
            result = rb->playlist_create(dir, filename);
            break;
        case PLAYL_START:
            index = luaL_checkint(L, 2);
            elapsed = luaL_checkint(L, 3);
            offset =  luaL_checkint(L, 4);
            rb->playlist_start(index, elapsed, offset);
            break;
        case PLAYL_RESUMETRACK:
            index = luaL_checkint(L, 2);
            crc = luaL_checkint(L, 3);
            elapsed = luaL_checkint(L, 4);
            offset = luaL_checkint(L, 5);
            rb->playlist_resume_track(index, (unsigned) crc, elapsed, offset);
            break;
        case PLAYL_RESUME:
            result = rb->playlist_resume();
            break;
        case PLAYL_SHUFFLE:
            random_seed = luaL_checkint(L, 2);
            index = luaL_checkint(L, 3);
            result = rb->playlist_shuffle(random_seed, index);
            break;
        case PLAYL_SYNC:
            rb->playlist_sync(NULL);
            break;
        case PLAYL_REMOVEALLTRACKS:
            result = rb->playlist_remove_all_tracks(NULL);
            break;
        case PLAYL_INSERTTRACK:
            filename = luaL_checkstring(L, 2); /* only required parameter */
            pos = luaL_optint(L, 3, PLAYLIST_INSERT);
            queue = lua_toboolean(L, 4); /* default to false */
            sync = lua_toboolean(L, 5); /* default to false */
            result = rb->playlist_insert_track(NULL, filename, pos, queue, sync);
            break;
        case PLAYL_INSERTDIRECTORY:
            dir = luaL_checkstring(L, 2); /* only required parameter */
            pos = luaL_optint(L, 3, PLAYLIST_INSERT);
            queue = lua_toboolean(L, 4); /* default to false */
            recurse = lua_toboolean(L, 5); /* default to false */
            result = rb->playlist_insert_directory(NULL, dir, pos, queue, recurse);
            break;
    }

    rb->yield();
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(audio)
{
    enum e_audio {AUDIO_STATUS = 0, AUDIO_PLAY, AUDIO_STOP, AUDIO_PAUSE,
                  AUDIO_RESUME, AUDIO_NEXT, AUDIO_PREV, AUDIO_FFREWIND,
                  AUDIO_FLUSHANDRELOADTRACKS, AUDIO_GETPOS, AUDIO_LENGTH,
                  AUDIO_ELAPSED, AUDIO_ECOUNT};
    const char *audio_option[] = {"status", "play", "stop",
                                  "pause", "resume", "next",
                                  "prev", "ff_rewind",
                                  "flush_and_reload_tracks",
                                  "get_file_pos", "length",
                                  "elapsed", NULL};
    long elapsed, offset, newtime;
    int status = rb->audio_status();

    int option = luaL_checkoption (L, 1, NULL, audio_option);
    switch(option)
    {
        case AUDIO_STATUS:
            break;
        case AUDIO_PLAY:
            elapsed = luaL_checkint(L, 2);
            offset  = luaL_checkint(L, 3);

            if (status == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE))
            {
                /* not perfect but provides a decent compromise */
                rb->audio_ff_rewind(elapsed + offset);
                rb->audio_resume();
            }
            else if (status != AUDIO_STATUS_PLAY)
                rb->audio_play((unsigned long) elapsed, (unsigned long) offset);

            break;
        case AUDIO_STOP:
            rb->audio_stop();
            break;
        case AUDIO_PAUSE:
            rb->audio_pause();
            break;
        case AUDIO_RESUME:
            rb->audio_resume();
            break;
        case AUDIO_NEXT:
            rb->audio_next();
            break;
        case AUDIO_PREV:
            rb->audio_prev();
            break;
        case AUDIO_FFREWIND:
            newtime = (long) luaL_checkint(L, 2);
            rb->audio_ff_rewind(newtime);
            break;
        case AUDIO_FLUSHANDRELOADTRACKS:
            rb->audio_flush_and_reload_tracks();
            break;
        case AUDIO_GETPOS:
            lua_pushinteger(L, rb->audio_get_file_pos());
            return 1;
        case AUDIO_LENGTH:
            if ((status & AUDIO_STATUS_PLAY) == AUDIO_STATUS_PLAY)
                lua_pushinteger(L, rb->audio_current_track()->length);
            else
                lua_pushnil(L);
            return 1;
        case AUDIO_ELAPSED:
            if ((status & AUDIO_STATUS_PLAY) == AUDIO_STATUS_PLAY)
                lua_pushinteger(L, rb->audio_current_track()->elapsed);
            else
                lua_pushnil(L);
            return 1;
    }

    rb->yield();
    lua_pushinteger(L, status); /* return previous (or current) audio status */
    return 1;
}

#if CONFIG_CODEC == SWCODEC
RB_WRAP(pcm)
{
    enum e_pcm {PCM_APPLYSETTINGS = 0, PCM_ISPLAYING, PCM_ISPAUSED,
                PCM_PLAYSTOP, PCM_PLAYPAUSE, PCM_PLAYLOCK, PCM_PLAYUNLOCK,
                PCM_CALCULATEPEAKS, PCM_SETFREQUENCY, PCM_GETBYTESWAITING, PCM_ECOUNT};

    const char *pcm_option[] = {"apply_settings", "is_playing", "is_paused",
                                "play_stop", "play_pause", "play_lock", "play_unlock",
                                "calculate_peaks", "set_frequency", "get_bytes_waiting", NULL};
    bool   b_result;
    int    left, right;
    size_t byteswait;

    lua_pushnil(L); /*push nil so options w/o return have something to return */

    int option = luaL_checkoption (L, 1, NULL, pcm_option);
    switch(option)
    {
        case PCM_APPLYSETTINGS:
            rb->pcm_apply_settings();
            break;
        case PCM_ISPLAYING:
            b_result = rb->pcm_is_playing();
            lua_pushboolean(L, b_result);
            break;
        case PCM_ISPAUSED:
            b_result = rb->pcm_is_paused();
            lua_pushboolean(L, b_result);
            break;
        case PCM_PLAYPAUSE:
            rb->pcm_play_pause(luaL_checkboolean(L, 2));
            break;
        case PCM_PLAYSTOP:
            rb->pcm_play_stop();
            break;
        case PCM_PLAYLOCK:
            rb->pcm_play_lock();
            break;
        case PCM_PLAYUNLOCK:
            rb->pcm_play_unlock();
            break;
        case PCM_CALCULATEPEAKS:
            rb->pcm_calculate_peaks(&left, &right);
            lua_pushinteger(L, left);
            lua_pushinteger(L, right);
            return 2;
        case PCM_SETFREQUENCY:
            rb->pcm_set_frequency((unsigned int) luaL_checkint(L, 2));
            break;
        case PCM_GETBYTESWAITING:
            byteswait = rb->pcm_get_bytes_waiting();
            lua_pushinteger(L, byteswait);
            break;
    }

    rb->yield();
    return 1;
}

RB_WRAP(mixer_frequency)
{
    unsigned int result = rb->mixer_get_frequency();

    if(!lua_isnoneornil(L, 1))
    {
        unsigned int samplerate = (unsigned int) luaL_checkint(L, 1);
        rb->mixer_set_frequency(samplerate);
    }
    lua_pushinteger(L, result);
    return 1;
}
#endif /*CONFIG_CODEC == SWCODEC*/

/* DEVICE LIGHTING CONTROL */
RB_WRAP(backlight_onoff)
{
    bool on = luaL_checkboolean(L, 1);
    if(on)
        rb->backlight_on();
    else
        rb->backlight_off();

    return 0;
}

SIMPLE_VOID_WRAPPER(backlight_force_on);
SIMPLE_VOID_WRAPPER(backlight_use_settings);

#ifdef HAVE_REMOTE_LCD
SIMPLE_VOID_WRAPPER(remote_backlight_force_on);
SIMPLE_VOID_WRAPPER(remote_backlight_use_settings);
#endif

#ifdef HAVE_BUTTON_LIGHT
SIMPLE_VOID_WRAPPER(buttonlight_force_on);
SIMPLE_VOID_WRAPPER(buttonlight_use_settings);
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
RB_WRAP(backlight_brightness_set)
{
    if(lua_isnoneornil(L, 1))
        backlight_brightness_use_setting();
    else
    {
        int brightness = luaL_checkint(L, 1);
        backlight_brightness_set(brightness);
    }

    return 0;
}
#endif

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
RB_WRAP(buttonlight_brightness_set)
{
    if(lua_isnoneornil(L, 1))
        buttonlight_brightness_use_setting();
    else
    {
        int brightness = luaL_checkint(L, 1);
        buttonlight_brightness_set(brightness);
    }

    return 0;
}
#endif


/* DEVICE STRING / FILENAME MANIPULATION */

RB_WRAP(strip_extension)
{
    const char* filename = luaL_checkstring(L, -1);
    const char* pos = rb->strrchr(filename, '.');
    if(pos != NULL)
        lua_pushlstring (L, filename, pos - filename);

    return 1;
}

RB_WRAP(create_numbered_filename)
{
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    char *buffer = luaL_prepbuffer(&b);
    buffer[0] = '\0';

    const char * path = luaL_checkstring(L, 1);
    const char * prefix = luaL_checkstring(L, 2);
    const char * suffix = luaL_checkstring(L, 3);
    int numberlen = luaL_checkint(L, 4);
    int num = luaL_optint(L, 5, -1);
    (void) num;

    if(rb->create_numbered_filename(buffer, path, prefix, suffix, numberlen
                                    IF_CNFN_NUM_(, &num)))
    {
        luaL_addstring(&b, buffer);
        luaL_pushresult(&b);
    }
    else
        return 0;

    return 1;
}

RB_WRAP(utf8encode)
{
    unsigned long ucs = (unsigned long) luaL_checkint(L, 1);
    unsigned char tmp[9];
    unsigned char *end = rb->utf8encode(ucs, tmp);
    *end = '\0';
    lua_pushstring(L, tmp);
    return 1;
}

RB_WRAP(strncasecmp)
{
    int result;
    const char * s1 = luaL_checkstring(L, 1);
    const char * s2 = luaL_checkstring(L, 2);
    if(lua_isnoneornil(L, 3))
        result = rb->strcasecmp(s1, s2);
    else
        result = rb->strncasecmp(s1, s2, (size_t) luaL_checkint(L, 3));

    lua_pushinteger(L, result);
    return 1;
}

static int mem_read_write(lua_State *L, uintptr_t address, size_t maxsize)
{
    intptr_t offset = (intptr_t) luaL_optint(L, 1, 0);
    size_t   size   = (size_t)   luaL_optint(L, 2, maxsize);
    size_t   written;
    int      type   = lua_type(L, 3);

    if(offset < 0)
    {
        /* allows pointer within structure to be calculated offset */
        offset = -(address + offset);
        size   = (size_t) maxsize - offset;
    }

    luaL_argcheck(L, ((uintptr_t) offset) + size <= maxsize,  2, ERR_IDX_RANGE);

    char *mem = (char*) address + ((uintptr_t) offset);
    const void *value = NULL;

    lua_Integer var_luaint;
#ifdef UINT64_MAX
    int64_t  var_64;
#endif
    int32_t  var_32;
    int16_t  var_16;
    int8_t   var_8;
    bool     var_bool;

    switch(type)
    {
        case LUA_TSTRING:
        {
            size_t len;
            const char* str = lua_tolstring (L, 3, &len);

            luaL_argcheck(L, len + 1 <= size,  3, ERR_DATA_OVF);
            size = len + 1; /* include \0 */
            value = str;
            break;
        }
        case LUA_TBOOLEAN:
        {
            var_bool = (bool) lua_toboolean(L, 3);
            value = &var_bool;
            break;
        }
        case LUA_TNUMBER:
        {
            var_luaint = lua_tointeger(L, 3);
            switch(size)
            {
                case sizeof(var_8):
                    var_8 = (int8_t) var_luaint;
                    value = &var_8;
                    break;
                case sizeof(var_16):
                    var_16 = (int16_t) var_luaint;
                    value = &var_16;
                    break;
                case sizeof(var_32):
                    var_32 = (int32_t) var_luaint;
                    value = &var_32;
                    break;
#ifdef UINT64_MAX
                case sizeof(var_64):
                    var_64 = (int64_t) var_luaint;
                    value = &var_64;
                    break;
#endif
            } /* switch size */
            break;
        }
        case  LUA_TNIL:
        case LUA_TNONE: /* reader */
        {
            luaL_Buffer b;
            luaL_buffinit(L, &b);
            while(size > 0)
            {
                written = MIN(LUAL_BUFFERSIZE, size);
                luaL_addlstring (&b, mem, written);
                mem += written;
                size -= written;
            }

            luaL_pushresult(&b);
            return 1;
        }

        default:
            break;
    } /* switch type */

    /* writer */
    luaL_argcheck(L, value != NULL,  3, "Unknown Type");
    rb->memcpy(mem, value, size);
    lua_pushinteger(L, 1);

    return 1;
}

RB_WRAP(global_status)
{
    const uintptr_t address = (uintptr_t) rb->global_status;
    const size_t    maxsize = sizeof(struct system_status);
    return mem_read_write(L, address, maxsize);
}

RB_WRAP(global_settings)
{
    const uintptr_t address = (uintptr_t) rb->global_settings;
    const size_t    maxsize = sizeof(struct user_settings);
    return mem_read_write(L, address, maxsize);
}

RB_WRAP(audio_next_track)
{
    lua_settop(L, 2); /* no writes allowed */
    const uintptr_t address = (uintptr_t) rb->audio_next_track();
    const size_t    maxsize = sizeof(struct mp3entry);
    return mem_read_write(L, address, maxsize);
}

RB_WRAP(audio_current_track)
{
    lua_settop(L, 2); /* no writes allowed */
    const uintptr_t address = (uintptr_t) rb->audio_current_track();
    const size_t    maxsize = sizeof(struct mp3entry);
    return mem_read_write(L, address, maxsize);
}

#define RB_FUNC(func) {#func, rock_##func}
#define RB_ALIAS(name, func) {name, rock_##func}
static const luaL_Reg rocklib[] =
{
    /* Kernel */
    RB_FUNC(current_tick),
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    RB_FUNC(schedule_cpu_boost),
#endif

    RB_FUNC(current_path),

    /* DEVICE INPUT CONTROL */
    RB_FUNC(get_plugin_action),
#ifdef HAVE_TOUCHSCREEN
    RB_FUNC(action_get_touchscreen_press),
    RB_FUNC(touchscreen_mode),
#endif
    RB_FUNC(kbd_input),
    RB_FUNC(gui_syncyesno_run),
    RB_FUNC(do_menu),

    /* DEVICE AUDIO / PLAYLIST CONTROL */
    RB_FUNC(audio),
    RB_FUNC(playlist),
#if CONFIG_CODEC == SWCODEC
    RB_FUNC(pcm),
    RB_FUNC(mixer_frequency),
#endif

    /* DEVICE LIGHTING CONTROL */
    RB_FUNC(backlight_onoff),

    /* Backlight helper */
    RB_FUNC(backlight_force_on),
    RB_FUNC(backlight_use_settings),

#ifdef HAVE_REMOTE_LCD
    RB_FUNC(remote_backlight_force_on),
    RB_FUNC(remote_backlight_use_settings),
#endif

#ifdef HAVE_BUTTON_LIGHT
    RB_FUNC(buttonlight_force_on),
    RB_FUNC(buttonlight_use_settings),
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    RB_FUNC(backlight_brightness_set),
#endif

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    RB_FUNC(buttonlight_brightness_set),
#endif

    /* DEVICE STRING / FILENAME MANIPULATION */
    RB_FUNC(strip_extension),
    RB_FUNC(create_numbered_filename),
    RB_FUNC(utf8encode),
    RB_FUNC(strncasecmp),

    /* ROCKBOX SETTINGS / INFO */
    RB_FUNC(global_status),
    RB_FUNC(global_settings),
    RB_FUNC(audio_next_track),
    RB_FUNC(audio_current_track),

    {NULL, NULL}
};
#undef RB_FUNC
#undef RB_ALIAS

extern const luaL_Reg rocklib_aux[];

/*
 ** Open Rockbox library
 */
LUALIB_API int luaopen_rock(lua_State *L)
{
    luaL_register(L, LUA_ROCKLIBNAME, rocklib);
    luaL_register(L, LUA_ROCKLIBNAME, rocklib_aux);

    static const struct lua_int_reg rlib_const_int[] =
    {
        /* useful integer constants */
        RB_CONSTANT(HZ),

        RB_CONSTANT(LCD_DEPTH),
        RB_CONSTANT(LCD_HEIGHT),
        RB_CONSTANT(LCD_WIDTH),
        RB_CONSTANT(SCREEN_MAIN),
#if LCD_DEPTH > 1
        RB_CONSTANT(LCD_DEFAULT_FG),
        RB_CONSTANT(LCD_DEFAULT_BG),
#endif
#ifdef HAVE_REMOTE_LCD
        RB_CONSTANT(LCD_REMOTE_DEPTH),
        RB_CONSTANT(LCD_REMOTE_HEIGHT),
        RB_CONSTANT(LCD_REMOTE_WIDTH),
        RB_CONSTANT(SCREEN_REMOTE),
#endif

        RB_CONSTANT(FONT_SYSFIXED),
        RB_CONSTANT(FONT_UI),

        RB_CONSTANT(PLAYLIST_INSERT),
        RB_CONSTANT(PLAYLIST_INSERT_LAST),
        RB_CONSTANT(PLAYLIST_INSERT_FIRST),
        RB_CONSTANT(PLAYLIST_INSERT_LAST_SHUFFLED),
        RB_CONSTANT(PLAYLIST_INSERT_SHUFFLED),
        RB_CONSTANT(PLAYLIST_PREPEND),
        RB_CONSTANT(PLAYLIST_REPLACE),

#ifdef HAVE_TOUCHSCREEN
        RB_CONSTANT(TOUCHSCREEN_POINT),
        RB_CONSTANT(TOUCHSCREEN_BUTTON),
#endif

        {NULL, 0}
    };

    static const struct lua_int_reg* rlci = rlib_const_int;
    for (; rlci->name; rlci++) {
        lua_pushinteger(L, rlci->value);
        lua_setfield(L, -2, rlci->name);
    }

    static const struct lua_str_reg rlib_const_str[] =
    {
        /* some useful paths constants */
        RB_STRING_CONSTANT(HOME_DIR),
        RB_STRING_CONSTANT(PLUGIN_DIR),
        RB_STRING_CONSTANT(PLUGIN_APPS_DATA_DIR),
        RB_STRING_CONSTANT(PLUGIN_GAMES_DATA_DIR),
        RB_STRING_CONSTANT(PLUGIN_DATA_DIR),
        RB_STRING_CONSTANT(ROCKBOX_DIR),
        RB_STRING_CONSTANT(VIEWERS_DATA_DIR),
        {NULL,NULL}
    };

    static const struct lua_str_reg* rlcs = rlib_const_str;
    for (; rlcs->name; rlcs++) {
        lua_pushstring(L, rlcs->value);
        lua_setfield(L, -2, rlcs->name);
    }

    return 1;
}

