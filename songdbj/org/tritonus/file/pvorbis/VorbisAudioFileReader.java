/*
 *	VorbisAudioFileReader.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2001 - 2004 by Matthias Pfisterer
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

package org.tritonus.sampled.file.pvorbis;

import java.io.InputStream;
import java.io.IOException;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.TAudioFileFormat;
import org.tritonus.share.sampled.file.TAudioFileReader;

import org.tritonus.lowlevel.pogg.Buffer;
import org.tritonus.lowlevel.pogg.Page;
import org.tritonus.lowlevel.pogg.Packet;
import org.tritonus.lowlevel.pogg.SyncState;
import org.tritonus.lowlevel.pogg.StreamState;



/**
 * @author Matthias Pfisterer
 */
public class VorbisAudioFileReader
extends TAudioFileReader
{
	private static final int	INITAL_READ_LENGTH = 4096;
	private static final int	MARK_LIMIT = INITAL_READ_LENGTH + 1;



	public VorbisAudioFileReader()
	{
		super(MARK_LIMIT, true);
	}



	protected AudioFileFormat getAudioFileFormat(InputStream inputStream,
						     long lFileSizeInBytes)
		throws UnsupportedAudioFileException, IOException
	{
		if (TDebug.TraceAudioFileReader) { TDebug.out(">VorbisAudioFileReader.getAudioFileFormat(): begin"); }
		SyncState	oggSyncState = new SyncState();
		StreamState	oggStreamState = new StreamState();
		Page		oggPage = new Page();
		Packet		oggPacket = new Packet();

		int		bytes = 0;

		// Decode setup

		oggSyncState.init(); // Now we can read pages

		// grab some data at the head of the stream.  We want the first page
		// (which is guaranteed to be small and only contain the Vorbis
		// stream initial header) We need the first page to get the stream
		// serialno.

		// submit a 4k block to libvorbis' Ogg layer
		byte[]	abBuffer = new byte[INITAL_READ_LENGTH];
		bytes = inputStream.read(abBuffer);
		if (TDebug.TraceAudioFileReader) { TDebug.out("read bytes from input stream: " + bytes); }
		int nResult = oggSyncState.write(abBuffer, bytes);
		if (TDebug.TraceAudioFileReader) { TDebug.out("SyncState.write() returned " + nResult); }
    
		// Get the first page.
		if (oggSyncState.pageOut(oggPage) != 1)
		{
			// have we simply run out of data?  If so, we're done.
			if (bytes < INITAL_READ_LENGTH)
			{
				if (TDebug.TraceAudioFileReader) { TDebug.out("stream ended prematurely"); }
				if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
				// IDEA: throw EOFException?
				oggSyncState.free();
				oggStreamState.free();
				oggPage.free();
				oggPacket.free();
				throw new UnsupportedAudioFileException("not a Vorbis stream: ended prematurely");
			}
			if (TDebug.TraceAudioFileReader) { TDebug.out("not in Ogg bitstream format"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
      			throw new UnsupportedAudioFileException("not a Vorbis stream: not in Ogg bitstream format");
		}

		// Get the serial number and set up the rest of decode.
		// serialno first; use it to set up a logical stream
		int nSerialNo = oggPage.getSerialNo();
		if (TDebug.TraceAudioFileReader) TDebug.out("serial no.: " + nSerialNo);
		oggStreamState.init(nSerialNo);

		// extract the initial header from the first page and verify that the
		// Ogg bitstream is in fact Vorbis data

		// I handle the initial header first instead of just having the code
		// read all three Vorbis headers at once because reading the initial
		// header is an easy way to identify a Vorbis bitstream and it's
		// useful to see that functionality seperated out.

		if (oggStreamState.pageIn(oggPage) < 0)
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("can't read first page of Ogg bitstream data"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			// error; stream version mismatch perhaps
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: can't read first page of Ogg bitstream data");
		}
    
		if (oggStreamState.packetOut(oggPacket) != 1)
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("can't read initial header packet"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			// no page? must not be vorbis
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: can't read initial header packet");
		}

		byte[]	abData = oggPacket.getData();
		if (TDebug.TraceAudioFileReader)
		{
			String	strData = "";
			for (int i = 0; i < abData.length; i++)
			{
				strData += " " + abData[i];
			}
			TDebug.out("packet data: " + strData);
		}

		int nPacketType = abData[0];
		if (TDebug.TraceAudioFileReader) { TDebug.out("packet type: " + nPacketType); }
		if (nPacketType != 1)
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("first packet is not the identification header"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: first packet is not the identification header");
		}
		if(abData[1] != 'v' ||
		   abData[2] != 'o' ||
		   abData[3] != 'r' ||
		   abData[4] != 'b' ||
		   abData[5] != 'i' ||
		   abData[6] != 's')
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("not a vorbis header packet"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: not a vorbis header packet");
		}
		if (! oggPacket.isBos())
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("initial packet not marked as beginning of stream"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: initial packet not marked as beginning of stream");
		}
		int	nVersion = (abData[7] & 0xFF) + 256 * (abData[8] & 0xFF) + 65536 * (abData[9] & 0xFF) + 16777216 * (abData[10] & 0xFF);
		if (TDebug.TraceAudioFileReader) { TDebug.out("version: " + nVersion); }
		if (nVersion != 0)
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("wrong vorbis version"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: wrong vorbis version");
		}
		int	nChannels = (abData[11] & 0xFF);
		float	fSampleRate = (abData[12] & 0xFF) + 256 * (abData[13] & 0xFF) + 65536 * (abData[14] & 0xFF) + 16777216 * (abData[15] & 0xFF);
		if (TDebug.TraceAudioFileReader) { TDebug.out("channels: " + nChannels); }
		if (TDebug.TraceAudioFileReader) { TDebug.out("rate: " + fSampleRate); }

		// These are only used for error checking.
		int bitrate_upper = abData[16] + 256 * abData[17] + 65536 * abData[18] + 16777216 * abData[19];
		int bitrate_nominal = abData[20] + 256 * abData[21] + 65536 * abData[22] + 16777216 * abData[23];
		int bitrate_lower = abData[24] + 256 * abData[25] + 65536 * abData[26] + 16777216 * abData[27];

		int[] blocksizes = new int[2];
		blocksizes[0] = 1 << (abData[28] & 0xF);
		blocksizes[1] = 1 << ((abData[28] >>> 4) & 0xF);
		if (TDebug.TraceAudioFileReader) { TDebug.out("blocksizes[0]: " + blocksizes[0]); }
		if (TDebug.TraceAudioFileReader) { TDebug.out("blocksizes[1]: " + blocksizes[1]); }

		if (fSampleRate < 1.0F ||
		    nChannels < 1 ||
		    blocksizes[0] < 8||
		    blocksizes[1] < blocksizes[0] ||
		    (abData[29] & 0x1) != 1)
		{
			if (TDebug.TraceAudioFileReader) { TDebug.out("illegal values in initial header"); }
			if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): throwing exception"); }
			oggSyncState.free();
			oggStreamState.free();
			oggPage.free();
			oggPacket.free();
			throw new UnsupportedAudioFileException("not a Vorbis stream: illegal values in initial header");
		}

		oggSyncState.free();
		oggStreamState.free();
		oggPage.free();
		oggPacket.free();

		/*
		  If the file size is known, we derive the number of frames
		  ('frame size') from it.
		  If the values don't fit into integers, we leave them at
		  NOT_SPECIFIED. 'Unknown' is considered less incorrect than
		  a wrong value.
		*/
		// [fb] not specifying it causes Sun's Wave file writer to write rubbish
		int	nByteSize = AudioSystem.NOT_SPECIFIED;
		if (lFileSizeInBytes != AudioSystem.NOT_SPECIFIED
		    && lFileSizeInBytes <= Integer.MAX_VALUE)
		{
			nByteSize = (int) lFileSizeInBytes;
		}
		int	nFrameSize = AudioSystem.NOT_SPECIFIED;
		/* Can we calculate a useful size?
		   Peeking into ogginfo gives the insight that the only
		   way seems to be reading through the file. This is
		   something we do not want, at least not by default.
		*/
		// nFrameSize = (int) (lFileSizeInBytes / ...;

		AudioFormat	format = new AudioFormat(
			new AudioFormat.Encoding("VORBIS"),
			fSampleRate,
			AudioSystem.NOT_SPECIFIED,
			nChannels,
			AudioSystem.NOT_SPECIFIED,
			AudioSystem.NOT_SPECIFIED,
			true);	// this value is chosen arbitrarily
		if (TDebug.TraceAudioFileReader) { TDebug.out("AudioFormat: " + format); }
		AudioFileFormat.Type	type = new AudioFileFormat.Type("Ogg","ogg");
		AudioFileFormat	audioFileFormat =
			new TAudioFileFormat(
				type,
				format,
				nFrameSize,
				nByteSize);
		if (TDebug.TraceAudioFileReader) { TDebug.out("AudioFileFormat: " + audioFileFormat); }
		if (TDebug.TraceAudioFileReader) { TDebug.out("<VorbisAudioFileReader.getAudioFileFormat(): end"); }
		return audioFileFormat;
	}
}



/*** VorbisAudioFileReader.java ***/

