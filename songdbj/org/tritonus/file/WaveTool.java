/*
 *	WaveTool.java
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

package org.tritonus.sampled.file;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;


/**
 * Common constants and methods for handling wave files.
 *
 * @author Florian Bomers
 */

public class WaveTool {

	public static final int WAVE_RIFF_MAGIC = 0x52494646; // "RIFF"
	public static final int WAVE_WAVE_MAGIC = 0x57415645; // "WAVE"
	public static final int WAVE_FMT_MAGIC  = 0x666D7420; // "fmt "
	public static final int WAVE_DATA_MAGIC = 0x64617461; // "DATA"
	public static final int WAVE_FACT_MAGIC = 0x66616374; // "fact"

	public static final short WAVE_FORMAT_UNSPECIFIED = 0;
	public static final short WAVE_FORMAT_PCM = 1;
	public static final short WAVE_FORMAT_MS_ADPCM = 2;
	public static final short WAVE_FORMAT_ALAW = 6;
	public static final short WAVE_FORMAT_ULAW = 7;
	public static final short WAVE_FORMAT_IMA_ADPCM = 17; // same as DVI_ADPCM
	public static final short WAVE_FORMAT_G723_ADPCM = 20;
	public static final short WAVE_FORMAT_GSM610 = 49;
	public static final short WAVE_FORMAT_G721_ADPCM = 64;
	public static final short WAVE_FORMAT_MPEG = 80;

	public static final int MIN_FMT_CHUNK_LENGTH=14;
	public static final int MIN_DATA_OFFSET=12+8+MIN_FMT_CHUNK_LENGTH+8;
	public static final int MIN_FACT_CHUNK_LENGTH = 4;

	// we always write the sample size in bits and the length of extra bytes.
	// There are programs (CoolEdit) that rely on the
	// additional entry for sample size in bits.
	public static final int FMT_CHUNK_SIZE=18;
	public static final int RIFF_CONTAINER_CHUNK_SIZE=12;
	public static final int CHUNK_HEADER_SIZE=8;
	public static final int DATA_OFFSET=RIFF_CONTAINER_CHUNK_SIZE
	                                    +CHUNK_HEADER_SIZE+FMT_CHUNK_SIZE+CHUNK_HEADER_SIZE;

	public static AudioFormat.Encoding GSM0610 = new AudioFormat.Encoding("GSM0610");
	public static AudioFormat.Encoding IMA_ADPCM = new AudioFormat.Encoding("IMA_ADPCM");

	public static short getFormatCode(AudioFormat format) {
		AudioFormat.Encoding encoding = format.getEncoding();
		int nSampleSize = format.getSampleSizeInBits();
		boolean littleEndian = !format.isBigEndian();
		boolean frameSizeOK=format.getFrameSize()==AudioSystem.NOT_SPECIFIED
		                    || format.getChannels()!=AudioSystem.NOT_SPECIFIED
		                    || format.getFrameSize()==nSampleSize/8*format.getChannels();

		if (nSampleSize==8 && frameSizeOK
		    && (encoding.equals(AudioFormat.Encoding.PCM_SIGNED)
		        || encoding.equals(AudioFormat.Encoding.PCM_UNSIGNED))) {
		     	return WAVE_FORMAT_PCM;
		} else if (nSampleSize>8 && frameSizeOK && littleEndian
		           && encoding.equals(AudioFormat.Encoding.PCM_SIGNED)) {
			return WAVE_FORMAT_PCM;
		} else if (encoding.equals(AudioFormat.Encoding.ULAW)
		           && (nSampleSize==AudioSystem.NOT_SPECIFIED || nSampleSize == 8)
		           && frameSizeOK) {
			return WAVE_FORMAT_ULAW;
		} else if (encoding.equals(AudioFormat.Encoding.ALAW)
		           && (nSampleSize==AudioSystem.NOT_SPECIFIED || nSampleSize == 8)
		           && frameSizeOK) {
			return WAVE_FORMAT_ALAW;
		} else if (encoding.equals(new AudioFormat.Encoding("IMA_ADPCM"))
		           && nSampleSize == 4)
		{
			return WAVE_FORMAT_IMA_ADPCM;
		}
		else if (encoding.equals(GSM0610)) {
			return WAVE_FORMAT_GSM610;
		}
		return WAVE_FORMAT_UNSPECIFIED;
	}

}

/*** WaveTool.java ***/
