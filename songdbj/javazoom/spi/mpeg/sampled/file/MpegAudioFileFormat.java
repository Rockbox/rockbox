/*
 * MpegAudioFileFormat.
 *
 * JavaZOOM : mp3spi@javazoom.net
 * 			  http://www.javazoom.net
 *
 *-----------------------------------------------------------------------
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
 *----------------------------------------------------------------------
 */
package javazoom.spi.mpeg.sampled.file;

import java.util.Map;

import javax.sound.sampled.AudioFormat;

import org.tritonus.share.sampled.file.TAudioFileFormat;

/**
 * @author JavaZOOM
 */
public class MpegAudioFileFormat extends TAudioFileFormat
{
	/**
	 * Contructor.
	 * @param type
	 * @param audioFormat
	 * @param nLengthInFrames
	 * @param nLengthInBytes
	 */
	public MpegAudioFileFormat(Type type, AudioFormat audioFormat, int nLengthInFrames, int nLengthInBytes, Map properties)
	{
		super(type, audioFormat, nLengthInFrames, nLengthInBytes, properties);
	}

	/**
	 * MP3 audio file format parameters.
	 * Some parameters might be unavailable. So availability test is required before reading any parameter.
	 *
	 * <br>AudioFileFormat parameters.
	 * <ul>
	 * <li><b>duration</b> [Long], duration in microseconds.
	 * <li><b>title</b> [String], Title of the stream.
	 * <li><b>author</b> [String], Name of the artist of the stream.
	 * <li><b>album</b> [String], Name of the album of the stream.
	 * <li><b>date</b> [String], The date (year) of the recording or release of the stream.
	 * <li><b>copyright</b> [String], Copyright message of the stream.
	 * <li><b>comment</b> [String], Comment of the stream.
	 * </ul>
	 * <br>MP3 parameters.
	 * <ul>
	 * <li><b>mp3.version.mpeg</b> [String], mpeg version : 1,2 or 2.5
	 * <li><b>mp3.version.layer</b> [String], layer version 1, 2 or 3
	 * <li><b>mp3.version.encoding</b> [String], mpeg encoding : MPEG1, MPEG2-LSF, MPEG2.5-LSF
	 * <li><b>mp3.channels</b> [Integer], number of channels 1 : mono, 2 : stereo.
	 * <li><b>mp3.frequency.hz</b> [Integer], sampling rate in hz.
	 * <li><b>mp3.bitrate.nominal.bps</b> [Integer], nominal bitrate in bps.
	 * <li><b>mp3.length.bytes</b> [Integer], length in bytes.
	 * <li><b>mp3.length.frames</b> [Integer], length in frames.
	 * <li><b>mp3.framesize.bytes</b> [Integer], framesize of the first frame. framesize is not constant for VBR streams.
	 * <li><b>mp3.framerate.fps</b> [Float], framerate in frames per seconds.
	 * <li><b>mp3.header.pos</b> [Integer], position of first audio header (or ID3v2 size).
	 * <li><b>mp3.vbr</b> [Boolean], vbr flag.
	 * <li><b>mp3.vbr.scale</b> [Integer], vbr scale.
	 * <li><b>mp3.crc</b> [Boolean], crc flag.
	 * <li><b>mp3.original</b> [Boolean], original flag.
	 * <li><b>mp3.copyright</b> [Boolean], copyright flag.
	 * <li><b>mp3.padding</b> [Boolean], padding flag.
	 * <li><b>mp3.mode</b> [Integer], mode 0:STEREO 1:JOINT_STEREO 2:DUAL_CHANNEL 3:SINGLE_CHANNEL
	 * <li><b>mp3.id3tag.genre</b> [String], ID3 tag (v1 or v2) genre.
	 * <li><b>mp3.id3tag.track</b> [String], ID3 tag (v1 or v2) track info.
	 * <li><b>mp3.id3tag.encoded</b> [String], ID3 tag v2 encoded by info.
	 * <li><b>mp3.id3tag.composer</b> [String], ID3 tag v2 composer info.
	 * <li><b>mp3.id3tag.grouping</b> [String], ID3 tag v2 grouping info.
	 * <li><b>mp3.id3tag.disc</b> [String], ID3 tag v2 track info.
	 * <li><b>mp3.id3tag.v2</b> [InputStream], ID3v2 frames.
	 * <li><b>mp3.id3tag.v2.version</b> [String], ID3v2 major version (2=v2.2.0, 3=v2.3.0, 4=v2.4.0).
	 * <li><b>mp3.shoutcast.metadata.key</b> [String], Shoutcast meta key with matching value.
	 * <br>For instance :
	 * <br>mp3.shoutcast.metadata.icy-irc=#shoutcast
	 * <br>mp3.shoutcast.metadata.icy-metaint=8192
	 * <br>mp3.shoutcast.metadata.icy-genre=Trance Techno Dance
	 * <br>mp3.shoutcast.metadata.icy-url=http://www.di.fm
	 * <br>and so on ...
	 * </ul>
	 */
	public Map properties()
	{
		return super.properties();
	}
}
