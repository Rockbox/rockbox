/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Jonathan Gordon
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

package org.rockbox;

import android.app.Activity;
import android.content.Intent;

public class RockboxYesno
{
	private boolean result;
	private boolean have_result;
	
    public RockboxYesno()
    {
    	have_result = false;
    }    

    public void yesno_display(String text) 
    {
        RockboxActivity a = (RockboxActivity) RockboxService.get_instance().get_activity();
        Intent kbd = new Intent(a, YesnoActivity.class);
        kbd.putExtra("value", text);
        a.waitForActivity(kbd, new HostCallback()
        {
            public void onComplete(int resultCode, Intent data) 
            {
                if (resultCode == Activity.RESULT_OK)
                {
                    result = true;
                    have_result = true;
                }
                else {
                    result = false;
                    have_result = true;
                }
            }
        });
    }

    public boolean result_ready()
    {
        return have_result;
    }
    public boolean get_result()
    {
        return result;
    }
    
    public boolean is_usable()
    {
        return RockboxService.get_instance().get_activity() != null;
    }
}
