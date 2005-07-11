/*
 * MpegAudioFormat.
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

import org.tritonus.share.sampled.TAudioFormat;

/**
 * @author JavaZOOM
 */
public class MpegAudioFormat extends TAudioFormat
{	
	/**
	 * Constructor.
	 * @param encoding
	 * @param nFrequency
	 * @param SampleSizeInBits
	 * @param nChannels
	 * @param FrameSize
	 * @param FrameRate
	 * @param isBigEndian
	 * @param properties
	 */
	public MpegAudioFormat(AudioFormat.Encoding encoding, float nFrequency, int SampleSizeInBits, int nChannels, int FrameSize, float FrameRate, boolean isBigEndian, Map properties)
	{
		super(encoding, nFrequency, SampleSizeInBits, nChannels, FrameSize, FrameRate, isBigEndian, properties);
	}

	/**
	 * MP3 audio format parameters. 
	 * Some parameters might be unavailable. So availability test is required before reading any parameter.  
	 *
	 * <br>AudioFormat parameters.
	 * <ul>
	 * <li><b>bitrate</b> [Integer], bitrate in bits per seconds, average bitrate for VBR enabled stream.
	 * <li><b>vbr</b> [Boolean], VBR flag.
	 * </ul>
	 */
	public Map properties()
	{
		return super.properties();	
	}	
}
