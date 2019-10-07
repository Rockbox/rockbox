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
 * rockev.suspend(["event"/nil][true/false]) passing nil affects all events
 * stops event from executing, any but the last event before
 * re-enabling will be lost. Passing false, unregistering or re-registering
 * an event will clear the suspend
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
#define EV_STACKSZ DEFAULT_STACK_SIZE

#define LUA_SUCCESS 0
#define EV_TIMER_FREQ (TIMER_FREQ / HZ)
#define EV_TICKS (HZ / 5)
#define EV_INPUT (HZ / 4)
//#define DEBUG_EV

enum e_thread_state_flags{
    THREAD_QUIT        = 0x0,
    THREAD_YIELD       = 0x1,
    THREAD_TIMEREVENT  = 0x2,
    THREAD_PLAYBKEVENT = 0x4,
    THREAD_ACTEVENT    = 0x8,
    THREAD_BUTEVENT    = 0x10,
    THREAD_CUSTOMEVENT = 0x20,
  //THREAD_AVAILEVENT  = 0x40,
  //THREAD_AVAILEVENT  = 0x80,
/* thread state holds 3 status items using masks and bitshifts */
    THREAD_STATEMASK   = 0x00FF,
    THREAD_SUSPENDMASK = 0xFF00,
    THREAD_INPUTMASK   = 0xFF0000,
};

enum {
    ACTEVENT = 0,
    BUTEVENT,
    CUSTOMEVENT,
    PLAYBKEVENT,
    TIMEREVENT,
    EVENT_CT
};

static const unsigned char thread_ev_states[EVENT_CT] =
{
    [ACTEVENT] = THREAD_ACTEVENT,
    [BUTEVENT] = THREAD_BUTEVENT,
    [CUSTOMEVENT] = THREAD_CUSTOMEVENT,
    [PLAYBKEVENT] = THREAD_PLAYBKEVENT,
    [TIMEREVENT] = THREAD_TIMEREVENT,
};

static const char *const ev_map[EVENT_CT] =
{
    [ACTEVENT] = "action",
    [BUTEVENT] = "button",
    [CUSTOMEVENT] = "custom",
    [PLAYBKEVENT] = "playback",
    [TIMEREVENT] = "timer",
};

struct cb_data {
    int            cb_ref;
    unsigned long  id;
    void           *data;
};

struct event_data {
    /* lua */
    lua_State      *L;
    lua_State      *NEWL;
    /* rockbox */
    unsigned int    thread_id;
    int             thread_state;
    long           *event_stack;
    long            timer_ticks;
    short           freq_input;
    short           next_input;
    short           next_event;
    /* callbacks */
    struct cb_data *cb[EVENT_CT];
};

static struct event_data ev_data;
static struct mutex rev_mtx SHAREDBSS_ATTR;

#ifdef DEBUG_EV
static int dbg_hook_calls = 0;
#endif

static inline bool has_event(int ev_flag)
{
    return ((THREAD_STATEMASK & (ev_data.thread_state & ev_flag)) == ev_flag);
}

static inline bool is_suspend(int ev_flag)
{
    ev_flag <<= 8;
    return ((THREAD_SUSPENDMASK & (ev_data.thread_state & ev_flag)) == ev_flag);
}

static void init_event_data(lua_State *L, struct event_data *ev_data)
{
    /* lua */
    ev_data->L = L;
    //ev_data->NEWL = NULL;
    /* rockbox */
    ev_data->thread_id = UINT_MAX;
    ev_data->thread_state = THREAD_YIELD | THREAD_SUSPENDMASK;
    //ev_data->event_stack = NULL;
    //ev_data->timer_ticks = 0;
    ev_data->freq_input  = EV_INPUT;
    ev_data->next_input  = EV_INPUT;
    ev_data->next_event  = EV_TICKS;
    /* callbacks */
    for (int i= 0; i < EVENT_CT; i++)
        ev_data->cb[i] = NULL;
}

/* lock and unlock routines allow us to execute the event thread without
 * trashing the lua state on error, yield, or sleep in the callback functions */

static inline void rev_lock_mtx(void)
{
    rb->mutex_lock(&rev_mtx);
}

static inline void rev_unlock_mtx(void)
{
    rb->mutex_unlock(&rev_mtx);
}

static void lua_interrupt_callback( lua_State *L, lua_Debug *ar)
{
    (void) L;
    (void) ar;
#ifdef DEBUG_EV
    dbg_hook_calls++;
#endif

    rb->yield();

    rev_lock_mtx();
    rev_unlock_mtx(); /* must wait till event thread is done to continue */

#ifdef DEBUG_EV
    rb->splashf(0, "spin %d, hooked %d", dbg_hook_calls, (lua_gethookmask(L) != 0));
    unsigned char delay = -1;
    /* we can't sleep or yield without affecting count so lets spin in a loop */
    while(delay > 0) {delay--;}
    if (lua_gethookmask(L) == 0)
        dbg_hook_calls = 0;
#endif

    /* if callback error, pass error to the main lua state */
    if (lua_status(ev_data.NEWL) != LUA_SUCCESS)
        luaL_error (L, lua_tostring (ev_data.NEWL, -1));
}

static void lua_interrupt_set(lua_State *L, bool is_enabled)
{
    const int hookmask = LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT;

    if (is_enabled)
        lua_sethook(L, lua_interrupt_callback, hookmask, 1 );
    else
        lua_sethook(L, NULL, 0, 0 );
}

static int lua_rev_callback(lua_State *L, struct cb_data *cbd)
{
    int lua_status = LUA_ERRRUN;

    if (L != NULL)
    {
        /* load cb function from lua registry */
        lua_rawgeti(L, LUA_REGISTRYINDEX, cbd->cb_ref);

        lua_pushinteger(L, cbd->id);
        lua_pushlightuserdata (L, cbd->data);

        lua_status = lua_resume(L, 2);
        if (lua_status == LUA_YIELD) /* coroutine.yield() disallowed */
            luaL_where(L, 0); /* push error string on stack */
    }
    return lua_status;
}

static void event_thread(void)
{
    unsigned long action;
    int event;
    int ev_flag;

    while(ev_data.thread_state != THREAD_QUIT && lua_status(ev_data.L) == LUA_SUCCESS)
    {
        rev_lock_mtx();
        lua_interrupt_set(ev_data.L, true);

        for (event = 0; event < EVENT_CT; event++)
        {
            ev_flag = thread_ev_states[event];
            if (!has_event(ev_flag) || is_suspend(ev_flag))
                continue; /* check next event */

            ev_data.thread_state &= ~(ev_flag); /* event handled */

            switch (event)
            {
                case ACTEVENT:
                    action = get_plugin_action(TIMEOUT_NOBLOCK, true);
                    if (action == ACTION_UNKNOWN)
                        continue; /* check next event */
                    else if (action == ACTION_NONE)
                    {
                        /* only send ACTION_NONE once */
                        if (ev_data.cb[ACTEVENT]->id == ACTION_NONE ||
                            rb->button_status() != 0)
                            continue; /* check next event */
                    }
                    ev_data.cb[ACTEVENT]->id = action;
                    break;
                case BUTEVENT:
                    ev_data.cb[BUTEVENT]->id = rb->button_get(false);
                    if (ev_data.cb[BUTEVENT]->id == 0)
                        continue; /* check next event */
                    break;
                case CUSTOMEVENT:
                    ev_data.thread_state |= thread_ev_states[CUSTOMEVENT]; // don't reset */
                    break;
                case PLAYBKEVENT:
                    break;
                case TIMEREVENT:
                    ev_data.cb[TIMEREVENT]->id = *rb->current_tick + ev_data.timer_ticks;
                    break;

            }

            if (lua_rev_callback(ev_data.NEWL, ev_data.cb[event]) != LUA_SUCCESS)
            {
                rev_unlock_mtx();
                goto event_error;
            }
        }
        rev_unlock_mtx(); /* we are safe to release back to main lua state */

        do
        {
#ifdef DEBUG_EV
            dbg_hook_calls--;
#endif
            lua_interrupt_set(ev_data.L, false);
            ev_data.next_event = EV_TICKS;
            rb->yield();
        } while(ev_data.thread_state == THREAD_YIELD || is_suspend(THREAD_SUSPENDMASK >> 8));

    }

event_error:

    /* thread is exiting -- clean up */
    rb->timer_unregister();
    //rb->yield();
    rb->thread_exit();

    return;
}

/* timer interrupt callback */
static void rev_timer_isr(void)
{
    if (ev_data.thread_state != THREAD_QUIT || is_suspend(THREAD_SUSPENDMASK >> 8))
    {
        ev_data.next_event--;
        ev_data.next_input--;

        if (ev_data.next_input <=0)
        {
            ev_data.thread_state |= ((ev_data.thread_state & THREAD_INPUTMASK) >> 16);
            ev_data.next_input = ev_data.freq_input;
        }

        if (ev_data.cb[TIMEREVENT] != NULL && !is_suspend(TIMEREVENT))
        {
            if (TIME_AFTER(*rb->current_tick, ev_data.cb[TIMEREVENT]->id))
            {
                ev_data.thread_state |= thread_ev_states[TIMEREVENT];
                ev_data.next_event = 0;
            }
        }

        if (ev_data.next_event <= 0)
            lua_interrupt_set(ev_data.L, true);
    }
}

static void create_event_thread_ref(struct event_data *ev_data)
{
    lua_State *L = ev_data->L;

    lua_createtable(L, 2, 0);

    ev_data->event_stack = (long *) lua_newuserdata (L, EV_STACKSZ);

    /* attach EVENT_METATABLE to ud so we get notified on garbage collection */
    luaL_getmetatable (L, EVENT_METATABLE);
    lua_setmetatable (L, -2);
    lua_rawseti(L, -2, 1);

    ev_data->NEWL = lua_newthread(L);
    lua_rawseti(L, -2, 2);

    lua_setfield (L, LUA_REGISTRYINDEX, EVENT_THREAD); /* store references */
}

static void destroy_event_thread_ref(struct event_data *ev_data)
{
    lua_State *L = ev_data->L;
    ev_data->event_stack = NULL;
    ev_data->NEWL = NULL;
    lua_pushnil(L);
    lua_setfield (L, LUA_REGISTRYINDEX, EVENT_THREAD); /* free references */
}

static void exit_event_thread(struct event_data *ev_data)
{
    ev_data->thread_state = THREAD_QUIT;
    rb->thread_wait(ev_data->thread_id); /* wait for thread to exit */
}

static void init_event_thread(bool init, struct event_data *ev_data)
{
    if (ev_data->event_stack != NULL) /* make sure we don't double free */
    {
        if (!init && ev_data->thread_id != UINT_MAX)
        {
            ev_data->thread_state |= THREAD_SUSPENDMASK; /* suspend all events */
            rb->yield();
            exit_event_thread(ev_data);
            destroy_event_thread_ref(ev_data);
            lua_interrupt_set(ev_data->L, false);
            ev_data->thread_state = THREAD_YIELD | THREAD_SUSPENDMASK;
            ev_data->thread_id = UINT_MAX;
        }
        return;
    }
    else if (!init)
        return;

    create_event_thread_ref(ev_data);
    if (ev_data->NEWL == NULL || ev_data->event_stack == NULL)
        return;

    ev_data->thread_id = rb->create_thread(&event_thread,
                                           ev_data->event_stack,
                                           EV_STACKSZ,
                                           0,
                                           EVENT_THREAD
                                           IF_PRIO(, PRIORITY_SYSTEM)
                                           IF_COP(, COP));

    /* Timer is used to poll waiting events */
    rb->timer_register(0, NULL, EV_TIMER_FREQ, rev_timer_isr IF_COP(, CPU));
    ev_data->thread_state &= ~THREAD_SUSPENDMASK;
}

static void playback_event_callback(unsigned short id, void *data)
{
    /* playback events are synchronous we need to return ASAP so set a flag */
    if (ev_data.thread_state != THREAD_QUIT && !is_suspend(THREAD_SUSPENDMASK >> 8))
    {
        ev_data.thread_state |= thread_ev_states[PLAYBKEVENT];
        ev_data.cb[PLAYBKEVENT]->id = id;
        ev_data.cb[PLAYBKEVENT]->data = data;
        lua_interrupt_set(ev_data.L, true);
    }
}

static void register_playbk_events(int flag_events,
                           void (*handler)(unsigned short id, void *data))
{
    long unsigned int i = 0;
    const unsigned short playback_events[7] =
    {                                         /*flags*/
        PLAYBACK_EVENT_START_PLAYBACK,        /* 0x1 */
        PLAYBACK_EVENT_TRACK_BUFFER,          /* 0x2 */
        PLAYBACK_EVENT_CUR_TRACK_READY,       /* 0x4 */
        PLAYBACK_EVENT_TRACK_FINISH,          /* 0x8 */
        PLAYBACK_EVENT_TRACK_CHANGE,          /* 0x10*/
        PLAYBACK_EVENT_TRACK_SKIP,            /* 0x20*/
        PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE /* 0x40*/
    };

    for(; i < ARRAYLEN(playback_events); i++, flag_events >>= 1)
    {
        if (flag_events == 0) /* remove events */
            rb->remove_event(playback_events[i], handler);
        else /* add events */
            if ((flag_events & 0x1) == 0x1)
                rb->add_event(playback_events[i], handler);
    }
}

static void destroy_event_userdata(lua_State *L, int event)
{
    if (ev_data.cb[event] != NULL)
    {
        int ev_flag = thread_ev_states[event];
        ev_data.thread_state &=  ~(ev_flag | (ev_flag << 8) | (ev_flag << 16));

        luaL_unref (L, LUA_REGISTRYINDEX, ev_data.cb[event]->cb_ref);
        ev_data.cb[event] = NULL;
    }
}

static void create_event_userdata(lua_State *L, int event, int index)
{
    /* if function is already registered , unregister it */
    destroy_event_userdata(L, event);

    if (!lua_isfunction (L, index))
    {
        init_event_thread(false, &ev_data);
        luaL_typerror (L, index, "function");
        return;
    }

    lua_pushvalue (L, index); /* copy passed lua function on top of stack */
    int ref_lua = luaL_ref(L, LUA_REGISTRYINDEX);

    ev_data.cb[event] = (struct cb_data *)lua_newuserdata(L, sizeof(struct cb_data));

    ev_data.cb[event]->cb_ref = ref_lua; /* store ref for later call/release */

    /* attach EVENT_METATABLE to ud so we get notified on garbage collection */
    luaL_getmetatable (L, EVENT_METATABLE);
    lua_setmetatable (L, -2);
    /* cb_data is on top of stack */
}

static int rockev_gc(lua_State *L) {
    bool has_events = false;
    void *d = (void *) lua_touserdata (L, 1);

    if (d == NULL)
    {
        return 0;
    }
    else if (d == ev_data.event_stack) /* thread stack is gc'd kill thread */
    {
        init_event_thread(false, &ev_data);
    }
    else if (d == ev_data.cb[PLAYBKEVENT])
    {
        register_playbk_events(0, &playback_event_callback);
    }

    for( int i= 0; i < EVENT_CT; i++)
    {
        if (d == ev_data.cb[i])
            destroy_event_userdata(L, i);
        else if (ev_data.cb[i] != NULL)
            has_events = true;
    }

    if (!has_events) /* nothing to wait for kill thread */
        init_event_thread(false, &ev_data);

  return 0;
}

/******************************************************************************
 * LUA INTERFACE **************************************************************
*******************************************************************************
*/

static int rockev_register(lua_State *L)
{
    if (ev_data.thread_state == THREAD_QUIT)
        return 0;

    int event = luaL_checkoption(L, 1, NULL, ev_map);
    int ev_flag = thread_ev_states[event];
    int playbk_events;

    lua_settop (L, 3); /* we need to lock our optional args before...*/
    create_event_userdata(L, event, 2);/* cb_data is on top of stack */

    switch (event)
    {
        case ACTEVENT:
            /* fall through */
        case BUTEVENT:
            ev_data.freq_input = luaL_optinteger(L, 3, EV_INPUT);
            if (ev_data.freq_input < HZ / 20) ev_data.freq_input = HZ / 20;
            ev_data.thread_state |= (ev_flag | (ev_flag << 16));
            break;
        case CUSTOMEVENT:
            break;
        case PLAYBKEVENT:
            /* see register_playbk_events() for flags */
            playbk_events = luaL_optinteger(L, 3, 0x3F);
            register_playbk_events(playbk_events, &playback_event_callback);
            break;
        case TIMEREVENT:
            ev_data.timer_ticks = luaL_checkinteger(L, 3);
            ev_data.cb[TIMEREVENT]->id = *rb->current_tick + ev_data.timer_ticks;
            break;
    }

    init_event_thread(true, &ev_data);

    return 1; /* returns cb_data */
}

static int rockev_suspend(lua_State *L)
{
    if (ev_data.thread_state == THREAD_QUIT)
        return 0;

    int event; /*Arg 1 is event pass nil to suspend all */
    bool suspend = luaL_optboolean(L, 2, true);
    int ev_flag = THREAD_SUSPENDMASK;

    if (!lua_isnoneornil(L, 1))
    {
        event = luaL_checkoption(L, 1, NULL, ev_map);
        ev_flag = thread_ev_states[event] << 8;
    }

    if (suspend)
        ev_data.thread_state |= ev_flag;
    else
        ev_data.thread_state &= ~(ev_flag);

    return 0;
}

static int rockev_trigger(lua_State *L)
{
    int event = luaL_checkoption(L, 1, NULL, ev_map);
    bool enable = luaL_optboolean(L, 2, true);

    int ev_flag;

    /* protect from invalid events */
    if (ev_data.cb[event] != NULL && ev_data.thread_state != THREAD_QUIT)
    {
        ev_flag = thread_ev_states[event];
        /* allow user to pass an id to some of the callback functions */
        ev_data.cb[event]->id = luaL_optinteger(L, 3, ev_data.cb[event]->id);

        if (enable)
            ev_data.thread_state |= ev_flag;
        else
            ev_data.thread_state &= ~(ev_flag);
    }
    return 0;
}

static int rockev_unregister(lua_State *L)
{
    luaL_checkudata (L, 1, EVENT_METATABLE);
    rockev_gc(L);
    lua_pushnil(L);
    return 1;
}
/*
** Creates events metatable.
*/
static int event_create_meta (lua_State *L) {
    luaL_newmetatable (L, EVENT_METATABLE);
    /* set __gc field so we can clean-up our objects */
    lua_pushcfunction (L, rockev_gc);
    lua_setfield (L, -2, "__gc");
    return 1;
}

static const struct luaL_reg evlib[] = {
    {"register",  rockev_register},
    {"suspend", rockev_suspend},
    {"trigger",    rockev_trigger},
    {"unregister", rockev_unregister},
    {NULL, NULL}
};

int luaopen_rockevents (lua_State *L) {
    rb->mutex_init(&rev_mtx);
    init_event_data(L, &ev_data);
    event_create_meta (L);
    luaL_register (L, LUA_ROCKEVENTSNAME, evlib);
    return 1;
}
