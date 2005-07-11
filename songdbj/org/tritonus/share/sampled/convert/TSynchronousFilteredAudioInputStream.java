/*
 *	TSynchronousFilteredAudioInputStream.java
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

package org.tritonus.share.sampled.convert;

import java.io.IOException;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.spi.FormatConversionProvider;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.AudioUtils;



/**
 * Base class for types of audio filter/converter that translate one frame to another frame.<br>
 * It provides all the transformation of frame sizes.<br>
 * It does NOT handle different sample rates of original stream and this stream !
 *
 * @author Florian Bomers
 */
public abstract class TSynchronousFilteredAudioInputStream
extends TAudioInputStream {

	private AudioInputStream originalStream;
	private AudioFormat originalFormat;
	/** 1 if original format's frame size is NOT_SPECIFIED */
	private int originalFrameSize;
	/** 1 if original format's frame size is NOT_SPECIFIED */
	private int newFrameSize;

	/**
	 * The intermediate buffer used during convert actions
	 * (if not convertInPlace is used).
	 * It remains until this audioStream is closed or destroyed
	 * and grows with the time - it always has the size of the
	 * largest intermediate buffer ever needed.
	 */
	protected byte[] buffer=null;

	/**
	 * For use of the more efficient method convertInPlace.
	 * it will be set to true when (frameSizeFactor==1)
	 */
	private boolean	m_bConvertInPlace = false;

	public TSynchronousFilteredAudioInputStream(AudioInputStream audioInputStream, AudioFormat newFormat) {
		// the super class will do nothing... we override everything
		super(audioInputStream, newFormat, audioInputStream.getFrameLength());
		originalStream=audioInputStream;
		originalFormat=audioInputStream.getFormat();
		originalFrameSize=(originalFormat.getFrameSize()<=0) ?
		                  1 : originalFormat.getFrameSize();
		newFrameSize=(getFormat().getFrameSize()<=0) ?
		             1 : getFormat().getFrameSize();
		if (TDebug.TraceAudioConverter) {
			TDebug.out("TSynchronousFilteredAudioInputStream: original format ="
			           +AudioUtils.format2ShortStr(originalFormat));
			TDebug.out("TSynchronousFilteredAudioInputStream: converted format="
			           +AudioUtils.format2ShortStr(getFormat()));
		}
		//$$fb 2000-07-17: convert in place has to be enabled explicitly with "enableConvertInPlace"
		//if (getFormat().getFrameSize() == originalFormat.getFrameSize()) {
		//  m_bConvertInPlace = true;
		//}
		m_bConvertInPlace = false;
	}

	protected boolean enableConvertInPlace() {
		if (newFrameSize >= originalFrameSize) {
			m_bConvertInPlace = true;
		}
		return m_bConvertInPlace;
	}


	/**
	 * Override this method to do the actual conversion.
	 * inBuffer starts always at index 0 (it is an internal buffer)
	 * You should always override this.
	 * inFrameCount is the number of frames in inBuffer. These
	 * frames are of the format originalFormat.
	 * @return the resulting number of <B>frames</B> converted and put into
	 * outBuffer. The return value is in the format of this stream.
	 */
	protected abstract int convert(byte[] inBuffer, byte[] outBuffer, int outByteOffset, int inFrameCount);



	/**
	 * Override this method to provide in-place conversion of samples.
	 * To use it, call "enableConvertInPlace()". It will only be used when
	 * input bytes per frame >= output bytes per frame.
	 * This method must always convert frameCount frames, so no return value is necessary.
	 */
	protected void convertInPlace(byte[] buffer, int byteOffset, int frameCount) {
		throw new RuntimeException("Illegal call to convertInPlace");
	}

	public int read()
	throws IOException {
		if (newFrameSize != 1) {
			throw new IOException("frame size must be 1 to read a single byte");
		}
		// very ugly, but efficient. Who uses this method anyway ?
		// TODO: use an instance variable
		byte[] temp = new byte[1];
		int result = read(temp);
		if (result == -1) {
			return -1;
		}
		if (result == 0) {
			// what in this case ??? Let's hope it never occurs.
			return -1;
		}
		return temp[0] & 0xFF;
	}



	private void clearBuffer() {
		buffer = null;
	}

	public AudioInputStream getOriginalStream() {
		return originalStream;
	}

	public AudioFormat getOriginalFormat() {
		return originalFormat;
	}

	/**
	 * Read nLength bytes that will be the converted samples
	 * of the original InputStream.
	 * When nLength is not an integral number of frames,
	 * this method may read less than nLength bytes.
	 */
	public int read(byte[] abData, int nOffset, int nLength)
	throws IOException {
		// number of frames that we have to read from the underlying stream.
		int	nFrameLength = nLength/newFrameSize;

		// number of bytes that we need to read from underlying stream.
		int	originalBytes = nFrameLength * originalFrameSize;

		if (TDebug.TraceAudioConverter) {
			TDebug.out("> TSynchronousFilteredAIS.read(buffer["+abData.length+"], "
			           +nOffset+" ,"+nLength+" bytes ^="+nFrameLength+" frames)");
		}
		int nFramesConverted = 0;

		// set up buffer to read
		byte readBuffer[];
		int readOffset;
		if (m_bConvertInPlace) {
			readBuffer=abData;
			readOffset=nOffset;
		} else {
			// assert that the buffer fits
			if (buffer == null || buffer.length < originalBytes) {
				buffer = new byte[originalBytes];
			}
			readBuffer=buffer;
			readOffset=0;
		}
		int nBytesRead = originalStream.read(readBuffer, readOffset, originalBytes);
		if (nBytesRead == -1) {
			// end of stream
			clearBuffer();
			return -1;
		}
		int nFramesRead = nBytesRead / originalFrameSize;
		if (TDebug.TraceAudioConverter) {
			TDebug.out("original.read returned "
			           +nBytesRead+" bytes ^="+nFramesRead+" frames");
		}
		if (m_bConvertInPlace) {
			convertInPlace(abData, nOffset, nFramesRead);
			nFramesConverted=nFramesRead;
		} else {
			nFramesConverted = convert(buffer, abData, nOffset, nFramesRead);
		}
		if (TDebug.TraceAudioConverter) {
			TDebug.out("< converted "+nFramesConverted+" frames");
		}
		return nFramesConverted*newFrameSize;
	}


	public long skip(long nSkip)
	throws IOException {
		// only returns integral frames
		long skipFrames = nSkip / newFrameSize;
		long originalSkippedBytes = originalStream.skip(skipFrames*originalFrameSize);
		long skippedFrames = originalSkippedBytes/originalFrameSize;
		return skippedFrames * newFrameSize;
	}


	public int available()
	throws IOException {
		int origAvailFrames = originalStream.available()/originalFrameSize;
		return origAvailFrames*newFrameSize;
	}


	public void close()
	throws IOException {
		originalStream.close();
		clearBuffer();
	}



	public void mark(int readlimit) {
		int readLimitFrames=readlimit/newFrameSize;
		originalStream.mark(readLimitFrames*originalFrameSize);
	}



	public void reset()
	throws IOException {
		originalStream.reset();
	}


	public boolean markSupported() {
		return originalStream.markSupported();
	}


	private int getFrameSize() {
		return getFormat().getFrameSize();
	}

}


/*** TSynchronousFilteredAudioInputStream.java ***/
