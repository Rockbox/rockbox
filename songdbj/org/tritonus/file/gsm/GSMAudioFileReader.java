/*
 *	GSMAudioFileReader.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 - 2004 by Matthias Pfisterer
 *  Copyright (c) 2001 by Florian Bomers <http://www.bomers.de>
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

package org.tritonus.sampled.file.gsm;

import java.io.InputStream;
import java.io.IOException;
import java.io.EOFException;

import java.util.HashMap;
import java.util.Map;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.UnsupportedAudioFileException;
import javax.sound.sampled.spi.AudioFileReader;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.TAudioFileFormat;
import org.tritonus.share.sampled.file.TAudioFileReader;



/**	AudioFileReader class for GSM 06.10 data.
	@author Matthias Pfisterer
 */
public class GSMAudioFileReader
extends	TAudioFileReader
{
	private static final int	GSM_MAGIC = 0xD0;
	private static final int	GSM_MAGIC_MASK = 0xF0;

	private static final int	MARK_LIMIT = 1;



	public GSMAudioFileReader()
	{
		super(MARK_LIMIT, true);
	}



	protected AudioFileFormat getAudioFileFormat(InputStream inputStream, long lFileSizeInBytes)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) { TDebug.out("GSMAudioFileReader.getAudioFileFormat(): begin"); }
		int	b0 = inputStream.read();
		if (b0 < 0)
		{
			throw new EOFException();
		}

		/*
		 *	Check for magic number.
		 */
		if ((b0 & GSM_MAGIC_MASK) != GSM_MAGIC)
		{
			throw new UnsupportedAudioFileException("not a GSM stream: wrong magic number");
		}


		/*
		  If the file size is known, we derive the number of frames
		  ('frame size') from it.
		  If the values don't fit into integers, we leave them at
		  NOT_SPECIFIED. 'Unknown' is considered less incorrect than
		  a wrong value.
		*/
		// [fb] not specifying it causes Sun's Wave file writer to write rubbish
		int	nByteSize = AudioSystem.NOT_SPECIFIED;
		int	nFrameSize = AudioSystem.NOT_SPECIFIED;
		Map<String, Object> properties = new HashMap<String, Object>();
		if (lFileSizeInBytes != AudioSystem.NOT_SPECIFIED)
		{
			// the number of GSM frames
			long lFrameSize = lFileSizeInBytes / 33;
			// duration in microseconds
			long lDuration = lFrameSize * 20000;
			properties.put("duration", lDuration);
			if (lFileSizeInBytes <= Integer.MAX_VALUE)
			{
				nByteSize = (int) lFileSizeInBytes;
				nFrameSize = (int) (lFileSizeInBytes / 33);
			}
		}

		Map<String, Object> afProperties = new HashMap<String, Object>();
		afProperties.put("bitrate", 13200L);
		AudioFormat	format = new AudioFormat(
			new AudioFormat.Encoding("GSM0610"),
			8000.0F,
			AudioSystem.NOT_SPECIFIED /* ??? [sample size in bits] */,
			1,
			33,
			50.0F,
			true,	// this value is chosen arbitrarily
			afProperties);
		AudioFileFormat	audioFileFormat =
			new TAudioFileFormat(
				new AudioFileFormat.Type("GSM","gsm"),
				format,
				nFrameSize,
				nByteSize,
				properties);
		if (TDebug.TraceAudioFileReader) { TDebug.out("GSMAudioFileReader.getAudioFileFormat(): end"); }
		return audioFileFormat;
	}
}



/*** GSMAudioFileReader.java ***/

