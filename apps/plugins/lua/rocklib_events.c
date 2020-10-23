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

/* lua events from rockbox *****************************************************
 * This library allows events to be subscribed / recieved within a lua script
 * most events in rb are synchronous so flags are set and later checked by a
 * secondary thread to make them (semi?) asynchronous.
 *
 * There are a few caveats to be aware of:
 * FIRST, The main lua state is halted till the lua callback(s) are finished
 *     Yielding will not return control to your script from within a callback
 *     Also, subsequent callbacks may be delayed by the code in your lua callback
 * SECOND, You must store the value returned from the event_register function
 *         you might get away with it for a bit but gc will destroy your callback
 *         eventually if you do not store the event
 * THIRD, You only get one cb per event type
 *     ["action", "button", "custom", "playback", "timer"]
 * (Re-registration of an event overwrites the previous one)
 *
 * Usage:
 * possible events =["action", "button", "custom", "playback", "timer"]
 *
 * local ev = rockev.register("event", cb_function, [timeout / flags])
 *    cb_function([id] [, data]) ... end
 *
 *
 * rockev.trigger("event", [true/false], [id])
 * sets an event to triggered,
 * NOTE!, CUSTOM_EVENT must be unset manually
 * id is only passed to callback by custom and playback events
 *
 * rockev.suspend(["event"/nil][true/false]) passing nil suspends all events.
 * stops event from executing, any event before re-enabling will be lost.
 * Passing false, unregistering or re-registering an event will clear the suspend
 *
 * rockev.unregister(evX)
 * Use unregister(evX) to remove an event
 * Unregistering is not necessary before script end, it will be
 * cleaned up on script exit
 *
 *******************************************************************************
 * *
 */

#define LUA_LIB

#define _ROCKCONF_H_ /* We don't need strcmp() etc. wrappers */
#include "lua.h"
#include "lauxlib.h"
#include "plugin.h"
#include "rocklib_events.h"

#define EVENT_METATABLE "event metatable"

#define EVENT_THREAD LUA_ROCKEVENTSNAME ".thread"
#define EV_STACKSZ (DEFAULT_STACK_SIZE * 2)

#define LUA_SUCCESS 0

#define EV_TIMER_TICKS 5 /* timer resolution */
#define EV_TIMER_FREQ ((TIMER_FREQ / HZ) * EV_TIMER_TICKS)
#define EV_TICKS (HZ / 5)

#define ENABLE_LUA_HOOK  (LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT)
#define DISABLE_LUA_HOOK (0)

enum {
    THREAD_ERROR       = 0x0,
/* event & suspend states */
    THREAD_ACTEVENT    = 0x1,
    THREAD_BTNEVENT    = 0x2,
    THREAD_CUSTOMEVENT = 0x4,
    THREAD_PLAYBKEVENT = 0x8,
    THREAD_TIMEREVENT  = 0x10,
  //THREAD_AVAILEVENT  = 0x20,
  //THREAD_AVAILEVENT  = 0x40,
  //THREAD_AVAILEVENT  = 0x80,
    THREAD_EVENT_ALL   = 0xFF,
/* thread states */
  //THREAD_AVAILSTATE  = 0x1,
  //THREAD_AVAILSTATE  = 0x2,
  //THREAD_AVAILSTATE  = 0x4,
  //THREAD_AVAILSTATE  = 0x8,
  //THREAD_AVAILSTATE  = 0x10,
  //THREAD_AVAILSTATE  = 0x20,
    THREAD_QUIT        = 0x40,
  //THREAD_AVAILSTATE  = 0x80,
};

enum {
    ACTEVENT = 0,
    BTNEVENT,
    CUSTOMEVENT,
    PLAYBKEVENT,
    TIMEREVENT,
    EVENT_CT
};

struct cb_data {
    int            cb_ref;
    unsigned long  id;
    void           *data;
    long           next_tick;
    long           ticks;
};

struct thread_status {
    uint8_t event;
    uint8_t suspended;
    uint8_t thread;
    uint8_t unused;
};

struct event_data {
    /* lua */
    lua_State           *L;
    lua_State           *NEWL;
    /* rockbox */
    unsigned int         thread_id;
    long                *event_stack;
    volatile long       *get_tick;
    struct thread_status status;
    /* callbacks */
    struct cb_data      *cb[EVENT_CT];
};

static void set_event_meta(lua_State *L);
static struct event_data ev_data;
static struct mutex rev_mtx SHAREDBSS_ATTR;

static inline uint8_t event_flag(unsigned int event)
{
    return (1UL << event) & 0xFF;
}

static inline bool has_thread_status(uint8_t ev_flag)
{
    return (ev_data.status.thread & ev_flag) == ev_flag;
}

static inline void set_evt(uint8_t ev_flag)
{
    ev_data.status.event |= ev_flag;
}

static inline bool remove_evt(uint8_t ev_flag)
{
    /* returns previous flag status and clears it */
    bool has_flag = (ev_data.status.event & ev_flag) == ev_flag;
    ev_data.status.event &= ~(ev_flag);
    return has_flag;
}

static inline bool is_suspend(uint8_t ev_flag)
{
    return (ev_data.status.suspended & ev_flag) == ev_flag;
}

static void suspend_evt(uint8_t ev_flag, bool suspend)
{
    if(!suspend && !has_thread_status(THREAD_QUIT))
    {
        ev_data.status.suspended &= ~(ev_flag);
        ev_flag = 0;
    }
    remove_evt(ev_flag);
    ev_data.status.suspended |= ev_flag;
}

/* mutex lock and unlock routines allow us to execute the event thread without
 * trashing the lua state on error, yield, or sleep in the callback functions */

static inline void rev_lock_mtx(void)
{
    rb->mutex_lock(&rev_mtx);
}

static inline void rev_unlock_mtx(void)
{
    rb->mutex_unlock(&rev_mtx);
}

static void lua_interrupt_callback(lua_State *L, lua_Debug *ar)
{
    (void) ar;

    rb->yield();

    rev_lock_mtx();
    rev_unlock_mtx(); /* must wait till event thread is done to continue */

    /* if callback error, pass error to the main lua state */
    if (lua_status(ev_data.NEWL) != LUA_SUCCESS)
        luaL_error(L, lua_tostring(ev_data.NEWL, -1));
}

static void lua_interrupt_set(lua_State *L, int hookmask)
{
    lua_Hook hook;
    int count;
    static lua_Hook oldhook = NULL;
    static int oldmask      = 0;
    static int oldcount     = 0;

    if (hookmask == ENABLE_LUA_HOOK)
    {
        hook = lua_gethook(L);
        if (hook == lua_interrupt_callback)
            return; /* our hook is already active */
        /* preserve prior hook */
        oldhook = hook;
        oldmask = lua_gethookmask(L);
        oldcount = lua_gethookcount(L);
        hook = lua_interrupt_callback;
        count = 10;
    }
    else
    {
        hook = oldhook;
        hookmask = oldmask;
        count = oldcount;
    }

    lua_sethook(L, hook, hookmask, count);
}

static int lua_rev_callback(lua_State *L, struct cb_data *evt)
{
    int lua_status = LUA_ERRRUN;

    /* load cb function from lua registry */
    lua_rawgeti(L, LUA_REGISTRYINDEX, evt->cb_ref);

    lua_pushinteger(L, evt->id);
    lua_pushlightuserdata(L, evt->data);

    lua_status = lua_resume(L, 2); /* call the saved function */
    if (lua_status == LUA_YIELD) /* coroutine.yield() disallowed */
        luaL_where(L, 1); /* push error string on stack */

    return lua_status;
}

/* timer interrupt callback */
static void rev_timer_isr(void)
{
    uint8_t ev_flag = 0;
    long curr_tick = *(ev_data.get_tick);
    struct cb_data *evt;

    for (unsigned int i= 0; i < EVENT_CT; i++)
    {
        if (!is_suspend(event_flag(i)))
        {
            evt = ev_data.cb[i];

            if ((i == ACTEVENT || i == BTNEVENT) &&
                (rb->button_queue_count() || rb->button_status() || evt->id))
                    ev_flag |= event_flag(i); /* any buttons ready? */
            else if(evt->ticks > 0 && TIME_AFTER(curr_tick, evt->next_tick))
                ev_flag |= event_flag(i);

        }
    }
    set_evt(ev_flag);
    if (ev_data.status.event) /* any events ready? */
        lua_interrupt_set(ev_data.L, ENABLE_LUA_HOOK);
}

static void event_thread(void)
{
    unsigned long action;
    uint8_t ev_flag;
    unsigned int event;
    struct cb_data *evt;

    while(!has_thread_status(THREAD_QUIT) && lua_status(ev_data.L) == LUA_SUCCESS)
    {
        rev_lock_mtx();

        lua_interrupt_set(ev_data.L, ENABLE_LUA_HOOK);

        for (event = 0; event < EVENT_CT; event++)
        {
            ev_flag = event_flag(event);
            if (!remove_evt(ev_flag) || is_suspend(ev_flag))
                continue; /* check next event */

            evt = ev_data.cb[event];

            switch (event)
            {
                case ACTEVENT:
                    action = get_plugin_action(TIMEOUT_NOBLOCK, true);
                    if (action == ACTION_UNKNOWN)
                        continue; /* check next event */
                    else if (action == ACTION_NONE)
                    {
                        /* only send ACTION_NONE once */
                        if (evt->id == ACTION_NONE || rb->button_status() != 0)
                            continue; /* check next event */
                    }
                    evt->id = action;
                    break;
                case BTNEVENT:
                    evt->id = rb->button_get(false);
                    /* only send BUTTON_NONE once */
                    if (evt->id == BUTTON_NONE)
                        continue; /* check next event */
                    break;
                case CUSTOMEVENT:
                case PLAYBKEVENT:
                case TIMEREVENT:
                    break;

            }

            if (lua_rev_callback(ev_data.NEWL, evt) != LUA_SUCCESS)
            {
                rev_unlock_mtx();
                goto event_error;
            }
            evt->next_tick = *(ev_data.get_tick) + evt->ticks;
        }
        rev_unlock_mtx(); /* we are safe to release back to main lua state */

        do
        {
            lua_interrupt_set(ev_data.L, DISABLE_LUA_HOOK);
            rb->yield();
        } while (!has_thread_status(THREAD_QUIT) && (is_suspend(THREAD_EVENT_ALL)
                                                || !ev_data.status.event));

    }
    rb->yield();
    lua_interrupt_set(ev_data.L, DISABLE_LUA_HOOK);

event_error:

    /* thread is exiting -- clean up */
    rb->timer_unregister();
    rb->thread_exit();

    return;
}

static inline void create_event_thread_ref(struct event_data *ev_data)
{
    lua_State *L = ev_data->L;

    lua_createtable(L, 2, 0);

    ev_data->event_stack = (long *) lua_newuserdata(L, EV_STACKSZ);

    /* attach EVENT_METATABLE to ud so we get notified on garbage collection */
    set_event_meta(L);
    lua_rawseti(L, -2, 1);

    ev_data->NEWL = lua_newthread(L);
    lua_rawseti(L, -2, 2);

    lua_setfield(L, LUA_REGISTRYINDEX, EVENT_THREAD); /* store references */
}

static inline void destroy_event_thread_ref(struct event_data *ev_data)
{
    lua_State *L = ev_data->L;
    ev_data->event_stack = NULL;
    ev_data->NEWL = NULL;
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, EVENT_THREAD); /* free references */
}

static inline void exit_event_thread(struct event_data *ev_data)
{
    suspend_evt(THREAD_EVENT_ALL, true);
    ev_data->status.thread = THREAD_QUIT;

    rb->thread_wait(ev_data->thread_id); /* wait for thread to exit */
    destroy_event_thread_ref(ev_data);

    ev_data->status.thread = 0;
    ev_data->thread_id = UINT_MAX;
}

static void init_event_thread(bool init, struct event_data *ev_data)
{
    if (ev_data->event_stack != NULL) /* make sure we don't double free */
    {
        if (!init && ev_data->thread_id != UINT_MAX)
            exit_event_thread(ev_data);

        return;
    }
    else if (!init)
        return;

    create_event_thread_ref(ev_data);

    ev_data->thread_id = rb->create_thread(&event_thread,
                                           ev_data->event_stack,
                                           EV_STACKSZ,
                                           0,
                                           EVENT_THREAD
                                           IF_PRIO(, PRIORITY_SYSTEM)
                                           IF_COP(, COP));

    /* Timer is used to poll waiting events */
    if (!rb->timer_register(1, NULL, EV_TIMER_FREQ, rev_timer_isr IF_COP(, CPU)))
    {
        rb->splash(100, "No timer available!");
    }
}

static void playback_event_callback(unsigned short id, void *data)
{
    /* playback events are synchronous we need to return ASAP so set a flag */
    struct cb_data *evt = ev_data.cb[PLAYBKEVENT];
    evt->id = id;
    evt->data = data;
    if (!is_suspend(THREAD_PLAYBKEVENT))
    {
        set_evt(THREAD_PLAYBKEVENT);
        lua_interrupt_set(ev_data.L, ENABLE_LUA_HOOK);
    }
}

static void register_playbk_events(int flag_events)
{
    unsigned short i, pb_evt;
    static const unsigned short playback_events[7] =
    {                                         /*flags*/
        PLAYBACK_EVENT_START_PLAYBACK,        /* 0x1 */
        PLAYBACK_EVENT_TRACK_BUFFER,          /* 0x2 */
        PLAYBACK_EVENT_CUR_TRACK_READY,       /* 0x4 */
        PLAYBACK_EVENT_TRACK_FINISH,          /* 0x8 */
        PLAYBACK_EVENT_TRACK_CHANGE,          /* 0x10*/
        PLAYBACK_EVENT_TRACK_SKIP,            /* 0x20*/
        PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE /* 0x40*/
    };

    for(i = 0; i < ARRAYLEN(playback_events); i++)
    {
        pb_evt = playback_events[i];
        if (!(flag_events & (1 << i)))
            rb->remove_event(pb_evt, playback_event_callback);
        else
            rb->add_event(pb_evt, playback_event_callback);
    }
}

static void destroy_event_userdata(lua_State *L, unsigned int event)
{
    uint8_t ev_flag = event_flag(event);
    struct cb_data *evt = ev_data.cb[event];
    suspend_evt(ev_flag, true);
    if (evt != NULL)
        luaL_unref(L, LUA_REGISTRYINDEX, evt->cb_ref);

    ev_data.cb[event] = NULL;
}

static void create_event_userdata(lua_State *L, unsigned int event, int index)
{
    /* if function is already registered , unregister it */
    destroy_event_userdata(L, event);

    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index); /* copy passed lua function on top of stack */
    int ref_lua = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_data.cb[event] = (struct cb_data *)lua_newuserdata(L, sizeof(struct cb_data));

    ev_data.cb[event]->cb_ref = ref_lua; /* store ref for later call/release */

    /* attach EVENT_METATABLE to ud so we get notified on garbage collection */
    set_event_meta(L);
    /* cb_data is on top of stack */
}

static int rockev_gc(lua_State *L) {
    bool has_events = false;
    void *d = (void *) lua_touserdata(L, 1);

    if (d != NULL)
    {
        for (unsigned int i= 0; i < EVENT_CT; i++)
        {
            if (d == ev_data.cb[i] || d == ev_data.event_stack)
            {
                if (i == PLAYBKEVENT)
                    register_playbk_events(0);
                destroy_event_userdata(L, i);
            }
            else if (ev_data.cb[i] != NULL)
                has_events = true;
        }

        if (!has_events) /* nothing to wait for kill thread */
            init_event_thread(false, &ev_data);
    }
  return 0;
}

static void set_event_meta(lua_State *L)
{
    if (luaL_newmetatable(L, EVENT_METATABLE))
    {
        /* set __gc field so we can clean-up our objects */
        lua_pushcfunction(L, rockev_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);
}

static void init_event_data(lua_State *L, struct event_data *ev_data)
{
    /* lua */
    ev_data->L = L;
    /*ev_data->NEWL = NULL;*/
    /* rockbox */
    ev_data->thread_id = UINT_MAX;
    ev_data->get_tick = rb->current_tick;

    ev_data->status.event = 0;
    ev_data->status.suspended = THREAD_EVENT_ALL;
    ev_data->status.thread = 0;

    /*ev_data->event_stack = NULL;*/
    /* callbacks */
    for (unsigned int i= 0; i < EVENT_CT; i++)
        ev_data->cb[i] = NULL;
}

/******************************************************************************
 * LUA INTERFACE **************************************************************
*******************************************************************************
*/
static unsigned int get_event_by_name(lua_State *L)
{
    static const char *const ev_map[EVENT_CT] =
    {
        [ACTEVENT] = "action",
        [BTNEVENT] = "button",
        [CUSTOMEVENT] = "custom",
        [PLAYBKEVENT] = "playback",
        [TIMEREVENT] = "timer",
    };

    return luaL_checkoption(L, 1, NULL, ev_map);
}


static int rockev_register(lua_State *L)
{
    /* register (event, cb [, args] */
    unsigned int event = get_event_by_name(L);
    uint8_t ev_flag = event_flag(event);
    int playbk_events;

    lua_settop(L, 3); /* we need to lock our optional args before...*/
    create_event_userdata(L, event, 2);/* cb_data is on top of stack */
    init_event_thread(!has_thread_status(THREAD_QUIT), &ev_data);

    long event_ticks = 0;
    struct cb_data *evt;

    switch (event)
    {
        case ACTEVENT:
            /* fall through */
        case BTNEVENT:
            event_ticks = 0; /* button events not triggered by timeout */
            break;
        case CUSTOMEVENT:
            event_ticks = luaL_optinteger(L, 3, EV_TICKS);
            ev_flag = 0; /* don't remove suspend */
            break;
        case PLAYBKEVENT: /* see register_playbk_events() for flags */
            event_ticks = 0; /* playback events are not triggered by timeout */
            playbk_events = luaL_optinteger(L, 3, 0x3F);
            register_playbk_events(playbk_events);
            break;
        case TIMEREVENT:
            event_ticks = luaL_checkinteger(L, 3);
            break;
    }

    evt = ev_data.cb[event];
    evt->ticks = event_ticks;
    evt->next_tick = *(ev_data.get_tick) + event_ticks;
    suspend_evt(ev_flag, false);

    return 1; /* returns cb_data */
}

static int rockev_suspend(lua_State *L)
{
    unsigned int event; /*Arg 1 is event, pass nil to suspend all */
    bool suspend = luaL_optboolean(L, 2, true);
    uint8_t ev_flag = THREAD_EVENT_ALL;

    if (!lua_isnoneornil(L, 1))
    {
        event = get_event_by_name(L);
        ev_flag = event_flag(event);
    }

    suspend_evt(ev_flag, suspend);

    /* don't resume invalid events */
    for (unsigned int i = 0; i < EVENT_CT; i++)
    {
        if (ev_data.cb[i] == NULL)
            suspend_evt(event_flag(i), true);
    }

    return 0;
}

static int rockev_trigger(lua_State *L)
{
    unsigned int event = get_event_by_name(L);
    bool enable = luaL_optboolean(L, 2, true);

    uint8_t ev_flag = event_flag(event);
    struct cb_data *evt = ev_data.cb[event];

    /* don't trigger invalid events */
    if (evt != NULL)
    {
        /* allow user to pass an id to some of the callback functions */
        evt->id = luaL_optinteger(L, 3, evt->id);

        if (event == CUSTOMEVENT)
            suspend_evt(ev_flag, !enable);

        if (enable)
            set_evt(ev_flag);
        else
            remove_evt(ev_flag);
    }
    return 0;
}

static int rockev_unregister(lua_State *L)
{
    luaL_checkudata(L, 1, EVENT_METATABLE);
    rockev_gc(L);
    return 0;
}

static const struct luaL_reg evlib[] = {
    {"register",  rockev_register},
    {"suspend", rockev_suspend},
    {"trigger",    rockev_trigger},
    {"unregister", rockev_unregister},
    {NULL, NULL}
};

int luaopen_rockevents(lua_State *L) {
    rb->mutex_init(&rev_mtx);
    init_event_data(L, &ev_data);
    luaL_register(L, LUA_ROCKEVENTSNAME, evlib);
    return 1;
}
