/*
 *	AudioSystemShadow.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999, 2000 by Matthias Pfisterer
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

package org.tritonus.share.sampled;

import java.io.File;
import java.io.OutputStream;
import java.io.IOException;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;

import org.tritonus.share.sampled.file.AudioOutputStream;
import org.tritonus.share.sampled.file.TDataOutputStream;
import org.tritonus.share.sampled.file.TSeekableDataOutputStream;
import org.tritonus.share.sampled.file.TNonSeekableDataOutputStream;
import org.tritonus.sampled.file.AiffAudioOutputStream;
import org.tritonus.sampled.file.AuAudioOutputStream;
import org.tritonus.sampled.file.WaveAudioOutputStream;



/**	Experminatal area for AudioSystem.
 *	This class is used to host features that may become part of the
 *	Java Sound API (In which case they will be moved to AudioSystem).
 */
public class AudioSystemShadow
{
	public static TDataOutputStream getDataOutputStream(File file)
		throws IOException
	{
			return new TSeekableDataOutputStream(file);
	}



	public static TDataOutputStream getDataOutputStream(OutputStream stream)
		throws IOException
	{
			return new TNonSeekableDataOutputStream(stream);
	}



	// TODO: lLengthInBytes actually should be lLengthInFrames (design problem of A.O.S.)
	public static AudioOutputStream getAudioOutputStream(AudioFileFormat.Type type, AudioFormat audioFormat, long lLengthInBytes, TDataOutputStream dataOutputStream)
	{
		AudioOutputStream	audioOutputStream = null;

		if (type.equals(AudioFileFormat.Type.AIFF) ||
			type.equals(AudioFileFormat.Type.AIFF))
		{
			audioOutputStream = new AiffAudioOutputStream(audioFormat, type, lLengthInBytes, dataOutputStream);
		}
		else if (type.equals(AudioFileFormat.Type.AU))
		{
			audioOutputStream = new AuAudioOutputStream(audioFormat, lLengthInBytes, dataOutputStream);
		}
		else if (type.equals(AudioFileFormat.Type.WAVE))
		{
			audioOutputStream = new WaveAudioOutputStream(audioFormat, lLengthInBytes, dataOutputStream);
		}
		return audioOutputStream;
	}



	public static AudioOutputStream getAudioOutputStream(AudioFileFormat.Type type, AudioFormat audioFormat, long lLengthInBytes, File file)
		throws IOException
	{
		TDataOutputStream	dataOutputStream = getDataOutputStream(file);
		AudioOutputStream	audioOutputStream = getAudioOutputStream(type, audioFormat, lLengthInBytes, dataOutputStream);
		return audioOutputStream;
	}



	public static AudioOutputStream getAudioOutputStream(AudioFileFormat.Type type, AudioFormat audioFormat, long lLengthInBytes, OutputStream outputStream)
		throws IOException
	{
		TDataOutputStream	dataOutputStream = getDataOutputStream(outputStream);
		AudioOutputStream	audioOutputStream = getAudioOutputStream(type, audioFormat, lLengthInBytes, dataOutputStream);
		return audioOutputStream;
	}
}


/*** AudioSystemShadow.java ***/
