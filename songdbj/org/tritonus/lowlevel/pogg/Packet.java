/*
 *	Packet.java
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

package org.tritonus.lowlevel.pogg;

import org.tritonus.share.TDebug;



/** Wrapper for ogg_packet.
 */
public class Packet
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
	 *	Holds the pointer to ogg_packet
	 *	for the native code.
	 *	This must be long to be 64bit-clean.
	 */
	private long	m_lNativeHandle;



	public Packet()
	{
		if (TDebug.TraceOggNative) { TDebug.out("Packet.<init>(): begin"); }
		int	nReturn = malloc();
		if (nReturn < 0)
		{
			throw new RuntimeException("malloc of ogg_packet failed");
		}
		if (TDebug.TraceOggNative) { TDebug.out("Packet.<init>(): end"); }
	}



	public void finalize()
	{
		// TODO: call free()
		// call super.finalize() first or last?
		// and introduce a flag if free() has already been called?
	}



	private native int malloc();
	public native void free();



	/** Calls ogg_packet_clear().
	 */
	public native void clear();



	/** Accesses packet and bytes.
	 */
	public native byte[] getData();


	/** Accesses b_o_s.
	 */
	public native boolean isBos();


	/** Accesses e_o_s.
	 */
	public native boolean isEos();


	public native long getGranulePos();


	public native long getPacketNo();


	/**	Sets the data in the packet.
	 */
	public native void setData(byte[] abData, int nOffset, int nLength);


	public native void setFlags(boolean bBos, boolean bEos, long lGranulePos,
								long lPacketNo);

	public void setFlags(boolean bBos, boolean bEos, long lGranulePos)
	{
		setFlags(bBos, bEos, lGranulePos, 0);
	}


	private static native void setTrace(boolean bTrace);
}





/*** Packet.java ***/
