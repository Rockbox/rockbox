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

public class RockboxKeyboardInput
{
	private String result;
	
    public RockboxKeyboardInput()
    {
        result = null;
    }    

    public void kbd_input(String text) 
    {
        RockboxActivity a = (RockboxActivity) RockboxService.get_instance().get_activity();
        Intent kbd = new Intent(a, KeyboardActivity.class);
        kbd.putExtra("value", text);
        a.waitForActivity(kbd, new HostCallback()
        {
            public void onComplete(int resultCode, Intent data) 
            {
                if (resultCode == Activity.RESULT_OK)
                {
                    result = data.getStringExtra("value");
                }
                else {
                    result = "";
                }
            }
        });
    }
    public String get_result()
    {
        return result;
    }
    
    public boolean is_usable()
    {
        return RockboxService.get_instance().get_activity() != null;
    }
}
