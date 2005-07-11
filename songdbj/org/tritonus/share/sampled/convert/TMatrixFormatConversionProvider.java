/*
 *	TMatrixFormatConversionProvider.java
 */

/*
 *  Copyright (c) 1999, 2000 by Matthias Pfisterer <Matthias.Pfisterer@gmx.de>
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


package	org.tritonus.share.sampled.convert;


import	java.util.Collection;
import	java.util.List;
import	java.util.Map;
import	java.util.Set;
import	java.util.HashMap;
import	java.util.ArrayList;
import	java.util.Iterator;

import	javax.sound.sampled.AudioFormat;
import	javax.sound.sampled.AudioInputStream;
import	javax.sound.sampled.spi.FormatConversionProvider;

import	org.tritonus.share.sampled.AudioFormats;
import	org.tritonus.share.ArraySet;

/**
 * Base class for arbitrary formatConversionProviders.
 *
 * @author Matthias Pfisterer
 */


public abstract class TMatrixFormatConversionProvider
	extends		TSimpleFormatConversionProvider
{
	/*
	 *	keys: source AudioFormat
	 *	values: collection of possible target encodings
	 *
	 *	Note that accessing values with get() is not appropriate,
	 *	since the equals() method in AudioFormat is not overloaded.
	 *	The hashtable is just used as a convenient storage
	 *	organization.
	 */
	private Map	m_targetEncodingsFromSourceFormat;


	/*
	 *	keys: source AudioFormat
	 *	values: a Map that contains a mapping from target encodings
	 *	(keys) to a collection of target formats (values).
	 *
	 *	Note that accessing values with get() is not appropriate,
	 *	since the equals() method in AudioFormat is not overloaded.
	 *	The hashtable is just used as a convenient storage
	 *	organization.
	 */
	private Map	m_targetFormatsFromSourceFormat;



	protected TMatrixFormatConversionProvider(
		List sourceFormats,
		List targetFormats,
		boolean[][] abConversionPossible)
	{
		super(sourceFormats,
		      targetFormats);
		m_targetEncodingsFromSourceFormat = new HashMap();
		m_targetFormatsFromSourceFormat = new HashMap();

		for (int nSourceFormat = 0;
		     nSourceFormat < sourceFormats.size();
		     nSourceFormat++)
		{
			AudioFormat	sourceFormat = (AudioFormat) sourceFormats.get(nSourceFormat);
			List	supportedTargetEncodings = new ArraySet();
			m_targetEncodingsFromSourceFormat.put(sourceFormat, supportedTargetEncodings);
			Map	targetFormatsFromTargetEncodings = new HashMap();
			m_targetFormatsFromSourceFormat.put(sourceFormat, targetFormatsFromTargetEncodings);
			for (int nTargetFormat = 0;
			     nTargetFormat < targetFormats.size();
			     nTargetFormat++)
			{
				AudioFormat	targetFormat = (AudioFormat) targetFormats.get(nTargetFormat);
				if (abConversionPossible[nSourceFormat][nTargetFormat])
				{
					AudioFormat.Encoding	targetEncoding = targetFormat.getEncoding();
					supportedTargetEncodings.add(targetEncoding);
					Collection	supportedTargetFormats = (Collection) targetFormatsFromTargetEncodings.get(targetEncoding);
					if (supportedTargetFormats == null)
					{
						supportedTargetFormats = new ArraySet();
						targetFormatsFromTargetEncodings.put(targetEncoding, supportedTargetFormats);
					}
					supportedTargetFormats.add(targetFormat);
				}
			}
		}
	}



	public AudioFormat.Encoding[] getTargetEncodings(AudioFormat sourceFormat)
	{
		Iterator	iterator = m_targetEncodingsFromSourceFormat.entrySet().iterator();
		while (iterator.hasNext())
		{
			Map.Entry	entry = (Map.Entry) iterator.next();
			AudioFormat	format = (AudioFormat) entry.getKey();
			if (AudioFormats.matches(format, sourceFormat))
			{
				Collection	targetEncodings = (Collection) entry.getValue();
				return (AudioFormat.Encoding[]) targetEncodings.toArray(EMPTY_ENCODING_ARRAY);
			}
			
		}
		return EMPTY_ENCODING_ARRAY;
	}



	// TODO: this should work on the array returned by getTargetEncodings(AudioFormat)
/*
  public boolean isConversionSupported(AudioFormat.Encoding targetEncoding, AudioFormat sourceFormat)
  {
  return isAllowedSourceFormat(sourceFormat) &&
  isTargetEncodingSupported(targetEncoding);
  }
*/



	public AudioFormat[] getTargetFormats(AudioFormat.Encoding targetEncoding, AudioFormat sourceFormat)
	{
		Iterator	iterator = m_targetFormatsFromSourceFormat.entrySet().iterator();
		while (iterator.hasNext())
		{
			Map.Entry	entry = (Map.Entry) iterator.next();
			AudioFormat	format = (AudioFormat) entry.getKey();
			if (AudioFormats.matches(format, sourceFormat))
			{
				Map	targetEncodings = (Map) entry.getValue();
				Collection	targetFormats = (Collection) targetEncodings.get(targetEncoding);
				if (targetFormats != null)
				{
					return (AudioFormat[]) targetFormats.toArray(EMPTY_FORMAT_ARRAY);
				}
				else
				{
					return EMPTY_FORMAT_ARRAY;
				}
			}
			
		}
		return EMPTY_FORMAT_ARRAY;
	}


}



/*** TMatrixFormatConversionProvider.java ***/
