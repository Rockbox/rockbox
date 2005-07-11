/*
 *	TEncodingFormatConversionProvider.java
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

package org.tritonus.share.sampled.convert;

import java.util.Collection;
import java.util.Iterator;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;

import org.tritonus.share.TDebug;
import org.tritonus.share.ArraySet;


// this class depends on handling of AudioSystem.NOT_SPECIFIED in AudioFormat.matches()

/**
 * This is a base class for FormatConversionProviders that only
 * change the encoding, i.e. they never
 * <ul>
 * <li> change the sample size in bits without changing the encoding
 * <li> change the sample rate
 * <li> change the number of channels
 * </ul>
 * <p>It is assumed that each source format can be encoded to all
 * target formats.
 * <p>In the sourceFormats and targetFormats collections that are passed to
 * the constructor of this class, fields may be set to AudioSystem.NOT_SPECIFIED.
 * This means that it handles all values of that field, but cannot change it.
 * <p>This class prevents that a conversion is done (e.g. for sample rates),
 * because the overriding class specified AudioSystem.NOT_SPECIFIED as sample rate,
 * meaning it handles all sample rates.
 * <p>Overriding classes must implement at least
 * <code>AudioInputStream getAudioInputStream(AudioFormat targetFormat, AudioInputStream sourceStream)</code>
 * and provide a constructor that calls the protected constructor of this class.
 *
 * @author Florian Bomers
 */
public abstract class TEncodingFormatConversionProvider
extends TSimpleFormatConversionProvider
{
	protected TEncodingFormatConversionProvider(
	    Collection<AudioFormat> sourceFormats,
	    Collection<AudioFormat> targetFormats)
	{
		super(sourceFormats, targetFormats);
	}



	/**
	 * This implementation assumes that the converter can convert
	 * from each of its source formats to each of its target
	 * formats. If this is not the case, the converter has to
	 * override this method.
	 * <p>When conversion is supported, for every target encoding,
	 * the fields sample size in bits, channels and sample rate are checked:
	 * <ul>
	 * <li>When a field in both the source and target format is AudioSystem.NOT_SPECIFIED,
	 *     one instance of that targetFormat is returned with this field set to AudioSystem.NOT_SPECIFIED.
	 * <li>When a field in sourceFormat is set and it is AudioSystem.NOT_SPECIFIED in the target format,
	 *     the value of the field of source format is set in the returned format.
	 * <li>The same applies for the other way round.
	 * </ul>
	 * For this, <code>replaceNotSpecified(sourceFormat, targetFormat)</code> in the base
	 * class TSimpleFormatConversionProvider is used - and accordingly, the frameSize
	 * is recalculated with <code>getFrameSize(...)</code> if a field with AudioSystem.NOT_SPECIFIED
	 * is replaced. Inheriting classes may wish to override this method if the
	 * default mode of calculating the frame size is not appropriate.
	 */
	public AudioFormat[] getTargetFormats(AudioFormat.Encoding targetEncoding, AudioFormat sourceFormat) {
		if (TDebug.TraceAudioConverter) {
			TDebug.out(">TEncodingFormatConversionProvider.getTargetFormats(AudioFormat.Encoding, AudioFormat):");
			TDebug.out("checking if conversion possible");
			TDebug.out("from: " + sourceFormat);
			TDebug.out("to: " + targetEncoding);
		}
		if (isConversionSupported(targetEncoding, sourceFormat)) {
			// TODO: check that no duplicates may occur...
			ArraySet<AudioFormat>	result=new ArraySet<AudioFormat>();
			Iterator<AudioFormat>	iterator = getCollectionTargetFormats().iterator();
			while (iterator.hasNext()) {
				AudioFormat	targetFormat = iterator.next();
				targetFormat=replaceNotSpecified(sourceFormat, targetFormat);
				result.add(targetFormat);
			}
			if (TDebug.TraceAudioConverter) {
				TDebug.out("< returning "+result.size()+" elements.");
			}
			return result.toArray(EMPTY_FORMAT_ARRAY);
		} else {
			if (TDebug.TraceAudioConverter) {
				TDebug.out("< returning empty array.");
			}
			return EMPTY_FORMAT_ARRAY;
		}
	}

}

/*** TEncodingFormatConversionProvider.java ***/
