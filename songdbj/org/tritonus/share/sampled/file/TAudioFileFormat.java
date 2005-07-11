/*
 *	TAudioFileFormat.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 by Matthias Pfisterer
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

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;



/**
 * This class is just to have a public constructor taking the
 * number of bytes of the whole file. The public constructor of
 * AudioFileFormat doesn't take this parameter, the one who takes
 * it is protected.
 *
 * @author Matthias Pfisterer
 */
public class TAudioFileFormat
extends	AudioFileFormat
{
	private Map<String, Object>	m_properties;
	private Map<String, Object>	m_unmodifiableProperties;


	/*
	 *	Note that the order of the arguments is different from
	 *	the one in AudioFileFormat.
	 */
	public TAudioFileFormat(Type type,
							AudioFormat audioFormat,
							int nLengthInFrames,
							int nLengthInBytes)
	{
		super(type,
			  nLengthInBytes,
			  audioFormat,
			  nLengthInFrames);
	}


	public TAudioFileFormat(Type type,
							AudioFormat audioFormat,
							int nLengthInFrames,
							int nLengthInBytes,
							Map<String, Object> properties)
	{
		super(type,
			  nLengthInBytes,
			  audioFormat,
			  nLengthInFrames);
		initMaps(properties);
	}


	private void initMaps(Map<String, Object> properties)
	{
		/* Here, we make a shallow copy of the map. It's unclear if this
		   is sufficient (of if a deep copy should be made).
		*/
		m_properties = new HashMap<String, Object>();
		m_properties.putAll(properties);
		m_unmodifiableProperties = Collections.unmodifiableMap(m_properties);
	}


	public Map<String, Object> properties()
	{
		return m_unmodifiableProperties;
	}



	protected void setProperty(String key, Object value)
	{
		m_properties.put(key, value);
	}
}



/*** TAudioFileFormat.java ***/
