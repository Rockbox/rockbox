package org.rockbox;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

public class YesnoActivity extends Activity 
{
	public void onCreate(Bundle savedInstanceState) 
	{
        super.onCreate(savedInstanceState);
        new AlertDialog.Builder(this)
    		.setTitle(R.string.KbdInputTitle)
    		.setIcon(R.drawable.icon)
    		.setCancelable(false)
    		.setMessage(getIntent().getStringExtra("value"))
    		.setPositiveButton(R.string.Yes, new DialogInterface.OnClickListener() 
    		{
    		    public void onClick(DialogInterface dialog, int whichButton) {
                    setResult(RESULT_OK, getIntent());
                    finish();
                }
            })

            .setNegativeButton(R.string.No, new DialogInterface.OnClickListener() 
            {
                public void onClick(DialogInterface dialog, int whichButton) 
                {
                    setResult(RESULT_CANCELED, getIntent());
                    finish();
                }
            })
            .show();
    }
}
