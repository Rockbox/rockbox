package org.rockbox;

import android.content.Intent;

public interface HostCallback 
{
    public void onComplete(int resultCode, Intent data);
}
