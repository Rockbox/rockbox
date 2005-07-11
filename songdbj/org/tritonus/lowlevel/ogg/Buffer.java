/*
 *	Buffer.java
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


/** Wrapper for oggpack_buffer.
 */
public class Buffer
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
	 *	Holds the pointer to oggpack_buffer
	 *	for the native code.
	 *	This must be long to be 64bit-clean.
	 */
	private long	m_lNativeHandle;



	public Buffer()
	{
		if (TDebug.TraceOggNative) { TDebug.out("Buffer.<init>(): begin"); }
		int	nReturn = malloc();
		if (nReturn < 0)
		{
			throw new RuntimeException("malloc of ogg_page failed");
		}
		if (TDebug.TraceOggNative) { TDebug.out("Buffer.<init>(): end"); }
	}



	public void finalize()
	{
		// TODO: call free()
		// call super.finalize() first or last?
		// and introduce a flag if free() has already been called?
	}



	private native int malloc();
	public native void free();


	/** Calls oggpack_writeinit().
	 */
	public native void writeInit();


	/** Calls oggpack_writetrunc().
	 */
	public native void writeTrunc(int nBits);


	/** Calls oggpack_writealign().
	 */
	public native void writeAlign();


	/** Calls oggpack_writecopy().
	 */
	public native void writeCopy(byte[] abSource, int nBits);


	/** Calls oggpack_reset().
	 */
	public native void reset();


	/** Calls oggpack_writeclear().
	 */
	public native void writeClear();


	/** Calls oggpack_readinit().
	 */
	public native void readInit(byte[] abBuffer, int nBytes);


	/** Calls oggpack_write().
	 */
	public native void write(int nValue, int nBits);


	/** Calls oggpack_look().
	 */
	public native int look(int nBits);


	/** Calls oggpack_look1().
	 */
	public native int look1();


	/** Calls oggpack_adv().
	 */
	public native void adv(int nBits);


	/** Calls oggpack_adv1().
	 */
	public native void adv1();


	/** Calls oggpack_read().
	 */
	public native int read(int nBits);


	/** Calls oggpack_read1().
	 */
	public native int read1();


	/** Calls oggpack_bytes().
	 */
	public native int bytes();


	/** Calls oggpack_bits().
	 */
	public native int bits();


	/** Calls oggpack_get_buffer().
	 */
	public native byte[] getBuffer();


	private static native void setTrace(boolean bTrace);
}



/*** Buffer.java ***/
