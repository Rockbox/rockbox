/*
 *	GSMAudioFileWriter.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 by Florian Bomers <http://www.bomers.de>
 *  Copyright (c) 2000 by Matthias Pfisterer
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

package org.tritonus.sampled.file.gsm;

import java.util.Arrays;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.THeaderlessAudioFileWriter;



/**	Class for writing GSM streams
 *
 * @author Florian Bomers
 * @author Matthias Pfisterer
 */
public class GSMAudioFileWriter
extends THeaderlessAudioFileWriter
{
	private static final AudioFileFormat.Type[]	FILE_TYPES =
	{
		new AudioFileFormat.Type("GSM", "gsm")
	};

	private static final AudioFormat[]	AUDIO_FORMATS =
	{
		new AudioFormat(new AudioFormat.Encoding("GSM0610"), 8000.0F, ALL, 1, 33, 50.0F, false),
		new AudioFormat(new AudioFormat.Encoding("GSM0610"), 8000.0F, ALL, 1, 33, 50.0F, true),
	};



	public GSMAudioFileWriter()
	{
		super(Arrays.asList(FILE_TYPES),
		      Arrays.asList(AUDIO_FORMATS));
		if (TDebug.TraceAudioFileWriter) { TDebug.out("GSMAudioFileWriter.<init>(): begin"); }
		if (TDebug.TraceAudioFileWriter) { TDebug.out("GSMAudioFileWriter.<init>(): end"); }
	}
}



/*** GSMAudioFileWriter.java ***/
