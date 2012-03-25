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
import android.app.AlertDialog;
import android.content.DialogInterface;

public class RockboxYesno
{
    private void yesno_display(final String text, final String yes, final String no)
    {
        final Activity c = RockboxService.getInstance().getActivity();

        c.runOnUiThread(new Runnable() {
            public void run()
            {
                new AlertDialog.Builder(c)
                .setTitle(R.string.KbdInputTitle)
                .setIcon(R.drawable.icon)
                .setCancelable(false)
                .setMessage(text)
                .setPositiveButton(yes, new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int whichButton)
                    {
                        put_result(true);
                    }
                })
                .setNegativeButton(no, new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int whichButton)
                    {
                        put_result(false);
                    }
                })
                .show();
            }
        });
    }

    private boolean is_usable()
    {
        return RockboxService.getInstance().getActivity() != null;
    }
    
    private native void put_result(boolean result);
}
