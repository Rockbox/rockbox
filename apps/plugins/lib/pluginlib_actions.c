/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2006 Jonathan Gordon
*
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "action.h"
#include "pluginlib_actions.h"

static struct button_mapping **plugin_context_order;
static int plugin_context_count = 0;
static int last_context = 0; /* index into plugin_context_order 
                                of the last context returned */

const struct button_mapping* get_context_map(int context)
{
    (void)context;
    if (last_context<plugin_context_count)
        return plugin_context_order[last_context++];
    else return NULL;
}

int pluginlib_getaction(struct plugin_api *api,int timeout,
                        const struct button_mapping *plugin_contexts[],
                        int count)
{
    plugin_context_order = (struct button_mapping **)plugin_contexts;
    plugin_context_count = count;
    last_context = 0;
    return api->get_custom_action(CONTEXT_CUSTOM,timeout,get_context_map);
}
