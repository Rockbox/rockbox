/*
 *	WaveAudioFileWriter.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999,2000 by Florian Bomers <http://www.bomers.de>
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

package org.tritonus.sampled.file;

import java.io.IOException;
import java.util.Arrays;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.AudioOutputStream;
import org.tritonus.share.sampled.file.TAudioFileWriter;
import org.tritonus.share.sampled.file.TDataOutputStream;


/**
 * Class for writing Microsoft(tm) WAVE files
 *
 * @author Florian Bomers
 */
public class WaveAudioFileWriter
extends TAudioFileWriter {

	private static final AudioFileFormat.Type[]	FILE_TYPES =
	    {
	        AudioFileFormat.Type.WAVE
	    };

	private static final int ALL=AudioSystem.NOT_SPECIFIED;

	// IMPORTANT: this array depends on the AudioFormat.match() algorithm which takes
	//            AudioSystem.NOT_SPECIFIED into account !
	private static final AudioFormat[]	AUDIO_FORMATS =
	    {
	        // Encoding, SampleRate, sampleSizeInBits, channels, frameSize, frameRate, bigEndian
	        new AudioFormat(AudioFormat.Encoding.PCM_UNSIGNED, ALL, 8, ALL, ALL, ALL, true),
	        new AudioFormat(AudioFormat.Encoding.PCM_UNSIGNED, ALL, 8, ALL, ALL, ALL, false),
	        new AudioFormat(AudioFormat.Encoding.ULAW, ALL, 8, ALL, ALL, ALL, false),
	        new AudioFormat(AudioFormat.Encoding.ULAW, ALL, 8, ALL, ALL, ALL, true),
	        new AudioFormat(AudioFormat.Encoding.ALAW, ALL, 8, ALL, ALL, ALL, false),
	        new AudioFormat(AudioFormat.Encoding.ALAW, ALL, 8, ALL, ALL, ALL, true),
	        new AudioFormat(AudioFormat.Encoding.PCM_SIGNED, ALL, 16, ALL, ALL, ALL, false),
	        new AudioFormat(AudioFormat.Encoding.PCM_SIGNED, ALL, 24, ALL, ALL, ALL, false),
	        new AudioFormat(AudioFormat.Encoding.PCM_SIGNED, ALL, 32, ALL, ALL, ALL, false),
	        new AudioFormat(WaveTool.GSM0610, ALL, ALL, ALL, ALL, ALL, false),
	        new AudioFormat(WaveTool.GSM0610, ALL, ALL, ALL, ALL, ALL, true),
	    };

	public WaveAudioFileWriter() {
		super(Arrays.asList(FILE_TYPES),
		      Arrays.asList(AUDIO_FORMATS));
	}

	// overwritten for quicker and more accurate check
	protected boolean isAudioFormatSupportedImpl(AudioFormat format,
	        AudioFileFormat.Type fileType) {
		return WaveTool.getFormatCode(format) != WaveTool.WAVE_FORMAT_UNSPECIFIED;
	}


	protected AudioOutputStream getAudioOutputStream(AudioFormat audioFormat,
	        long lLengthInBytes,
	        AudioFileFormat.Type fileType,
	        TDataOutputStream dataOutputStream)	throws IOException {
	            return new WaveAudioOutputStream(audioFormat,
	                                             lLengthInBytes,
	                                             dataOutputStream);
	        }

}

/*** WaveAudioFileWriter.java ***/
