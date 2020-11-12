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
#include "lstring.h"

#include "lauxlib.h"
#include "rocklib.h"
#include "lib/helper.h"

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
extern size_t rock_get_allocated_bytes(void); /* tlsf_helper.c */
extern size_t rock_get_unused_bytes(void);

#define RB_WRAP(func) static int rock_##func(lua_State UNUSED_ATTR *L)
#define SIMPLE_VOID_WRAPPER(func) RB_WRAP(func) { (void)L; func(); return 0; }

/* KERNEL */
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

RB_WRAP(sleep)
{
    unsigned ticks = (unsigned) lua_tonumber(L, 1);
    rb->sleep(ticks);
    return 0;
}

#ifdef HAVE_PRIORITY_SCHEDULING
RB_WRAP(thread_set_priority)
{
    unsigned int thread_id = rb->thread_self();
    int priority = (int) luaL_checkint(L, 1);
    int result = rb->thread_set_priority(thread_id, priority);
    lua_pushinteger(L, result);
    return 1;
}
#endif

RB_WRAP(current_path)
{
    return get_current_path(L, 1);
}


/* DEVICE INPUT CONTROL */

RB_WRAP(get_plugin_action)
{
    int timeout = luaL_checkint(L, 1);
    bool with_remote = luaL_optint(L, 2, 0);
    int btn = get_plugin_action(timeout, with_remote);
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

    if(!rb->kbd_input(buffer, LUAL_BUFFERSIZE, NULL))
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

static lua_State* store_luastate(lua_State *L, bool bStore)
{
    /* it is dangerous to store the lua state byond its guaranteed lifetime
       be sure to clear state asap (as in before you exit the calling function) */
    static lua_State *LStored = NULL;
    if(bStore)
        LStored = L;
    return LStored;
}

static int menu_callback(int action,
                         const struct menu_item_ex *this_item,
                         struct gui_synclist *this_list)
{
    (void) this_item;
    (void) this_list;
    static int lua_ref = LUA_NOREF;
    lua_State *L = store_luastate(NULL, false);
    if(!L)
    {
        lua_ref = action;
        action = ACTION_STD_CANCEL;
    }
    else if (lua_ref != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, lua_ref);
        lua_pushnumber(L, action);
        lua_pcall (L, 1, 1, 0);
        action = luaL_optnumber (L, -1, ACTION_STD_CANCEL);
        lua_pop(L, 1);
    }

    return action;
}

RB_WRAP(do_menu)
{
    struct menu_callback_with_desc menu_desc = {NULL, NULL, Icon_NOICON};
    struct menu_item_ex menu = {MT_RETURN_ID | MENU_HAS_DESC, {.strings = NULL},
                                {.callback_and_desc = &menu_desc}};
    int n, start_selected;
    int ref_lua = LUA_NOREF;
    const char **items, *title;

    title = luaL_checkstring(L, 1);

    start_selected = lua_tointeger(L, 3);

    if (lua_isfunction (L, -1))
    {
        /*lua callback function cb(action) return action end */
        ref_lua = luaL_ref(L, LUA_REGISTRYINDEX);
        menu_callback(ref_lua, NULL, NULL);
        store_luastate(L, true);
        menu_desc.menu_callback = &menu_callback;
    }

    /* newuserdata will be pushed onto stack after args*/
    items = get_table_items(L, 2, &n);

    menu.strings = items;
    menu.flags |= MENU_ITEM_COUNT(n);
    menu_desc.desc = (unsigned char*) title;

    int result = rb->do_menu(&menu, &start_selected, NULL, false);

    if (ref_lua != LUA_NOREF)
    {
            store_luastate(NULL, true);
            luaL_unref (L, LUA_REGISTRYINDEX, ref_lua);
            menu_callback(LUA_NOREF, NULL, NULL);
    }

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(splash_scroller)
{
    int timeout = luaL_checkint(L, 1);
    const char *str = luaL_checkstring(L, 2);
    int action = splash_scroller(timeout, str); /*rockaux.c*/
    lua_pushinteger(L, action);
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

    yield();
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

    yield();
    lua_pushinteger(L, status); /* return previous (or current) audio status */
    return 1;
}

RB_WRAP(sound)
{
    enum e_snd {SOUND_SET = 0, SOUND_CURRENT, SOUND_DEFAULT,
                SOUND_MIN, SOUND_MAX, SOUND_UNIT, SOUND_SET_PITCH,
                SOUND_VAL2PHYS, SOUND_ECOUNT};

    const char *snd_option[] = {"set", "current", "default",
                                "min", "max", "unit", "pitch",
                                "val2phys", NULL};

    lua_pushnil(L); /*push nil so options w/o return have something to return */

    int option = luaL_checkoption (L, 1, NULL, snd_option);
    int setting = luaL_checkint(L, 2);
    int value, result;
    switch(option)
    {
        case SOUND_SET:
            value = luaL_checkint(L, 3);
            rb->sound_set(setting, value);
            return 1; /*nil*/
            break;
        case SOUND_CURRENT:
            result = rb->sound_current(setting);
            break;
        case SOUND_DEFAULT:
            result = rb->sound_default(setting);
            break;
        case SOUND_MIN:
            result = rb->sound_min(setting);
            break;
        case SOUND_MAX:
            result = rb->sound_max(setting);
            break;
        case SOUND_UNIT:
            lua_pushstring (L, rb->sound_unit(setting));
            return 1;
            break;
#if defined (HAVE_PITCHCONTROL)
        case SOUND_SET_PITCH:
            rb->sound_set_pitch(setting);
            return 1;/*nil*/
            break;
#endif
        case SOUND_VAL2PHYS:
            value = luaL_checkint(L, 3);
            result = rb->sound_val2phys(setting, value);
            break;

        default:
            return 1;
            break;
    }

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(pcm)
{
    enum e_pcm {PCM_APPLYSETTINGS = 0, PCM_ISPLAYING,
                PCM_PLAYSTOP, PCM_PLAYLOCK, PCM_PLAYUNLOCK,
                PCM_SETFREQUENCY, PCM_ECOUNT};

    const char *pcm_option[] = {"apply_settings", "is_playing",
                                "play_stop", "play_lock", "play_unlock",
                                "set_frequency", NULL};
    bool   b_result;

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
        case PCM_PLAYSTOP:
            rb->pcm_play_stop();
            break;
        case PCM_PLAYLOCK:
            rb->pcm_play_lock();
            break;
        case PCM_PLAYUNLOCK:
            rb->pcm_play_unlock();
            break;
        case PCM_SETFREQUENCY:
            rb->pcm_set_frequency((unsigned int) luaL_checkint(L, 2));
            break;
    }

    yield();
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

#ifdef HAVE_BACKLIGHT
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
#endif /* HAVE_BACKLIGHT */

#ifdef HAVE_BUTTON_LIGHT
SIMPLE_VOID_WRAPPER(buttonlight_force_on);
SIMPLE_VOID_WRAPPER(buttonlight_use_settings);
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

#if 0 /*See files.lua */
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
#endif

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

    /* ROCKBOX SETTINGS / INFO */
static int mem_read_write(lua_State *L, uintptr_t address, size_t maxsize, bool isstr_p)
{
    if(isstr_p) /*pointer to string (**char)*/
    {
        lua_settop(L, 2); /* no writes allowed */
    }
    intptr_t offset = (intptr_t) luaL_optnumber(L, 1, 0);
    size_t   size   = (size_t)   luaL_optnumber(L, 2, maxsize);
    size_t   written;
    int      type   = lua_type(L, 3);

    if(offset < 0)
    {
        /* allows pointer within structure to be calculated offset */
        offset = -(address + offset);
        luaL_argcheck(L, ((size_t) offset) <= maxsize,  1, ERR_IDX_RANGE);
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
            if(isstr_p && mem)
            {
                lua_pushstring (L, *(char**) mem);
                return 1;
            }
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
    /*const bool      isstr_p = lua_toboolean(L, 4);*/
    return mem_read_write(L, address, maxsize, false);
}

RB_WRAP(global_settings)
{
    const uintptr_t address = (uintptr_t) rb->global_settings;
    const size_t    maxsize = sizeof(struct user_settings);
    /*const bool      isstr_p = lua_toboolean(L, 4);*/
    return mem_read_write(L, address, maxsize, false);
}

RB_WRAP(audio_next_track)
{

    const uintptr_t address = (uintptr_t) rb->audio_next_track();
    const size_t    maxsize = sizeof(struct mp3entry);
    const bool      isstr_p = lua_toboolean(L, 4);
    lua_settop(L, 2); /* no writes allowed */
    return mem_read_write(L, address, maxsize, isstr_p);
}

RB_WRAP(audio_current_track)
{

    const uintptr_t address = (uintptr_t) rb->audio_current_track();
    const size_t    maxsize = sizeof(struct mp3entry);
    const bool      isstr_p = lua_toboolean(L, 4);
    lua_settop(L, 2); /* no writes allowed */
    return mem_read_write(L, address, maxsize, isstr_p);
}

RB_WRAP(settings_save)
{
    rb->settings_save();
    return 0;
}

#if 0
RB_WRAP(read_mem)
{
    lua_settop(L, 2); /* no writes allowed */
    const uintptr_t address = lua_tonumber(L, 1);
    const size_t    maxsize = luaL_optnumber(L, 2, strlen((char *)address));
    luaL_argcheck(L, address > 0,  1, ERR_IDX_RANGE);
    lua_pushnil(L);
    lua_replace(L, -3);/* stk pos 1 is no longer offset it is starting address */
    return mem_read_write(L, address, maxsize, false);
}

/* will add this back if anyone finds a target that needs it */
RB_WRAP(system_memory_guard)
{
    int newmode = (int) luaL_checkint(L, 1);
    int result = rb->system_memory_guard(newmode);
    lua_pushinteger(L, result);
    return 1;
}
#endif

/* SPEAKING */
static int rock_talk(lua_State *L)
{
    int result;
    bool enqueue = lua_toboolean(L, 2);
    if (lua_isnumber(L, 1))
    {
        long n = (long) lua_tonumber(L, 1);
        result = rb->talk_number(n, enqueue);
    }
    else
    {
        const char* spell = luaL_checkstring(L, 1);
        result = rb->talk_spell(spell, enqueue);
    }

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(talk_shutup)
{
    if (lua_toboolean(L, 1))
        rb->talk_force_shutup();
    else
        rb->talk_shutup();
    return 0;
}

/* MISC */
RB_WRAP(restart_lua)
{
    /*close lua state, open a new lua state, load script @ filename */
    luaL_checktype (L, 1, LUA_TSTRING);
    lua_settop(L, 1);
    lua_pushlightuserdata(L, L); /* signal exit handler */
    exit(1); /* atexit in rocklua.c */
    return -1;
}

RB_WRAP(show_logo)
{
    rb->show_logo();
    return 0;
}

RB_WRAP(mem_stats)
{
    /* used, allocd, free = rb.mem_stats() */
    /* note free is the high watermark */
    size_t allocd = rock_get_allocated_bytes();
    size_t free = rock_get_unused_bytes();

    lua_pushinteger(L, allocd - free);
    lua_pushinteger(L, allocd);
    lua_pushinteger(L, free);
    return 3;
}

#define RB_FUNC(func) {#func, rock_##func}
#define RB_ALIAS(name, func) {name, rock_##func}
static const luaL_Reg rocklib[] =
{
    /* KERNEL */
    RB_FUNC(current_tick),
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    RB_FUNC(schedule_cpu_boost),
#endif
    RB_FUNC(sleep),
#ifdef HAVE_PRIORITY_SCHEDULING
    RB_FUNC(thread_set_priority),
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
    RB_FUNC(splash_scroller),

    /* DEVICE AUDIO / SOUND / PLAYLIST CONTROL */
    RB_FUNC(audio),
    RB_FUNC(playlist),
    RB_FUNC(sound),
    RB_FUNC(pcm),
    RB_FUNC(mixer_frequency),

#ifdef HAVE_BACKLIGHT
    /* DEVICE LIGHTING CONTROL */
    RB_FUNC(backlight_onoff),

    /* Backlight helper */
    RB_FUNC(backlight_force_on),
    RB_FUNC(backlight_use_settings),

#ifdef HAVE_REMOTE_LCD
    RB_FUNC(remote_backlight_force_on),
    RB_FUNC(remote_backlight_use_settings),
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    RB_FUNC(backlight_brightness_set),
#endif
#endif  /* HAVE_BACKLIGHT */

#ifdef HAVE_BUTTON_LIGHT
    RB_FUNC(buttonlight_force_on),
    RB_FUNC(buttonlight_use_settings),
#endif

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    RB_FUNC(buttonlight_brightness_set),
#endif

    /* DEVICE STRING / FILENAME MANIPULATION */
#if 0 /*See files.lua */
    RB_FUNC(strip_extension),
    RB_FUNC(create_numbered_filename),
#endif
    RB_FUNC(utf8encode),
    RB_FUNC(strncasecmp),

    /* ROCKBOX SETTINGS / INFO */
    RB_FUNC(global_status),
    RB_FUNC(global_settings),
    RB_FUNC(audio_next_track),
    RB_FUNC(audio_current_track),
    RB_FUNC(settings_save),

    /* SPEAKING */
    {"talk_number", rock_talk},
    {"talk_spell", rock_talk},
    RB_FUNC(talk_shutup),

    /* MISC */
    RB_FUNC(restart_lua),
    RB_FUNC(show_logo),
    RB_FUNC(mem_stats),

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
    lua_getglobal(L, "require");
    lua_pushstring(L, "rb_defines");
    if (lua_pcall (L, 1, 0, 0))
        lua_pop(L, 1);
#if 0 /* see rb_defines.lua */
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

/* queue sys events */
        RB_CONSTANT(SYS_USB_CONNECTED),
        RB_CONSTANT(SYS_USB_DISCONNECTED),
        RB_CONSTANT(SYS_TIMEOUT),
        RB_CONSTANT(SYS_POWEROFF),
        RB_CONSTANT(SYS_CHARGER_CONNECTED),
        RB_CONSTANT(SYS_CHARGER_DISCONNECTED),

#ifdef HAVE_TOUCHSCREEN
        RB_CONSTANT(TOUCHSCREEN_POINT),
        RB_CONSTANT(TOUCHSCREEN_BUTTON),
#endif

        {NULL, 0}
    };

    static const struct lua_int_reg* rlci = rlib_const_int;
    for (; rlci->name; rlci++) {
        lua_pushinteger(L, rlci->value);
        luaS_newlloc(L, rlci->name, TSTR_INBIN);
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
        luaS_newlloc(L, rlcs->value, TSTR_INBIN);
        lua_pushstring(L, rlcs->value);
        luaS_newlloc(L, rlcs->name, TSTR_INBIN);
        lua_setfield(L, -2, rlcs->name);
    }
#endif
    return 1;
}
