package org.rockbox;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

public class KeyboardActivity extends Activity 
{
	public void onCreate(Bundle savedInstanceState) 
	{
        super.onCreate(savedInstanceState);
        LayoutInflater inflater=LayoutInflater.from(this);
		View addView=inflater.inflate(R.layout.keyboardinput, null);
		EditText input = (EditText) addView.findViewById(R.id.KbdInput);
        input.setText(getIntent().getStringExtra("value"));
        new AlertDialog.Builder(this)
    		.setTitle(R.string.KbdInputTitle)
    		.setView(addView)
    		.setIcon(R.drawable.icon)
    		.setCancelable(false)
    		.setPositiveButton(R.string.OK, new DialogInterface.OnClickListener() 
    		{
    			public void onClick(DialogInterface dialog, int whichButton) {
    				EditText input = (EditText)((Dialog)dialog)
    				                        .findViewById(R.id.KbdInput);
    				Editable s = input.getText();
    				getIntent().putExtra("value", s.toString());
    				setResult(RESULT_OK, getIntent());
    				finish();
    			}
    		})

    		.setNegativeButton(R.string.Cancel, new DialogInterface.OnClickListener() 
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
