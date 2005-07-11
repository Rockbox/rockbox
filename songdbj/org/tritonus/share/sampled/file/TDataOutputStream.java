/*
 *	TDataOutputStream.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 by Florian Bomers <http://www.bomers.de>
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

package org.tritonus.share.sampled.file;

import java.io.DataOutput;
import java.io.IOException;
import java.io.InputStream;


/**
 * Interface for the file writing classes.
 * <p>Like that it is possible to write to a file without knowing
 * the length before.
 *
 * @author Florian Bomers
 */
public interface TDataOutputStream
extends DataOutput
{
	public boolean supportsSeek();



	public void seek(long position)
		throws IOException;



	public long getFilePointer()
		throws IOException;



	public long length()
		throws IOException;


	public void writeLittleEndian32(int value)
		throws IOException;


	public void writeLittleEndian16(short value)
		throws IOException;

	public void close()
		throws IOException;
}



/*** TDataOutputStream.java ***/
