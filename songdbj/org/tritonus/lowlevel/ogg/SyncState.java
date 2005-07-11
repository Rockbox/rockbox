/*
 *	SyncState.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 - 2001 by Matthias Pfisterer
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as published
 *   by the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
|<---            this code is formatted to fit into 80 columns             --->|
*/

package org.tritonus.lowlevel.ogg;

import org.tritonus.share.TDebug;


/** Wrapper for ogg_sync_state.
 */
public class SyncState
{
        static
        {
                Ogg.loadNativeLibrary();
                if (TDebug.TraceOggNative)
                {
                        setTrace(true);
                }
        }


	/**
	 *	Holds the pointer to ogg_sync_state
	 *	for the native code.
	 *	This must be long to be 64bit-clean.
	 */
	private long	m_lNativeHandle;



	public SyncState()
	{
		if (TDebug.TraceOggNative) { TDebug.out("SyncState.<init>(): begin"); }
		int	nReturn = malloc();
		if (nReturn < 0)
		{
			throw new RuntimeException("malloc of ogg_sync_state failed");
		}
		if (TDebug.TraceOggNative) { TDebug.out("SyncState.<init>(): end"); }
	}



	public void finalize()
	{
		// TODO: call free()
		// call super.finalize() first or last?
		// and introduce a flag if free() has already been called?
	}



	private native int malloc();
	public native void free();



	/** Calls ogg_sync_init().
	 */
	public native void init();


	/** Calls ogg_sync_clear().
	 */
	public native void clear();


	/** Calls ogg_sync_reset().
	 */
	public native void reset();


	/** Calls ogg_sync_destroy().
	 */
	public native void destroy();


	/** Calls ogg_sync_buffer()
	    and ogg_sync_wrote().
	*/
	public native int write(byte[] abBuffer, int nBytes);


	/** Calls ogg_sync_pageseek().
	 */
	public native int pageseek(Page page);


	/** Calls ogg_sync_pageout().
	 */
	public native int pageOut(Page page);


	private static native void setTrace(boolean bTrace);
}





/*** SyncState.java ***/
