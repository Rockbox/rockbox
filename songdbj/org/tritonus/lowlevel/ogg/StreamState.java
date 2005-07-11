/*
 *	StreamState.java
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


/** Wrapper for ogg_stream_state.
 */
public class StreamState
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
	 *	Holds the pointer to ogg_stream_state
	 *	for the native code.
	 *	This must be long to be 64bit-clean.
	 */
	private long	m_lNativeHandle;



	public StreamState()
	{
		if (TDebug.TraceOggNative) { TDebug.out("StreamState.<init>(): begin"); }
		int	nReturn = malloc();
		if (nReturn < 0)
		{
			throw new RuntimeException("malloc of ogg_stream_state failed");
		}
		if (TDebug.TraceOggNative) { TDebug.out("StreamState.<init>(): end"); }
	}



	public void finalize()
	{
		// TODO: call free()
		// call super.finalize() first or last?
		// and introduce a flag if free() has already been called?
	}



	private native int malloc();
	public native void free();



	/** Calls ogg_stream_init().
	 */
	public native int init(int nSerialNo);

	/** Calls ogg_stream_clear().
	 */
	public native int clear();

	/** Calls ogg_stream_reset().
	 */
	public native int reset();

	/** Calls ogg_stream_destroy().
	 */
	public native int destroy();

	/** Calls ogg_stream_eos().
	 */
	public native boolean isEOSReached();



	/** Calls ogg_stream_packetin().
	 */
	public native int packetIn(Packet packet);


	/** Calls ogg_stream_pageout().
	 */
	public native int pageOut(Page page);


	/** Calls ogg_stream_flush().
	 */
	public native int flush(Page page);


	/** Calls ogg_stream_pagein().
	 */
	public native int pageIn(Page page);


	/** Calls ogg_stream_packetout().
	 */
	public native int packetOut(Packet packet);


	/** Calls ogg_stream_packetpeek().
	 */
	public native int packetPeek(Packet packet);


	private static native void setTrace(boolean bTrace);
}





/*** StreamState.java ***/
