/*
 *	THeaderlessAudioFileWriter.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 by Florian Bomers <http://www.bomers.de>
 *  Copyright (c) 2000 - 2002 by Matthias Pfisterer
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

package org.tritonus.share.sampled.file;

import java.io.IOException;
import java.util.Collection;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;

import org.tritonus.share.TDebug;



/**	Base class for formats without extra header.
	This AudioFileWriter is typically used for compressed formats
	where the encoder puts a header into the encoded stream. In this
	case, the AudioFileWriter needs not to add a header. This is why
	THeaderlessAudioOutputStream is used here.

	@author Florian Bomers
	@author Matthias Pfisterer
*/
public class THeaderlessAudioFileWriter
extends TAudioFileWriter
{
	protected THeaderlessAudioFileWriter(Collection<AudioFileFormat.Type> fileTypes,
										 Collection<AudioFormat> audioFormats)
	{
		super(fileTypes, audioFormats);
		if (TDebug.TraceAudioFileWriter) { TDebug.out("THeaderlessAudioFileWriter.<init>(): begin"); }
		if (TDebug.TraceAudioFileWriter) { TDebug.out("THeaderlessAudioFileWriter.<init>(): end"); }
	}



	protected AudioOutputStream getAudioOutputStream(
		AudioFormat audioFormat,
		long lLengthInBytes,
		AudioFileFormat.Type fileType,
		TDataOutputStream dataOutputStream)
		throws IOException
	{
		if (TDebug.TraceAudioFileWriter) { TDebug.out("THeaderlessAudioFileWriter.getAudioOutputStream(): begin"); }
		AudioOutputStream	aos = new HeaderlessAudioOutputStream(
			audioFormat,
			lLengthInBytes,
			dataOutputStream);
		if (TDebug.TraceAudioFileWriter) { TDebug.out("THeaderlessAudioFileWriter.getAudioOutputStream(): end"); }
		return aos;
	}

}



/*** THeaderlessAudioFileWriter.java ***/
