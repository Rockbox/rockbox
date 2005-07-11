/*
 *	TAudioOutputStream.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
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

package org.tritonus.share.sampled.file;

import java.io.IOException;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;

import org.tritonus.share.TDebug;


/**	
 * Base class for classes implementing AudioOutputStream.
 *
 * @author Matthias Pfisterer
 */
 
public abstract class TAudioOutputStream
implements AudioOutputStream
{
	private AudioFormat		m_audioFormat;
	private long			m_lLength; // in bytes
	private long			m_lCalculatedLength;
	private TDataOutputStream	m_dataOutputStream;
	private boolean			m_bDoBackPatching;
	private boolean			m_bHeaderWritten;



	protected TAudioOutputStream(AudioFormat audioFormat,
				     long lLength,
				     TDataOutputStream dataOutputStream,
				     boolean bDoBackPatching)
	{
		m_audioFormat = audioFormat;
		m_lLength = lLength;
		m_lCalculatedLength = 0;
		m_dataOutputStream = dataOutputStream;
		m_bDoBackPatching = bDoBackPatching;
		m_bHeaderWritten = false;
	}



	public AudioFormat getFormat()
	{
		return m_audioFormat;
	}



	/**	Gives length of the stream.
	 *	This value is in bytes. It may be AudioSystem.NOT_SPECIFIED
	 *	to express that the length is unknown.
	 */
	public long getLength()
	{
		return m_lLength;
	}



	/**	Gives number of bytes already written.
	 */
	// IDEA: rename this to BytesWritten or something like that ?
	public long getCalculatedLength()
	{
		return m_lCalculatedLength;
	}

	protected TDataOutputStream getDataOutputStream()
	{
		return m_dataOutputStream;
	}


	/**	Writes audio data to the destination (file or output stream).
	 */
	// IDEA: use long?
	public int write(byte[] abData, int nOffset, int nLength)
		throws IOException
	{
		if (TDebug.TraceAudioOutputStream)
		{
			TDebug.out("TAudioOutputStream.write(): wanted length: " + nLength);
		}
		if (! m_bHeaderWritten)
		{
			writeHeader();
			m_bHeaderWritten = true;
		}
		// $$fb added
		// check that total writes do not exceed specified length
		long lTotalLength=getLength();
		if (lTotalLength!=AudioSystem.NOT_SPECIFIED && (m_lCalculatedLength+nLength)>lTotalLength) {
			if (TDebug.TraceAudioOutputStream) {
				TDebug.out("TAudioOutputStream.write(): requested more bytes to write than possible.");
			}
			nLength=(int) (lTotalLength-m_lCalculatedLength);
			// sanity
			if (nLength<0) {
				nLength=0;
			}
		}
		// TODO: throw an exception if nLength==0 ? (to indicate end of file ?)
		if (nLength>0) {
			m_dataOutputStream.write(abData, nOffset, nLength);
			m_lCalculatedLength += nLength;
		}
		if (TDebug.TraceAudioOutputStream)
		{
			TDebug.out("TAudioOutputStream.write(): calculated (total) length: " + m_lCalculatedLength+" bytes = "+(m_lCalculatedLength/getFormat().getFrameSize())+" frames");
		}
		return nLength;
	}



	/**	Writes the header of the audio file.
	 */
	protected abstract void writeHeader()
		throws IOException;



	/**	Closes the stream.
	 *	This does write remaining buffered data to the destination,
	 *	backpatch the header, if necessary, and closes the destination.
	 */
	public void close()
		throws IOException
	{
		if (TDebug.TraceAudioOutputStream)
		{
			TDebug.out("TAudioOutputStream.close(): called");
		}
		// flush?
		if (m_bDoBackPatching)
		{
			if (TDebug.TraceAudioOutputStream)
			{
				TDebug.out("TAudioOutputStream.close(): patching header");
			}
			patchHeader();
		}
		m_dataOutputStream.close();
	}



	protected void patchHeader()
		throws IOException
	{
		TDebug.out("TAudioOutputStream.patchHeader(): called");
		// DO NOTHING
	}



	protected void setLengthFromCalculatedLength()
	{
		m_lLength = m_lCalculatedLength;
	}
}



/*** TAudioOutputStream.java ***/
