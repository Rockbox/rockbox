/*
 *	WaveAudioOutputStream.java
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

import java.io.IOException;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioSystem;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.TAudioOutputStream;
import org.tritonus.share.sampled.file.TDataOutputStream;


/**
 * AudioOutputStream for Wave files.
 *
 * @author Florian Bomers
 */

public class WaveAudioOutputStream extends TAudioOutputStream {

	// this constant is used for chunk lengths when the length is not known yet
	private static final int LENGTH_NOT_KNOWN=-1;
	private int formatCode;

	public WaveAudioOutputStream(AudioFormat audioFormat,
	                             long lLength,
	                             TDataOutputStream dataOutputStream) {
		super(audioFormat,
		      lLength,
		      dataOutputStream,
		      lLength == AudioSystem.NOT_SPECIFIED && dataOutputStream.supportsSeek());
		// wave cannot store more than 4GB
		if (lLength != AudioSystem.NOT_SPECIFIED
		    && (lLength+WaveTool.DATA_OFFSET)>0xFFFFFFFFl) {
			if (TDebug.TraceAudioOutputStream) {
				TDebug.out("WaveAudioOutputStream: Length exceeds 4GB: "
				           +lLength+"=0x"+Long.toHexString(lLength)
				           +" with header="+(lLength+WaveTool.DATA_OFFSET)
				           +"=0x"+Long.toHexString(lLength+WaveTool.DATA_OFFSET));
			}
			throw new IllegalArgumentException("Wave files cannot be larger than 4GB.");
		}
		formatCode = WaveTool.getFormatCode(getFormat());
		if (formatCode == WaveTool.WAVE_FORMAT_UNSPECIFIED) {
			throw new IllegalArgumentException("Unknown encoding/format for this wave file.");
		}

	}

	protected void writeHeader()
	throws IOException {
		if (TDebug.TraceAudioOutputStream) {
			TDebug.out("WaveAudioOutputStream.writeHeader()");
		}
		AudioFormat		format = getFormat();
		long			lLength = getLength();
		int formatChunkAdd=0;
		if (formatCode==WaveTool.WAVE_FORMAT_GSM610) {
			// space for extra fields
			formatChunkAdd+=2;
		}
		int dataOffset=WaveTool.DATA_OFFSET+formatChunkAdd;
		if (formatCode!=WaveTool.WAVE_FORMAT_PCM) {
			// space for fact chunk
			dataOffset+=4+WaveTool.CHUNK_HEADER_SIZE;
		}

		// if patching the header, and the length has not been known at first
		// writing of the header, just truncate the size fields, don't throw an exception
		if (lLength != AudioSystem.NOT_SPECIFIED
		    && lLength+dataOffset>0xFFFFFFFFl) {
			lLength=0xFFFFFFFFl-dataOffset;
		}

		// chunks must be on word-boundaries
		long 			lDataChunkSize=lLength+(lLength%2);
		TDataOutputStream	dos = getDataOutputStream();

		// write RIFF container chunk
		dos.writeInt(WaveTool.WAVE_RIFF_MAGIC);
		dos.writeLittleEndian32((int) ((lDataChunkSize+dataOffset-WaveTool.CHUNK_HEADER_SIZE)
		                               & 0xFFFFFFFF));
		dos.writeInt(WaveTool.WAVE_WAVE_MAGIC);

		// write fmt_ chunk
		int formatChunkSize=WaveTool.FMT_CHUNK_SIZE+formatChunkAdd;
		short sampleSizeInBits=(short) format.getSampleSizeInBits();
		int decodedSamplesPerBlock=1;

		if (formatCode==WaveTool.WAVE_FORMAT_GSM610) {
			if (format.getFrameSize()==33) {
				decodedSamplesPerBlock=320;
			} else if (format.getFrameSize()==65) {
				decodedSamplesPerBlock=320;
			} else {
				// how to retrieve this value here ?
				decodedSamplesPerBlock=(int) (format.getFrameSize()*(320.0f/65.0f));
			}
			sampleSizeInBits=0; // MS standard
		}


		int avgBytesPerSec=((int) format.getSampleRate())/decodedSamplesPerBlock*format.getFrameSize();
		dos.writeInt(WaveTool.WAVE_FMT_MAGIC);
		dos.writeLittleEndian32(formatChunkSize);
		dos.writeLittleEndian16((short) formatCode);             // wFormatTag
		dos.writeLittleEndian16((short) format.getChannels());   // nChannels
		dos.writeLittleEndian32((int) format.getSampleRate());   // nSamplesPerSec
		dos.writeLittleEndian32(avgBytesPerSec);                 // nAvgBytesPerSec
		dos.writeLittleEndian16((short) format.getFrameSize());  // nBlockalign
		dos.writeLittleEndian16(sampleSizeInBits);               // wBitsPerSample
		dos.writeLittleEndian16((short) formatChunkAdd);         // cbSize

		if (formatCode==WaveTool.WAVE_FORMAT_GSM610) {
			dos.writeLittleEndian16((short) decodedSamplesPerBlock); // wSamplesPerBlock
		}

		// write fact chunk


		if (formatCode!=WaveTool.WAVE_FORMAT_PCM) {
			// write "fact" chunk: number of samples
			// todo: add this as an attribute or property
			// in AudioOutputStream or AudioInputStream
			long samples=0;
			if (lLength!=AudioSystem.NOT_SPECIFIED) {
				samples=lLength/format.getFrameSize()*decodedSamplesPerBlock;
			}
			// saturate sample count
			if (samples>0xFFFFFFFFl) {
				samples=(0xFFFFFFFFl/decodedSamplesPerBlock)*decodedSamplesPerBlock;
			}
			dos.writeInt(WaveTool.WAVE_FACT_MAGIC);
			dos.writeLittleEndian32(4);
			dos.writeLittleEndian32((int) (samples & 0xFFFFFFFF));
		}

		// write header of data chunk
		dos.writeInt(WaveTool.WAVE_DATA_MAGIC);
		dos.writeLittleEndian32((lLength!=AudioSystem.NOT_SPECIFIED)?((int) lLength):LENGTH_NOT_KNOWN);
	}

	protected void patchHeader()
	throws IOException {
		TDataOutputStream	tdos = getDataOutputStream();
		tdos.seek(0);
		setLengthFromCalculatedLength();
		writeHeader();
	}

	public void close() throws IOException {
		long nBytesWritten=getCalculatedLength();

		if ((nBytesWritten % 2)==1) {
			if (TDebug.TraceAudioOutputStream) {
				TDebug.out("WaveOutputStream.close(): adding padding byte");
			}
			// extra byte for to align on word boundaries
			TDataOutputStream tdos = getDataOutputStream();
			tdos.writeByte(0);
			// DON'T adjust calculated length !
		}


		super.close();
	}

}

/*** WaveAudioOutputStream.java ***/
