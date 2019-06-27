/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 William Wilgus
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

/* lua events from rockbox
 * this library allows events to be subscribed / recieved within a lua script
 * most events in rb are synchronous so flags are set and later checked by a
 * secondary thread to make them asynchronous.
 *
 * There are a few caveats to be aware of:
 * First if your lua script is hogging all thread time you will
 * NEVER GET ANY CALLBACKS
 * Second, subsequent callbacks may be delayed by the code in your lua callback
 * Third, you must store the value returned from the event_register function
 * Fourth, you only get one cb per event type (timer, playback, button, action)
 * use event_release(EV) before you register a new lua function,
 * unregistering is not necessary before script end however,
 * it will be cleaned up on script exit
 *
 */


#define LUA_LIB

#define _ROCKCONF_H_ /* We don't need strcmp() etc. wrappers */
#include "lua.h"
#include "lauxlib.h"
#include "plugin.h"
#include "rocklib_events.h"

#define EVENT_METATABLE "event metatable"

struct cb_data {
    int            cb_ref;
    unsigned long  id;
    void           *data;
};

struct event_data {
    lua_State          *L;
    int                thread_state;
    unsigned int       thread_id;
    bool               thread_quit;
    long               *event_stack;
    int                 event_stack_ref;
    struct cb_data     *ad; /* Action event */
    struct cb_data     *bd; /* button event */
    struct cb_data     *pd; /* playback event */
    struct cb_data     *td; /* timer event */
};

enum {
    THREAD_YIELD       = 0x0,
    THREAD_TIMEREVENT  = 0x1,
    THREAD_PLAYBKEVENT = 0x2,
    THREAD_ACTEVENT    = 0x4,
    THREAD_BUTEVENT    = 0x8,
};

static struct event_data ev_data;

static inline void init_event_data(lua_State *L, struct event_data *ev_data)
{
    ev_data->L = L;
    ev_data->thread_state = THREAD_YIELD;
    ev_data->thread_id = UINT_MAX;
    ev_data->thread_quit = false;
    ev_data->event_stack = NULL;
    ev_data->event_stack_ref = LUA_NOREF;
    ev_data->ad = NULL;
    ev_data->bd = NULL;
    ev_data->pd = NULL;
    ev_data->td = NULL;
}

static bool lua_event_callback(lua_State *L, struct cb_data *cbd)
{
    bool is_success = false;
    if(cbd != NULL)
    {
        lua_State *NEWL = lua_newthread(L);
        lua_rawgeti(NEWL, LUA_REGISTRYINDEX, cbd->cb_ref);

        lua_pushinteger(NEWL, cbd->id);
        lua_pushlightuserdata (NEWL, cbd->data);

        if (lua_resume(NEWL, 2) != 0)
        {
            /* display error but don't dump state so we can still clean up */
            rb->splash(3, lua_tostring (NEWL, -1));
        }
        else
            is_success = true;

        lua_pop (L, 1); /* pop new thread from original lua stack */
    }
    return is_success;
}

static void event_thread(void)
{
    while(!ev_data.thread_quit && lua_status(ev_data.L) == 0)
    {
        if ((ev_data.thread_state & THREAD_TIMEREVENT) == THREAD_TIMEREVENT)
        {
            if (!lua_event_callback(ev_data.L, ev_data.td))
                break;

            ev_data.thread_state &= ~THREAD_TIMEREVENT;
        }

        if ((ev_data.thread_state & THREAD_PLAYBKEVENT) == THREAD_PLAYBKEVENT)
        {
            if (!lua_event_callback(ev_data.L, ev_data.pd))
                break;
            ev_data.thread_state &= ~THREAD_PLAYBKEVENT;
        }

        if ((ev_data.thread_state & THREAD_ACTEVENT) == THREAD_ACTEVENT)
        {
            unsigned long action = get_plugin_action(TIMEOUT_NOBLOCK, true);
            if(ev_data.ad->id != action)
            {
                ev_data.ad->id = action;
                if (!lua_event_callback(ev_data.L, ev_data.ad))
                    break;
            }
        }

        if ((ev_data.thread_state & THREAD_BUTEVENT) == THREAD_BUTEVENT)
        {
            ev_data.bd->id = rb->button_get(false);
            if(ev_data.bd->id)
            {
                if (!lua_event_callback(ev_data.L, ev_data.bd))
                    break;
            }
        }

        do
        {
            rb->sleep(0);
        } while(ev_data.thread_state == THREAD_YIELD && !ev_data.thread_quit);
    }

    rb->timer_unregister();
    rb->thread_exit();

    return;
}

static void init_event_thread(bool init, struct event_data *ev_data)
{
    if (ev_data->event_stack_ref != LUA_NOREF) /* make sure we don't double free */
    {
        if (!init && ev_data->thread_id != UINT_MAX)
        {
            ev_data->thread_state = THREAD_YIELD;
            ev_data->thread_quit = true;
            rb->thread_wait(ev_data->thread_id); /* wait for thread to exit */

            ev_data->thread_quit = false;
            ev_data->thread_id = UINT_MAX;
            /* remove stack ref so it can be garbage collected */
            luaL_unref (ev_data->L, LUA_REGISTRYINDEX, ev_data->event_stack_ref);
            ev_data->event_stack_ref = LUA_NOREF;
        }
        return;
    }
    else if (!init)
        return;

    ev_data->event_stack = (long *) lua_newuserdata (ev_data->L, DEFAULT_STACK_SIZE);

    /* attach EVENT_METATABLE to ud so we get notified on garbage collection */
    luaL_getmetatable (ev_data->L, EVENT_METATABLE);
    lua_setmetatable (ev_data->L, -2);
     /* store ud ref to make it persist */
    ev_data->event_stack_ref = luaL_ref(ev_data->L, LUA_REGISTRYINDEX);

    ev_data->thread_id = rb->create_thread(&event_thread,
                                           ev_data->event_stack,
                                           DEFAULT_STACK_SIZE,
                                           0,
                                           "lua_events"
                                           IF_PRIO(, PRIORITY_BACKGROUND)
                                           IF_COP(, COP));
}

/* timer interrupt callback */
static void timer_isr(void)
{
    /* timer events are synchronous we need to return ASAP so set a flag */
    ev_data.thread_state |= THREAD_TIMEREVENT;
}

static void register_playbk_events(int flag_events,
                           void (*handler)(unsigned short id, void *event_data))
{
    if (flag_events > 0)
    {
        if ((flag_events & 0x1) == 0x1)
            rb->add_event(PLAYBACK_EVENT_START_PLAYBACK, handler);
        if ((flag_events & 0x2) == 0x2)
            rb->add_event(PLAYBACK_EVENT_TRACK_BUFFER, handler);
        if ((flag_events & 0x4) == 0x4)
            rb->add_event(PLAYBACK_EVENT_CUR_TRACK_READY, handler);
        if ((flag_events & 0x8) == 0x8)
            rb->add_event(PLAYBACK_EVENT_TRACK_FINISH, handler);
        if ((flag_events & 0x10) == 0x10)
            rb->add_event(PLAYBACK_EVENT_TRACK_CHANGE, handler);
        if ((flag_events & 0x20) == 0x20)
            rb->add_event(PLAYBACK_EVENT_TRACK_SKIP, handler);
        if ((flag_events & 0x40) == 0x40)
            rb->add_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, handler);
    }
    else
    {
        rb->remove_event(PLAYBACK_EVENT_START_PLAYBACK, handler);
        rb->remove_event(PLAYBACK_EVENT_TRACK_BUFFER, handler);
        rb->remove_event(PLAYBACK_EVENT_CUR_TRACK_READY, handler);
        rb->remove_event(PLAYBACK_EVENT_TRACK_FINISH, handler);
        rb->remove_event(PLAYBACK_EVENT_TRACK_CHANGE, handler);
        rb->remove_event(PLAYBACK_EVENT_TRACK_SKIP, handler);
        rb->remove_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, handler);
    }
}

/* playback event callback */
static void playback_events(unsigned short id, void *event_data)
{
    /* playback events are synchronous we need to return ASAP so set a flag */
    ev_data.thread_state |= THREAD_PLAYBKEVENT;
    ev_data.pd->id = id;
    ev_data.pd->data = event_data;
}

static void create_event_userdata(lua_State *L, struct cb_data ** cbdata)
{
    if (!lua_isfunction (L, 1))
    {
        init_event_thread(false, &ev_data);
        luaL_typerror (L, 1, "function");
        return;
    }

    lua_pushvalue (L, 1); /* put passed lua function on top of stack */
    int ref_lua = luaL_ref(L, LUA_REGISTRYINDEX);

    *cbdata = (struct cb_data *) lua_newuserdata (L, sizeof(struct cb_data));

    (*cbdata)->cb_ref = ref_lua;

    /* attach EVENT_METATABLE to ud so we get notified on garbage collection */
    luaL_getmetatable (L, EVENT_METATABLE);
    lua_setmetatable (L, -2);
    /* cb_data is on top of stack */
}

static int rocklib_events_gc(lua_State *L) {

    void *d = (void *) lua_touserdata (L, 1);
    if (d == ev_data.ad)
    {
        ev_data.thread_state &= ~THREAD_ACTEVENT;
        luaL_unref (L, LUA_REGISTRYINDEX, ev_data.ad->cb_ref);
        ev_data.ad = NULL;
    }
    else if (d == ev_data.bd)
    {
        ev_data.thread_state &= ~THREAD_BUTEVENT;
        luaL_unref (L, LUA_REGISTRYINDEX, ev_data.bd->cb_ref);
        ev_data.bd = NULL;
    }
    else if (d == ev_data.td)
    {
        rb->timer_unregister();
        ev_data.thread_state &= ~THREAD_TIMEREVENT;
        luaL_unref (L, LUA_REGISTRYINDEX, ev_data.td->cb_ref);
        ev_data.td = NULL;
    }
    else if (d == ev_data.pd)
    {
        register_playbk_events(0, &playback_events);
        ev_data.thread_state &= ~THREAD_PLAYBKEVENT;
        luaL_unref (L, LUA_REGISTRYINDEX, ev_data.pd->cb_ref);
        ev_data.pd = NULL;
    }
    else if (d == ev_data.event_stack)
    {
        init_event_thread(false, &ev_data); /* thread stack is gc'd kill thread */
    }

    if (ev_data.ad == NULL &&
        ev_data.bd == NULL &&
        ev_data.td == NULL &&
        ev_data.pd == NULL)
    {
        init_event_thread(false, &ev_data); /* nothing to wait for kill thread */
    }

  return 0;
}

static int rock_event_unregister(lua_State *L)
{
    luaL_checkudata (L, 1, EVENT_METATABLE);
    rocklib_events_gc(L);
    lua_pushnil(L);
    return 1;
}

static int rock_action_register(lua_State *L)
{
    create_event_userdata(L, &ev_data.ad);

    init_event_thread(true, &ev_data);
    ev_data.thread_state |= THREAD_ACTEVENT;
    return 1;
}

static int rock_button_register(lua_State *L)
{
    create_event_userdata(L, &ev_data.bd);

    init_event_thread(true, &ev_data);
    ev_data.thread_state |= THREAD_BUTEVENT;
    return 1; /* returns cb_data */
}

static int rock_playbk_register(lua_State *L)
{
    /* see register_playbk_events for flags */
    int flag_events = luaL_optinteger(L, 2, 0x3F);
    create_event_userdata(L, &ev_data.pd);

    init_event_thread(true, &ev_data);

    register_playbk_events(flag_events, &playback_events);

    return 1; /* returns cb_data */
}

static int rock_timer_set_period(lua_State *L)
{
    unsigned int timeout = luaL_checkinteger(L, 1);

    /* it takes time to call lua cb function no point in firing timer faster */
    if (timeout < HZ / 5)
        timeout = HZ / 5;
    rb->timer_set_period((TIMER_FREQ / 1000) * timeout);
    return 0;
}

static int rock_timer_register(lua_State *L)
{
    unsigned int timeout = luaL_checkinteger(L, 2);

    rb->timer_unregister();

    create_event_userdata(L, &ev_data.td);

    /* it takes time to call lua cb function no point in firing timer faster */
    if (timeout < HZ / 5)
        timeout = HZ / 5;

    init_event_thread(true, &ev_data);

    rb->timer_register(0,
                       NULL,
                       (TIMER_FREQ / 1000) * timeout,
                       timer_isr IF_COP(, CPU));

    return 1; /* returns cb_data */
}

/*
** Creates events metatable.
*/
static int event_create_meta (lua_State *L) {
    luaL_newmetatable (L, EVENT_METATABLE);
    /* set its __gc field */
    lua_pushstring (L, "__index");
    lua_newtable(L);
    lua_settable (L, -3);
    lua_pushstring (L, "__gc");
    lua_pushcfunction (L, rocklib_events_gc);
    lua_settable (L, -3);
    return 1;
}

static const struct luaL_reg evlib[] = {
    {"event_unregister", rock_event_unregister},
    {"action_register",  rock_action_register},
    {"button_register",  rock_button_register},
    {"playbk_register",  rock_playbk_register},
    {"timer_register",   rock_timer_register},
    {"timer_set_period", rock_timer_set_period},
    {NULL, NULL}
};

int luaopen_rockevents (lua_State *L) {
    init_event_data(L, &ev_data);
    event_create_meta (L);
    luaL_register (L, LUA_ROCKEVENTSNAME, evlib);
    return 1;
}
