/*
 *	AuTool.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000,2001 by Florian Bomers <http://www.bomers.de>
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

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioSystem;


/** Common constants and methods for handling au files.
 *
 * @author Florian Bomers
 */

public class AuTool {

	public static final int	AU_HEADER_MAGIC = 0x2e736e64;
	public static final int	AUDIO_UNKNOWN_SIZE = -1;

	// length of header in bytes
	public static final int		DATA_OFFSET = 24;

	public static final int	SND_FORMAT_UNSPECIFIED = 0;
	public static final int	SND_FORMAT_MULAW_8 = 1;
	public static final int	SND_FORMAT_LINEAR_8 = 2;
	public static final int	SND_FORMAT_LINEAR_16 = 3;
	public static final int	SND_FORMAT_LINEAR_24 = 4;
	public static final int	SND_FORMAT_LINEAR_32 = 5;
	public static final int	SND_FORMAT_FLOAT = 6;
	public static final int	SND_FORMAT_DOUBLE = 7;
	public static final int	SND_FORMAT_ADPCM_G721 = 23;
	public static final int	SND_FORMAT_ADPCM_G722 = 24;
	public static final int	SND_FORMAT_ADPCM_G723_3 = 25;
	public static final int	SND_FORMAT_ADPCM_G723_5 = 26;
	public static final int	SND_FORMAT_ALAW_8 = 27;

	public static int getFormatCode(AudioFormat format) {
		AudioFormat.Encoding encoding = format.getEncoding();
		int nSampleSize = format.getSampleSizeInBits();
		// must be big endian for >8 bit formats
		boolean bigEndian = format.isBigEndian();
		// $$fb 2000-08-16: check the frame size, too.
		boolean frameSizeOK=(
		                        format.getFrameSize()==AudioSystem.NOT_SPECIFIED
		                        || format.getChannels()!=AudioSystem.NOT_SPECIFIED
		                        || format.getFrameSize()==nSampleSize/8*format.getChannels());

		if (encoding.equals(AudioFormat.Encoding.ULAW) && nSampleSize == 8 && frameSizeOK) {
			return SND_FORMAT_MULAW_8;
		} else if (encoding.equals(AudioFormat.Encoding.PCM_SIGNED) && frameSizeOK) {
			if (nSampleSize == 8) {
				return SND_FORMAT_LINEAR_8;
			} else if (nSampleSize == 16 && bigEndian) {
				return SND_FORMAT_LINEAR_16;
			} else if (nSampleSize == 24 && bigEndian) {
				return SND_FORMAT_LINEAR_24;
			} else if (nSampleSize == 32 && bigEndian) {
				return SND_FORMAT_LINEAR_32;
			}
		} else if (encoding.equals(AudioFormat.Encoding.ALAW) && nSampleSize == 8 && frameSizeOK) {
			return SND_FORMAT_ALAW_8;
		}
		return SND_FORMAT_UNSPECIFIED;
	}
}

/*** AuTool.java ***/
