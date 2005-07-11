/*
 *	HeaderlessAudioOutputStream.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 by Florian Bomers <http://www.bomers.de>
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

import java.io.IOException;
import javax.sound.sampled.AudioFormat;


/**
 * AudioOutputStream for files without a header; the input is written as it is.
 *
 * @author Florian Bomers
 */

// todo: implement directly AudioOutputStream without using TAudioOutputStream

public class HeaderlessAudioOutputStream extends TAudioOutputStream {

	public HeaderlessAudioOutputStream(AudioFormat audioFormat,
				   long lLength,
				   TDataOutputStream dataOutputStream) {
		super(audioFormat, lLength, dataOutputStream, false);
	}

	protected void writeHeader() throws IOException
	{
	}
}

/*** HeaderlessAudioOutputStream.java ***/
