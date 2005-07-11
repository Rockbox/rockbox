/*
 *	JorbisAudioFileReader.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2001 by Matthias Pfisterer
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

package org.tritonus.sampled.file.jorbis;

import java.io.InputStream;
import java.io.IOException;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.TAudioFileFormat;
import org.tritonus.share.sampled.file.TAudioFileReader;

import com.jcraft.jogg.Buffer;
import com.jcraft.jogg.SyncState;
import com.jcraft.jogg.StreamState;
import com.jcraft.jogg.Page;
import com.jcraft.jogg.Packet;



/**
 * @author Matthias Pfisterer
 */
public class JorbisAudioFileReader
extends TAudioFileReader
{
	private static final int	INITAL_READ_LENGTH = 4096;
	private static final int	MARK_LIMIT = INITAL_READ_LENGTH + 1;



	public JorbisAudioFileReader()
	{
		super(MARK_LIMIT, true);
	}



	protected AudioFileFormat getAudioFileFormat(InputStream inputStream, long lFileSizeInBytes)
		throws UnsupportedAudioFileException, IOException
	{
		// sync and verify incoming physical bitstream
		SyncState	oggSyncState = new SyncState();

		// take physical pages, weld into a logical stream of packets
		StreamState	oggStreamState = new StreamState();

		// one Ogg bitstream page.  Vorbis packets are inside
		Page		oggPage = new Page();

		// one raw packet of data for decode
		Packet		oggPacket = new Packet();

		int		bytes = 0;

		// Decode setup

		oggSyncState.init(); // Now we can read pages

		// grab some data at the head of the stream.  We want the first page
		// (which is guaranteed to be small and only contain the Vorbis
		// stream initial header) We need the first page to get the stream
		// serialno.

		// submit a 4k block to libvorbis' Ogg layer
		int	index = oggSyncState.buffer(INITAL_READ_LENGTH);
		bytes = inputStream.read(oggSyncState.data, index, INITAL_READ_LENGTH);
		oggSyncState.wrote(bytes);
    
		// Get the first page.
		if (oggSyncState.pageout(oggPage) != 1)
		{
			// have we simply run out of data?  If so, we're done.
			if (bytes < 4096)
			{
				// IDEA: throw EOFException?
				throw new UnsupportedAudioFileException("not a Vorbis stream: ended prematurely");
			}
      			throw new UnsupportedAudioFileException("not a Vorbis stream: not in Ogg bitstream format");
		}

		// Get the serial number and set up the rest of decode.
		// serialno first; use it to set up a logical stream
		oggStreamState.init(oggPage.serialno());

		// extract the initial header from the first page and verify that the
		// Ogg bitstream is in fact Vorbis data

		// I handle the initial header first instead of just having the code
		// read all three Vorbis headers at once because reading the initial
		// header is an easy way to identify a Vorbis bitstream and it's
		// useful to see that functionality seperated out.

		if (oggStreamState.pagein(oggPage) < 0)
		{
			// error; stream version mismatch perhaps
			throw new UnsupportedAudioFileException("not a Vorbis stream: can't read first page of Ogg bitstream data");
		}
    
		if (oggStreamState.packetout(oggPacket) != 1)
		{
			// no page? must not be vorbis
			throw new UnsupportedAudioFileException("not a Vorbis stream: can't read initial header packet");
		}

		Buffer	oggPacketBuffer = new Buffer();
		oggPacketBuffer.readinit(oggPacket.packet_base, oggPacket.packet, oggPacket.bytes);

		int nPacketType = oggPacketBuffer.read(8);
		byte[] buf = new byte[6];
		oggPacketBuffer.read(buf, 6);
		if(buf[0]!='v' || buf[1]!='o' || buf[2]!='r' ||
		   buf[3]!='b' || buf[4]!='i' || buf[5]!='s')
		{
			throw new UnsupportedAudioFileException("not a Vorbis stream: not a vorbis header packet");
		}
		if (nPacketType != 1)
		{
			throw new UnsupportedAudioFileException("not a Vorbis stream: first packet is not the identification header");
		}
		if(oggPacket.b_o_s == 0)
		{
			throw new UnsupportedAudioFileException("not a Vorbis stream: initial packet not marked as beginning of stream");
		}
		int	nVersion = oggPacketBuffer.read(32);
		if (nVersion != 0)
		{
			throw new UnsupportedAudioFileException("not a Vorbis stream: wrong vorbis version");
		}
		int	nChannels = oggPacketBuffer.read(8);
		float	fSampleRate = oggPacketBuffer.read(32);

		// These are only used for error checking.
		int bitrate_upper = oggPacketBuffer.read(32);
		int bitrate_nominal = oggPacketBuffer.read(32);
		int bitrate_lower = oggPacketBuffer.read(32);

		int[] blocksizes = new int[2];
		blocksizes[0] = 1 << oggPacketBuffer.read(4);
		blocksizes[1] = 1 << oggPacketBuffer.read(4);

		if (fSampleRate < 1.0F ||
		    nChannels < 1 ||
		    blocksizes[0] < 8||
		    blocksizes[1] < blocksizes[0] ||
		    oggPacketBuffer.read(1) != 1)
		{
			throw new UnsupportedAudioFileException("not a Vorbis stream: illegal values in initial header");
		}


		if (TDebug.TraceAudioFileReader) { TDebug.out("JorbisAudioFileReader.getAudioFileFormat(): channels: " + nChannels); }
		if (TDebug.TraceAudioFileReader) { TDebug.out("JorbisAudioFileReader.getAudioFileFormat(): rate: " + fSampleRate); }

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
		if (TDebug.TraceAudioFileReader) { TDebug.out("JorbisAudioFileReader.getAudioFileFormat(): AudioFormat: " + format); }
		AudioFileFormat.Type	type = new AudioFileFormat.Type("Ogg","ogg");
		AudioFileFormat	audioFileFormat =
			new TAudioFileFormat(
				type,
				format,
				nFrameSize,
				nByteSize);
		if (TDebug.TraceAudioFileReader) { TDebug.out("JorbisAudioFileReader.getAudioFileFormat(): AudioFileFormat: " + audioFileFormat); }
		return audioFileFormat;
	}
}



/*** JorbisAudioFileReader.java ***/

