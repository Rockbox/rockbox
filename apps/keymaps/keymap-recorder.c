/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Antoine Cellerier <dionoea @t videolan d.t org>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* *
 * Button Code Definitions for archos recorder target
 *
 * \TODO handle F3
 */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|1 = the standard list/tree defines (without directions)


*/

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_OK,          BUTTON_ON,                  BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT },

    { ACTION_STD_OK,          BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_STD_MENU,        BUTTON_F1,                  BUTTON_NONE },
    { ACTION_STD_QUICKSCREEN, BUTTON_F2,                  BUTTON_NONE },
    { ACTION_STD_CONTEXT,     BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_STD_CANCEL,      BUTTON_OFF,                 BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_F3,      BUTTON_F3,                BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

static const struct button_mapping button_context_wps[]  = {
    { ACTION_NONE,            BUTTON_ON,                  BUTTON_NONE },
    { ACTION_WPS_PLAY,        BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_WPS_SKIPNEXT,    BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,    BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,     BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,    BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_STOP,        BUTTON_OFF|BUTTON_REL,      BUTTON_OFF },
    { ACTION_WPS_VOLDOWN,     BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_UP,                  BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_MENU,        BUTTON_F1|BUTTON_REL,       BUTTON_F1 },
    { ACTION_WPS_CONTEXT,     BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_WPS_QUICKSCREEN, BUTTON_F2,                  BUTTON_NONE },
    { ACTION_WPS_BROWSE,      BUTTON_ON|BUTTON_REL,       BUTTON_ON   },
    { ACTION_WPS_ID3SCREEN,   BUTTON_F1|BUTTON_ON,        BUTTON_NONE },
    { ACTION_WPS_PITCHSCREEN, BUTTON_ON|BUTTON_UP,        BUTTON_ON },
    { ACTION_WPS_PITCHSCREEN, BUTTON_ON|BUTTON_DOWN,      BUTTON_ON },
    { ACTION_STD_KEYLOCK,     BUTTON_F1|BUTTON_DOWN,      BUTTON_NONE },
    { ACTION_F3,      BUTTON_F3,                BUTTON_NONE },
    { ACTION_WPS_ABSETB_NEXTDIR, BUTTON_ON|BUTTON_RIGHT,  BUTTON_NONE },
    { ACTION_WPS_ABSETA_PREVDIR, BUTTON_ON|BUTTON_LEFT,   BUTTON_NONE },
    { ACTION_WPS_ABRESET,        BUTTON_ON|BUTTON_OFF,    BUTTON_ON },
    

    LAST_ITEM_IN_LIST
};

static const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,        BUTTON_LEFT,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,              BUTTON_ON|BUTTON_REL,                   BUTTON_ON },
    { ACTION_TREE_STOP,             BUTTON_OFF,                             BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_ON,                              BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,         BUTTON_ON|BUTTON_UP,                    BUTTON_ON },
    { ACTION_LISTTREE_PGUP,         BUTTON_UP|BUTTON_REL,                   BUTTON_ON|BUTTON_UP },
    { ACTION_LISTTREE_PGUP,         BUTTON_ON|BUTTON_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_ON|BUTTON_DOWN,                  BUTTON_ON },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_DOWN|BUTTON_REL,                 BUTTON_ON|BUTTON_DOWN },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_ON|BUTTON_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_listtree */


static const struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|1),
};

static const struct button_mapping button_context_yesno[] = {
    { ACTION_YESNO_ACCEPT,     BUTTON_PLAY,    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; 
static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_UP,                      BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_PLAY,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_F1,                  BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_ON,                  BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_OFF,                 BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,             BUTTON_PLAY,                BUTTON_NONE },
    { ACTION_REC_F2,                BUTTON_F2,                  BUTTON_NONE },
    { ACTION_REC_F3,                BUTTON_F3,                  BUTTON_NONE },
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                           BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,             BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                          BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_ON|BUTTON_LEFT,                 BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_ON|BUTTON_LEFT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_ON|BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_ON|BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_PLAY,                           BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_F1,                             BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_F2,                             BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_OFF,                            BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_F3,                             BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_F3|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                             BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                           BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,             BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_PLAY|BUTTON_ON,   BUTTON_PLAY },
    { ACTION_BMS_DELETE,      BUTTON_PLAY|BUTTON_ON,   BUTTON_ON },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
    
}; /* button_context_settings_bmark */

static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,        BUTTON_F1,                             BUTTON_NONE },
    { ACTION_FM_PRESET,      BUTTON_F2,                             BUTTON_NONE },
    { ACTION_FM_RECORD,      BUTTON_F3,                             BUTTON_NONE },
    { ACTION_FM_FREEZE,      BUTTON_PLAY,                           BUTTON_NONE },
    { ACTION_FM_STOP,        BUTTON_OFF,                            BUTTON_NONE },
    { ACTION_FM_MODE,        BUTTON_ON | BUTTON_REPEAT,             BUTTON_ON },
    { ACTION_FM_EXIT,        BUTTON_ON | BUTTON_REL,                BUTTON_ON },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
    
};

#if BUTTON_REMOTE != 0
/*****************************************************************************
 *    Remote control mappings
 *****************************************************************************/

static const struct button_mapping remote_button_context_standard[]  = {
    { ACTION_STD_PREV,      BUTTON_RC_LEFT,     BUTTON_NONE },
    { ACTION_STD_NEXT,      BUTTON_RC_RIGHT,    BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_RC_STOP,     BUTTON_NONE },
    { ACTION_STD_OK,        BUTTON_RC_PLAY,     BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

static const struct button_mapping remote_button_context_wps[]  = {
    { ACTION_WPS_PLAY,      BUTTON_RC_PLAY,     BUTTON_NONE },
    { ACTION_WPS_SKIPNEXT,  BUTTON_RC_RIGHT,    BUTTON_NONE },
    { ACTION_WPS_SKIPPREV,  BUTTON_RC_LEFT,     BUTTON_NONE },
    { ACTION_WPS_STOP,      BUTTON_RC_STOP,     BUTTON_NONE },
    
    { ACTION_WPS_VOLDOWN,   BUTTON_RC_VOL_DOWN, BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_RC_VOL_UP,   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};


static const struct button_mapping* get_context_mapping_remote( int context )
{
    context ^= CONTEXT_REMOTE;
    
    switch (context)
    {
        case CONTEXT_WPS:
            return remote_button_context_wps;

        default:
            return remote_button_context_standard;
    }
}
#endif /* BUTTON_REMOTE != 0 */

const struct button_mapping* get_context_mapping( int context )
{
#if BUTTON_REMOTE != 0
    if (context&CONTEXT_REMOTE)
        return get_context_mapping_remote(context);
#endif
    
    switch( context )
    {
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS:
            return button_context_settings;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;

        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_tree_scroll_lr;
            /* else fall through to CUSTOM|1 */
        case CONTEXT_CUSTOM|1:
            return button_context_tree;

        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;

        case CONTEXT_RECSCREEN:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_recscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
        case CONTEXT_FM:
            return button_context_radio;

        case CONTEXT_STD:
        case CONTEXT_LIST:
        default:
            return button_context_standard;
    }
}
