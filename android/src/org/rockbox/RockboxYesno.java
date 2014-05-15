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
    private class Listener implements DialogInterface.OnClickListener, DialogInterface.OnDismissListener
    {
        /* default to "No" if onClick isn't called at all */
        private boolean result = false;
        
        /* called when the user presses the Yes or the No button */
        @Override
        public void onClick(DialogInterface dialog, int which)
        {
            result = (which == DialogInterface.BUTTON_POSITIVE);
        }

        /* onDismiss is (hopefully) also called when the dialog is closed
         * for any reason other than clicking yes or no. This should
         * avoid the native code waiting for the result indefinitely in
         * case the dialog just disappears  */
        @Override
        public void onDismiss(DialogInterface dialog)
        {
            put_result(result);
        }
    }
    
    private void yesno_display(final String text, final String yes, final String no)
    {
        final Activity c = RockboxService.getInstance().getActivity();

        c.runOnUiThread(new Runnable() {
            public void run()
            {
                Listener l = new Listener();
                new AlertDialog.Builder(c)
                .setTitle(R.string.YesNoTitle)
                .setIcon(R.drawable.icon)
                .setMessage(text)
                .setPositiveButton(yes, l)
                .setNegativeButton(no, l)
                .show()
                .setOnDismissListener(l);
            }
        });
    }

    private boolean is_usable()
    {
        return RockboxService.getInstance().getActivity() != null;
    }
    
    private native void put_result(boolean result);
}
