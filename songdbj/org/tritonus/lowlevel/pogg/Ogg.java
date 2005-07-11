/*
 *	Ogg.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 - 2001 by Matthias Pfisterer
 *
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
 *
 */

/*
|<---            this code is formatted to fit into 80 columns             --->|
*/

package org.tritonus.lowlevel.pogg;

import org.tritonus.share.TDebug;


/**	libogg loading.
 */
public class Ogg
{
	private static boolean	sm_bIsLibraryAvailable = false;



	static
	{
		Ogg.loadNativeLibrary();
	}



	public static void loadNativeLibrary()
	{
		if (TDebug.TraceOggNative) { TDebug.out("Ogg.loadNativeLibrary(): begin"); }

		if (! isLibraryAvailable())
		{
			loadNativeLibraryImpl();
		}
		if (TDebug.TraceOggNative) { TDebug.out("Ogg.loadNativeLibrary(): end"); }
	}



	/** Load the native library for ogg vorbis.

	This method actually does the loading of the library.  Unlike
	{@link loadNativeLibrary() loadNativeLibrary()}, it does not
	check if the library is already loaded.

	 */
	private static void loadNativeLibraryImpl()
	{
		if (TDebug.TraceOggNative) { TDebug.out("Ogg.loadNativeLibraryImpl(): loading native library tritonuspvorbis"); }
		try
		{
			System.loadLibrary("tritonuspvorbis");
			// only reached if no exception occures
			sm_bIsLibraryAvailable = true;
		}
		catch (Error e)
		{
			if (TDebug.TraceOggNative ||
			    TDebug.TraceAllExceptions)
			{
				TDebug.out(e);
			}
			// throw e;
		}
		if (TDebug.TraceOggNative) { TDebug.out("Ogg.loadNativeLibraryImpl(): loaded"); }
	}



	/**	Returns whether the libraries are installed correctly.
	 */
	public static boolean isLibraryAvailable()
	{
		return sm_bIsLibraryAvailable;
	}
}



/*** Ogg.java ***/
