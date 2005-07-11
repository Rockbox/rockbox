/*
 *	TAudioFileReader.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 by Matthias Pfisterer
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

package org.tritonus.share.sampled.file;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.DataInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.EOFException;

import java.net.URL;
import java.net.URLConnection;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import javax.sound.sampled.spi.AudioFileReader;

import org.tritonus.share.TDebug;



/**	Base class for audio file readers.
	This is Tritonus' base class for classes that provide the facility
	of detecting an audio file type and reading its header.
	Classes should be derived from this class or one of its subclasses
	rather than from javax.sound.sampled.spi.AudioFileReader.

	@author Matthias Pfisterer
	@author Florian Bomers
*/
public abstract class TAudioFileReader
extends	AudioFileReader
{
	private int	m_nMarkLimit = -1;
	private boolean	m_bRereading;


	protected TAudioFileReader(int nMarkLimit)
	{
		this(nMarkLimit, false);
	}



	protected TAudioFileReader(int nMarkLimit, boolean bRereading)
	{
		m_nMarkLimit = nMarkLimit;
		m_bRereading = bRereading;
	}



	private int getMarkLimit()
	{
		return m_nMarkLimit;
	}



	private boolean isRereading()
	{
		return m_bRereading;
	}



	/**	Get an AudioFileFormat object for a File.
		This method calls getAudioFileFormat(InputStream, long).
		Subclasses should not override this method unless there are
		really severe reasons. Normally, it is sufficient to
		implement getAudioFileFormat(InputStream, long).

		@param file	the file to read from.
		@return	an AudioFileFormat instance containing
		information from the header of the file passed in.
	*/
	public AudioFileFormat getAudioFileFormat(File file)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioFileFormat(File): begin"); }
		long	lFileLengthInBytes = file.length();
		InputStream	inputStream = new FileInputStream(file);
		AudioFileFormat	audioFileFormat = null;
		try
		{
			audioFileFormat = getAudioFileFormat(inputStream, lFileLengthInBytes);
		}
		finally
		{
			inputStream.close();
		}
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioFileFormat(File): end"); }
		return audioFileFormat;
	}



	/**	Get an AudioFileFormat object for a URL.
		This method calls getAudioFileFormat(InputStream, long).
		Subclasses should not override this method unless there are
		really severe reasons. Normally, it is sufficient to
		implement getAudioFileFormat(InputStream, long).

		@param url	the URL to read from.
		@return	an AudioFileFormat instance containing
		information from the header of the URL passed in.
	*/
	public AudioFileFormat getAudioFileFormat(URL url)
		throws UnsupportedAudioFileException, IOException

	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioFileFormat(URL): begin"); }
		long	lFileLengthInBytes = getDataLength(url);
		InputStream	inputStream = url.openStream();
		AudioFileFormat	audioFileFormat = null;
		try
		{
			audioFileFormat = getAudioFileFormat(inputStream, lFileLengthInBytes);
		}
		finally
		{
			inputStream.close();
		}
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioFileFormat(URL): end"); }
		return audioFileFormat;
	}



	/**	Get an AudioFileFormat object for an InputStream.
		This method calls getAudioFileFormat(InputStream, long).
		Subclasses should not override this method unless there are
		really severe reasons. Normally, it is sufficient to
		implement getAudioFileFormat(InputStream, long).

		@param inputStream	the stream to read from.
		@return	an AudioFileFormat instance containing
		information from the header of the stream passed in.
	*/
	public AudioFileFormat getAudioFileFormat(InputStream inputStream)
		throws UnsupportedAudioFileException, IOException

	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioFileFormat(InputStream): begin"); }
		long	lFileLengthInBytes = AudioSystem.NOT_SPECIFIED;
		inputStream.mark(getMarkLimit());
		AudioFileFormat	audioFileFormat = null;
		try
		{
			audioFileFormat = getAudioFileFormat(inputStream, lFileLengthInBytes);
		}
		finally
		{
			/* TODO: required semantics is unclear: should reset()
			   be executed only when there is an exception or
			   should it be done always?
			*/
			inputStream.reset();
		}
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioFileFormat(InputStream): end"); }
		return audioFileFormat;
	}



	/**	Get an AudioFileFormat (internal implementation).
		Subclasses must implement this method in a way specific
		to the file format they handle.

		Note that depending on the implementation of this method,
		you should or should not override
		getAudioInputStream(InputStream, long), too (see comment
		there).

		@param inputStream	The InputStream to read from.
		@param lFileLengthInBytes	The size of the originating
		file, if known. If it isn't known, AudioSystem.NOT_SPECIFIED
		should be passed. This value may be used for byteLength in
		AudioFileFormat, if this value can't be derived from the
		informmation in the file header.

		@return	an AudioFileFormat instance containing
		information from the header of the stream passed in as
		inputStream.
	*/
	protected abstract AudioFileFormat getAudioFileFormat(
		InputStream inputStream,
		long lFileLengthInBytes)
		throws UnsupportedAudioFileException, IOException;



	/**	Get an AudioInputStream object for a file.
		This method calls getAudioInputStream(InputStream, long).
		Subclasses should not override this method unless there are
		really severe reasons. Normally, it is sufficient to
		implement getAudioFileFormat(InputStream, long) and perhaps
		override getAudioInputStream(InputStream, long).

		@param file	the File object to read from.
		@return	an AudioInputStream instance containing
		the audio data from this file.
	*/
	public AudioInputStream getAudioInputStream(File file)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(File): begin"); }
		long	lFileLengthInBytes = file.length();
		InputStream	inputStream = new FileInputStream(file);
		AudioInputStream	audioInputStream = null;
		try
		{
			audioInputStream = getAudioInputStream(inputStream, lFileLengthInBytes);
		}
		catch (UnsupportedAudioFileException e)
		{
			inputStream.close();
			throw e;
		}
		catch (IOException e)
		{
			inputStream.close();
			throw e;
		}
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(File): end"); }
		return audioInputStream;
	}



	/**	Get an AudioInputStream object for a URL.
		This method calls getAudioInputStream(InputStream, long).
		Subclasses should not override this method unless there are
		really severe reasons. Normally, it is sufficient to
		implement getAudioFileFormat(InputStream, long) and perhaps
		override getAudioInputStream(InputStream, long).

		@param url	the URL to read from.
		@return	an AudioInputStream instance containing
		the audio data from this URL.
	*/
	public AudioInputStream getAudioInputStream(URL url)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(URL): begin"); }
		long	lFileLengthInBytes = getDataLength(url);
		InputStream	inputStream = url.openStream();
		AudioInputStream	audioInputStream = null;
		try
		{
			audioInputStream = getAudioInputStream(inputStream, lFileLengthInBytes);
		}
		catch (UnsupportedAudioFileException e)
		{
			inputStream.close();
			throw e;
		}
		catch (IOException e)
		{
			inputStream.close();
			throw e;
		}
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(URL): end"); }
		return audioInputStream;
	}



	/**	Get an AudioInputStream object for an InputStream.
		This method calls getAudioInputStream(InputStream, long).
		Subclasses should not override this method unless there are
		really severe reasons. Normally, it is sufficient to
		implement getAudioFileFormat(InputStream, long) and perhaps
		override getAudioInputStream(InputStream, long).

		@param inputStream	the stream to read from.
		@return	an AudioInputStream instance containing
		the audio data from this stream.
	*/
	public AudioInputStream getAudioInputStream(InputStream inputStream)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(InputStream): begin"); }
		long	lFileLengthInBytes = AudioSystem.NOT_SPECIFIED;
		AudioInputStream	audioInputStream = null;
		inputStream.mark(getMarkLimit());
		try
		{
			audioInputStream = getAudioInputStream(inputStream, lFileLengthInBytes);
		}
		catch (UnsupportedAudioFileException e)
		{
			inputStream.reset();
			throw e;
		}
		catch (IOException e)
		{
			inputStream.reset();
			throw e;
		}
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(InputStream): end"); }
		return audioInputStream;
	}



	/**	Get an AudioInputStream (internal implementation).
		This implementation calls getAudioFileFormat() with the
		same arguments as passed in here. Then, it constructs
		an AudioInputStream instance. This instance takes the passed
		inputStream in the state it is left after getAudioFileFormat()
		did its work. In other words, the implementation here
		assumes that getAudioFileFormat() reads the entire header
		up to a position exactely where the audio data starts.
		If this can't be realized for a certain format, this method
		should be overridden.

		@param inputStream	The InputStream to read from.
		@param lFileLengthInBytes	The size of the originating
		file, if known. If it isn't known, AudioSystem.NOT_SPECIFIED
		should be passed. This value may be used for byteLength in
		AudioFileFormat, if this value can't be derived from the
		informmation in the file header.
	*/
	protected AudioInputStream getAudioInputStream(InputStream inputStream, long lFileLengthInBytes)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(InputStream, long): begin"); }
		if (isRereading())
		{
			inputStream = new BufferedInputStream(inputStream, getMarkLimit());
			inputStream.mark(getMarkLimit());
		}
		AudioFileFormat	audioFileFormat = getAudioFileFormat(inputStream, lFileLengthInBytes);
		if (isRereading())
		{
			inputStream.reset();
		}
		AudioInputStream	audioInputStream =
			new AudioInputStream(inputStream,
					     audioFileFormat.getFormat(),
					     audioFileFormat.getFrameLength());
		if (TDebug.TraceAudioFileReader) {TDebug.out("TAudioFileReader.getAudioInputStream(InputStream, long): end"); }
		return audioInputStream;
	}



	protected static int calculateFrameSize(int nSampleSize, int nNumChannels)
	{
		return ((nSampleSize + 7) / 8) * nNumChannels;
	}



	private static long getDataLength(URL url)
		throws IOException
	{
		long	lFileLengthInBytes = AudioSystem.NOT_SPECIFIED;
		URLConnection connection = url.openConnection();
		connection.connect();
		int nLength = connection.getContentLength();
		if (nLength > 0)
		{
			lFileLengthInBytes = nLength;
		}
		return lFileLengthInBytes;
	}



	public static int readLittleEndianInt(InputStream is)
		throws IOException
	{
		int	b0 = is.read();
		int	b1 = is.read();
		int	b2 = is.read();
		int	b3 = is.read();
		if ((b0 | b1 | b2 | b3) < 0)
		{
			throw new EOFException();
		}
		return (b3 << 24) + (b2 << 16) + (b1 << 8) + (b0 << 0);
	}



	public static short readLittleEndianShort(InputStream is)
		throws IOException
	{
		int	b0 = is.read();
		int	b1 = is.read();
		if ((b0 | b1) < 0)
		{
			throw new EOFException();
		}
		return (short) ((b1 << 8) + (b0 << 0));
	}


/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D
 */

/*
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

	public static double readIeeeExtended(DataInputStream dis)
		throws IOException
	{
		double f = 0.0D;
		int expon = 0;
		long hiMant = 0L;
		long loMant = 0L;
		double HUGE = 3.4028234663852886E+038D;
		expon = dis.readUnsignedShort();
		long t1 = dis.readUnsignedShort();
		long t2 = dis.readUnsignedShort();
		hiMant = t1 << 16 | t2;
		t1 = dis.readUnsignedShort();
		t2 = dis.readUnsignedShort();
		loMant = t1 << 16 | t2;
		if(expon == 0 && hiMant == 0L && loMant == 0L)
		{
			f = 0.0D;
		}
		else
		{
			if(expon == 32767)
			{
				f = HUGE;
			}
			else
			{
				expon -= 16383;
				expon -= 31;
				f = hiMant * Math.pow(2D, expon);
				expon -= 32;
				f += loMant * Math.pow(2D, expon);
			}
		}
		return f;
	}
}



/*** TAudioFileReader.java ***/

