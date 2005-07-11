/*
 *	Page.java
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



/** Wrapper for ogg_page.
 */
public class Page
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
	 *	Holds the pointer to ogg_page
	 *	for the native code.
	 *	This must be long to be 64bit-clean.
	 */
	private long	m_lNativeHandle;



	public Page()
	{
		if (TDebug.TraceOggNative) { TDebug.out("Page.<init>(): begin"); }
		int	nReturn = malloc();
		if (nReturn < 0)
		{
			throw new RuntimeException("malloc of ogg_page failed");
		}
		if (TDebug.TraceOggNative) { TDebug.out("Page.<init>(): end"); }
	}



	public void finalize()
	{
		// TODO: call free()
		// call super.finalize() first or last?
		// and introduce a flag if free() has already been called?
	}



	private native int malloc();
	public native void free();


	/** Calls ogg_page_version().
	 */
	public native int getVersion();

	/** Calls ogg_page_continued().
	 */
	public native boolean isContinued();

	/** Calls ogg_page_packets().
	 */
	public native int getPackets();

	/** Calls ogg_page_bos().
	 */
	public native boolean isBos();

	/** Calls ogg_page_eos().
	 */
	public native boolean isEos();

	/** Calls ogg_page_granulepos().
	 */
	public native long getGranulePos();

	/** Calls ogg_page_serialno().
	 */
	public native int getSerialNo();

	/** Calls ogg_page_pageno().
	 */
	public native int getPageNo();

	/** Calls ogg_page_checksum_set().
	 */
	public native void setChecksum();


	public native byte[] getHeader();

	public native byte[] getBody();


	private static native void setTrace(boolean bTrace);
}





/*** Page.java ***/
