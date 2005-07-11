/*
 *	TSimpleFormatConversionProvider.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 - 2004 by Matthias Pfisterer
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

import java.util.Collection;
import java.util.Iterator;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;

import org.tritonus.share.sampled.AudioFormats;
import org.tritonus.share.ArraySet;
import org.tritonus.share.TDebug;


/**
 *	This is a base class for FormatConversionProviders that can convert
 *	from each source encoding/format to each target encoding/format.
 *	If this is not the case, use TEncodingFormatConversionProvider.
 *
 *	<p>Overriding classes must 
 *      provide a constructor that calls the protected constructor of this class and override
 *	<code>AudioInputStream getAudioInputStream(AudioFormat targetFormat, AudioInputStream sourceStream)</code>.
 *	The latter method should be able to handle the case that all fields are NOT_SPECIFIED
 *      and provide appropriate default values.
 *
 * @author Matthias Pfisterer
 */

// todo:
// - declare a constant ALL_BUT_SAME_VALUE (==-2) or so that can be used in format lists
// - consistent implementation of replacing NOT_SPECIFIED when not given in conversion

public abstract class TSimpleFormatConversionProvider
extends TFormatConversionProvider
{
	private Collection<AudioFormat.Encoding>	m_sourceEncodings;
	private Collection<AudioFormat.Encoding>	m_targetEncodings;
	private Collection<AudioFormat>	m_sourceFormats;
	private Collection<AudioFormat>	m_targetFormats;



	protected TSimpleFormatConversionProvider(
		Collection<AudioFormat> sourceFormats,
		Collection<AudioFormat> targetFormats)
	{
		m_sourceEncodings = new ArraySet<AudioFormat.Encoding>();
		m_targetEncodings = new ArraySet<AudioFormat.Encoding>();
		m_sourceFormats = sourceFormats;
		m_targetFormats = targetFormats;
		collectEncodings(m_sourceFormats, m_sourceEncodings);
		collectEncodings(m_targetFormats, m_targetEncodings);
	}



	/**	Disables this FormatConversionProvider.
		This may be useful when e.g. native libraries are not present.
		TODO: enable method, better implementation
	*/
	protected void disable()
	{
		if (TDebug.TraceAudioConverter) { TDebug.out("TSimpleFormatConversionProvider.disable(): disabling " + getClass().getName()); }
		m_sourceEncodings = new ArraySet<AudioFormat.Encoding>();
		m_targetEncodings = new ArraySet<AudioFormat.Encoding>();
		m_sourceFormats = new ArraySet<AudioFormat>();
		m_targetFormats = new ArraySet<AudioFormat>();
	}		



	private static void collectEncodings(Collection<AudioFormat> formats,
										 Collection<AudioFormat.Encoding> encodings)
	{
		Iterator<AudioFormat>	iterator = formats.iterator();
		while (iterator.hasNext())
		{
			AudioFormat	format = iterator.next();
			encodings.add(format.getEncoding());
		}
	}



	public AudioFormat.Encoding[] getSourceEncodings()
	{
		return m_sourceEncodings.toArray(EMPTY_ENCODING_ARRAY);
	}



	public AudioFormat.Encoding[] getTargetEncodings()
	{
		return m_targetEncodings.toArray(EMPTY_ENCODING_ARRAY);
	}



	// overwritten of FormatConversionProvider
	public boolean isSourceEncodingSupported(AudioFormat.Encoding sourceEncoding)
	{
		return m_sourceEncodings.contains(sourceEncoding);
	}



	// overwritten of FormatConversionProvider
	public boolean isTargetEncodingSupported(AudioFormat.Encoding targetEncoding)
	{
		return m_targetEncodings.contains(targetEncoding);
	}



	/**
	 *	This implementation assumes that the converter can convert
	 *	from each of its source encodings to each of its target
	 *	encodings. If this is not the case, the converter has to
	 *	override this method.
	 */
	public AudioFormat.Encoding[] getTargetEncodings(AudioFormat sourceFormat)
	{
		if (isAllowedSourceFormat(sourceFormat))
		{
			return getTargetEncodings();
		}
		else
		{
			return EMPTY_ENCODING_ARRAY;
		}
	}



	/**
	 *	This implementation assumes that the converter can convert
	 *	from each of its source formats to each of its target
	 *	formats. If this is not the case, the converter has to
	 *	override this method.
	 */
	public AudioFormat[] getTargetFormats(AudioFormat.Encoding targetEncoding, AudioFormat sourceFormat)
	{
		if (isConversionSupported(targetEncoding, sourceFormat))
		{
			return m_targetFormats.toArray(EMPTY_FORMAT_ARRAY);
		}
		else
		{
			return EMPTY_FORMAT_ARRAY;
		}
	}


	// TODO: check if necessary
	protected boolean isAllowedSourceEncoding(AudioFormat.Encoding sourceEncoding)
	{
		return m_sourceEncodings.contains(sourceEncoding);
	}



	protected boolean isAllowedTargetEncoding(AudioFormat.Encoding targetEncoding)
	{
		return m_targetEncodings.contains(targetEncoding);
	}



	protected boolean isAllowedSourceFormat(AudioFormat sourceFormat)
	{
		Iterator<AudioFormat>	iterator = m_sourceFormats.iterator();
		while (iterator.hasNext())
		{
			AudioFormat	format = iterator.next();
			if (AudioFormats.matches(format, sourceFormat))
			{
				return true;
			}
		}
		return false;
	}



	protected boolean isAllowedTargetFormat(AudioFormat targetFormat)
	{
		Iterator<AudioFormat>	iterator = m_targetFormats.iterator();
		while (iterator.hasNext())
		{
			AudioFormat	format = iterator.next();
			if (AudioFormats.matches(format, targetFormat))
			{
				return true;
			}
		}
		return false;
	}
	
	// $$fb 2000-04-02 added some convenience methods for overriding classes
	protected Collection<AudioFormat.Encoding> getCollectionSourceEncodings()
	{
		return m_sourceEncodings;
	}
	
	protected Collection<AudioFormat.Encoding> getCollectionTargetEncodings()
	{
		return m_targetEncodings;
	}
	
	protected Collection<AudioFormat> getCollectionSourceFormats() {
		return m_sourceFormats;
	}
	
	protected Collection<AudioFormat> getCollectionTargetFormats() {
		return m_targetFormats;
	}
	
	/**
	 * Utility method to check whether these values match, 
	 * taking into account AudioSystem.NOT_SPECIFIED.
	 * @return true if any of the values is AudioSystem.NOT_SPECIFIED 
	 * or both values have the same value.
	 */
	//$$fb 2000-08-16: moved from TEncodingFormatConversionProvider
	protected static boolean doMatch(int i1, int i2) {
		return i1==AudioSystem.NOT_SPECIFIED
			|| i2==AudioSystem.NOT_SPECIFIED
			|| i1==i2;
	}
    
	/**
	 * @see #doMatch(int,int)
	 */
	//$$fb 2000-08-16: moved from TEncodingFormatConversionProvider
	protected static boolean doMatch(float f1, float f2) {
		return f1==AudioSystem.NOT_SPECIFIED
			|| f2==AudioSystem.NOT_SPECIFIED
			|| Math.abs(f1 - f2) < 1.0e-9;
	}

	/**
	 * Utility method, replaces all occurences of AudioSystem.NOT_SPECIFIED
	 * in <code>targetFormat</code> with the corresponding value in <code>sourceFormat</code>.
	 * If <code>targetFormat</code> does not contain any fields with AudioSystem.NOT_SPECIFIED,
	 * it is returned unmodified. The endian-ness and encoding remain the same in all cases.
	 * <p>
	 * If any of the fields is AudioSystem.NOT_SPECIFIED in both <code>sourceFormat</code> and 
	 * <code>targetFormat</code>, it will remain not specified.
	 * <p>
	 * This method uses <code>getFrameSize(...)</code> (see below) to set the new frameSize, 
	 * if a new AudioFormat instance is created.
	 * <p>
	 * This method isn't used in TSimpleFormatConversionProvider - it is solely there
	 * for inheriting classes.
	 */
	//$$fb 2000-08-16: moved from TEncodingFormatConversionProvider
	protected AudioFormat replaceNotSpecified(AudioFormat sourceFormat, AudioFormat targetFormat) {
		boolean bSetSampleSize=false;
		boolean bSetChannels=false;
		boolean bSetSampleRate=false;
		boolean bSetFrameRate=false;
		if (targetFormat.getSampleSizeInBits()==AudioSystem.NOT_SPECIFIED 
		    && sourceFormat.getSampleSizeInBits()!=AudioSystem.NOT_SPECIFIED) {
			bSetSampleSize=true;
		}
		if (targetFormat.getChannels()==AudioSystem.NOT_SPECIFIED 
		    && sourceFormat.getChannels()!=AudioSystem.NOT_SPECIFIED) {
			bSetChannels=true;
		}
		if (targetFormat.getSampleRate()==AudioSystem.NOT_SPECIFIED 
		    && sourceFormat.getSampleRate()!=AudioSystem.NOT_SPECIFIED) {
			bSetSampleRate=true;
		}
		if (targetFormat.getFrameRate()==AudioSystem.NOT_SPECIFIED 
		    && sourceFormat.getFrameRate()!=AudioSystem.NOT_SPECIFIED) {
			bSetFrameRate=true;
		}
		if (bSetSampleSize || bSetChannels || bSetSampleRate || bSetFrameRate 
		    || (targetFormat.getFrameSize()==AudioSystem.NOT_SPECIFIED
			&& sourceFormat.getFrameSize()!=AudioSystem.NOT_SPECIFIED)) {
			// create new format in place of the original target format
			float sampleRate=bSetSampleRate?
				sourceFormat.getSampleRate():targetFormat.getSampleRate();
			float frameRate=bSetFrameRate?
				sourceFormat.getFrameRate():targetFormat.getFrameRate();
			int sampleSize=bSetSampleSize?
				sourceFormat.getSampleSizeInBits():targetFormat.getSampleSizeInBits();
			int channels=bSetChannels?
				sourceFormat.getChannels():targetFormat.getChannels();
			int frameSize=getFrameSize(
						   targetFormat.getEncoding(),
						   sampleRate,
						   sampleSize,
						   channels,
						   frameRate,
						   targetFormat.isBigEndian(),
						   targetFormat.getFrameSize());
			targetFormat= new AudioFormat(
						 targetFormat.getEncoding(),
						 sampleRate,
						 sampleSize,
						 channels,
						 frameSize,
						 frameRate,
						 targetFormat.isBigEndian());
		}
		return targetFormat;
	}

	/**
	 * Calculates the frame size for the given format description.
	 * The default implementation returns AudioSystem.NOT_SPECIFIED
	 * if either <code>sampleSize</code> or <code>channels</code> is AudioSystem.NOT_SPECIFIED,
	 * otherwise <code>sampleSize*channels/8</code> is returned.
	 * <p>
	 * If this does not reflect the way to calculate the right frame size,
	 * inheriting classes should overwrite this method if they use 
	 * replaceNotSpecified(...). It is not used elsewhere in this class.
	 */
	//$$fb 2000-08-16: added
	protected int getFrameSize(
				   AudioFormat.Encoding encoding,
				   float sampleRate,
				   int sampleSize,
				   int channels,
				   float frameRate,
				   boolean bigEndian,
				   int oldFrameSize) {
		if (sampleSize==AudioSystem.NOT_SPECIFIED || channels==AudioSystem.NOT_SPECIFIED) {
			return AudioSystem.NOT_SPECIFIED;
		}
		return sampleSize*channels/8;
	}



}

/*** TSimpleFormatConversionProvider.java ***/
