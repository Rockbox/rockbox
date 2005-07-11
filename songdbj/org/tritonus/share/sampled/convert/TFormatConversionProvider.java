/*
 *	TFormatConversionProvider.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999, 2000 by Matthias Pfisterer
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

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.spi.FormatConversionProvider;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.AudioFormats;



/**	Base class for all conversion providers of Tritonus.
 *
 *	@author Matthias Pfisterer
 */
public abstract class TFormatConversionProvider
extends FormatConversionProvider
{
	protected static final AudioFormat.Encoding[]	EMPTY_ENCODING_ARRAY = new AudioFormat.Encoding[0];
	protected static final AudioFormat[]		EMPTY_FORMAT_ARRAY = new AudioFormat[0];



	// $$fb2000-10-04: use AudioSystem.NOT_SPECIFIED for all fields.
	public AudioInputStream getAudioInputStream(AudioFormat.Encoding targetEncoding, AudioInputStream audioInputStream)
	{
		AudioFormat	sourceFormat = audioInputStream.getFormat();
		AudioFormat	targetFormat = new AudioFormat(
			targetEncoding,
			AudioSystem.NOT_SPECIFIED,   // sample rate
			AudioSystem.NOT_SPECIFIED,   // sample size in bits
			AudioSystem.NOT_SPECIFIED,   // channels
			AudioSystem.NOT_SPECIFIED,   // frame size
			AudioSystem.NOT_SPECIFIED,   // frame rate
			sourceFormat.isBigEndian());  // big endian
		if (TDebug.TraceAudioConverter)
		{
			TDebug.out("TFormatConversionProvider.getAudioInputStream(AudioFormat.Encoding, AudioInputStream):");
			TDebug.out("trying to convert to " + targetFormat);
		}
		return getAudioInputStream(targetFormat, audioInputStream);
	}



	/**
	 * WARNING: this method uses <code>getTargetFormats(AudioFormat.Encoding, AudioFormat)</code>
	 * which may create infinite loops if the latter is overwritten.
	 * <p>
	 * This method is overwritten here to make use of org.tritonus.share.sampled.AudioFormats.matches
	 * and is considered temporary until AudioFormat.matches is corrected in the JavaSound API.
	 */
	/* $$mp: if we decide to use getMatchingFormat(), this method should be
	   implemented by simply calling getMatchingFormat() and comparing the
	   result against null.
	*/
	public boolean isConversionSupported(
		AudioFormat targetFormat,
		AudioFormat sourceFormat)
	{
		if (TDebug.TraceAudioConverter)
		{
			TDebug.out(">TFormatConversionProvider.isConversionSupported(AudioFormat, AudioFormat):");
			TDebug.out("class: "+getClass().getName());
			TDebug.out("checking if conversion possible");
			TDebug.out("from: " + sourceFormat);
			TDebug.out("to: " + targetFormat);
		}
		AudioFormat[]	aTargetFormats = getTargetFormats(targetFormat.getEncoding(), sourceFormat);
		for (int i = 0; i <  aTargetFormats.length; i++)
		{
			if (TDebug.TraceAudioConverter)
			{
				TDebug.out("checking against possible target format: " + aTargetFormats[i]);
			}
			if (aTargetFormats[i] != null 
			    && AudioFormats.matches(aTargetFormats[i], targetFormat))
			{
				if (TDebug.TraceAudioConverter) 
				{
					TDebug.out("<result=true");
				}
				return true;
			}
		}
		if (TDebug.TraceAudioConverter) {
			TDebug.out("<result=false");
		}
		return false;
	}


	/**
	 * WARNING: this method uses <code>getTargetFormats(AudioFormat.Encoding, AudioFormat)</code>
	 * which may create infinite loops if the latter is overwritten.
	 * <p>
	 * This method is overwritten here to make use of org.tritonus.share.sampled.AudioFormats.matches
	 * and is considered temporary until AudioFormat.matches is corrected in the JavaSound API.
	 */
	public AudioFormat getMatchingFormat(
		AudioFormat targetFormat,
		AudioFormat sourceFormat)
	{
		if (TDebug.TraceAudioConverter)
		{
			TDebug.out(">TFormatConversionProvider.isConversionSupported(AudioFormat, AudioFormat):");
			TDebug.out("class: "+getClass().getName());
			TDebug.out("checking if conversion possible");
			TDebug.out("from: " + sourceFormat);
			TDebug.out("to: " + targetFormat);
		}
		AudioFormat[]	aTargetFormats = getTargetFormats(targetFormat.getEncoding(), sourceFormat);
		for (int i = 0; i <  aTargetFormats.length; i++)
		{
			if (TDebug.TraceAudioConverter)
			{
				TDebug.out("checking against possible target format: " + aTargetFormats[i]);
			}
			if (aTargetFormats[i] != null 
			    && AudioFormats.matches(aTargetFormats[i], targetFormat))
			{
				if (TDebug.TraceAudioConverter) 
				{
					TDebug.out("<result=true");
				}
				return aTargetFormats[i];
			}
		}
		if (TDebug.TraceAudioConverter) {
			TDebug.out("<result=false");
		}
		return null;
	}

}



/*** TFormatConversionProvider.java ***/
