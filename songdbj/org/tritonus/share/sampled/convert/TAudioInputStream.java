/*
 *	TAudioInputStream.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2003 by Matthias Pfisterer
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

package org.tritonus.share.sampled.convert;

import java.io.InputStream;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;


/** AudioInputStream base class.  This class implements "dynamic"
	properties. "Dynamic" properties are properties that may change
	during the life time of the objects. This is typically used to
	pass information like the current frame number, volume of subbands
	and similar values. "Dynamic" properties are different from
	properties in AudioFormat and AudioFileFormat, which are
	considered "static", as they aren't allowed to change after
	creating of the object, thereby maintaining the immutable
	character of these classes.
*/

public class TAudioInputStream
extends AudioInputStream
{
	private Map<String, Object>	m_properties;
	private Map<String, Object>	m_unmodifiableProperties;


	/** Constructor without properties.
		Creates an empty properties map.
	*/
	public TAudioInputStream(InputStream inputStream,
							 AudioFormat audioFormat,
							 long lLengthInFrames)
	{
		super(inputStream, audioFormat, lLengthInFrames);
		initMaps(new HashMap<String, Object>());
	}


	/** Constructor with properties.
		The passed properties map is not copied. This allows subclasses
		to change values in the map after creation, and the changes are
		reflected in the map the application program can obtain.
	*/
	public TAudioInputStream(InputStream inputStream,
							 AudioFormat audioFormat,
							 long lLengthInFrames,
							 Map<String, Object> properties)
	{
		super(inputStream, audioFormat, lLengthInFrames);
		initMaps(properties);
	}


	private void initMaps(Map<String, Object> properties)
	{
		/* Here, we make a shallow copy of the map. It's unclear if this
		   is sufficient (of if a deep copy should be made).
		*/
		m_properties = properties;
		m_unmodifiableProperties = Collections.unmodifiableMap(m_properties);
	}


	/** Obtain a Map containing the properties.  This method returns a
		Map that cannot be modified by the application program, but
		reflects changes to the map made by the implementation.

		@return a map containing the properties.
	*/
	public Map<String, Object> properties()
	{
		return m_unmodifiableProperties;
	}


	/** Set a property.  Unlike in AudioFormat and AudioFileFormat,
	   this method may be used anywhere by subclasses - it is not
	   restricted to be used in the constructor.
	 */
	protected void setProperty(String key, Object value)
	{
		m_properties.put(key, value);
	}
}



/*** TAudioInputStream.java ***/
