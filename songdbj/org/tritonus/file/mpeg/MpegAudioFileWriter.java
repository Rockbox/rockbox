/*
 *	MpegAudioFileWriter.java
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

package org.tritonus.sampled.file.mpeg;

import java.util.Arrays;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.THeaderlessAudioFileWriter;



/** Class for writing mpeg files
 *
 * @author Florian Bomers
 */
public class MpegAudioFileWriter extends THeaderlessAudioFileWriter {

	private static final AudioFileFormat.Type[]	FILE_TYPES = {
	    //new AudioFileFormat.Type("MPEG", "mpeg"),
	    // workaround for the fixed extension problem in AudioFileFormat.Type
	    // see org.tritonus.share.sampled.AudioFileTypes.java
	    new AudioFileFormat.Type("MP3", "mp3")
	};

	public static AudioFormat.Encoding MPEG1L3=new AudioFormat.Encoding("MPEG1L3");

	private static final AudioFormat[]	AUDIO_FORMATS = {
	    new AudioFormat(MPEG1L3, ALL, ALL, 1, ALL, ALL, false),
	    new AudioFormat(MPEG1L3, ALL, ALL, 1, ALL, ALL, true),
	    new AudioFormat(MPEG1L3, ALL, ALL, 2, ALL, ALL, false),
	    new AudioFormat(MPEG1L3, ALL, ALL, 2, ALL, ALL, true),
	};

	public MpegAudioFileWriter()
	{
		super(Arrays.asList(FILE_TYPES),
		      Arrays.asList(AUDIO_FORMATS));
		if (TDebug.TraceAudioFileWriter) { TDebug.out("MpegAudioFileWriter.<init>(): begin"); }
		if (TDebug.TraceAudioFileWriter) { TDebug.out("MpegAudioFileWriter.<init>(): end"); }
	}
}


/*** MpegAudioFileWriter.java ***/
