/*
 *	AudioFormatSet.java
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

package org.tritonus.share.sampled;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import javax.sound.sampled.AudioFormat;

import org.tritonus.share.ArraySet;
import org.tritonus.share.sampled.AudioFormats;


/**
 * A set where the elements are uniquely referenced by
 * AudioFormats.equals rather than their object reference.
 * No 2 equal AudioFormats can exist in the set.
 * <p>
 * This class provide convenience methods like
 * <code>getAudioFormat(AudioFormat)</code> and
 * <code>matches(AudioFormat)</code>.
 * <p>
 * The <code>contains(Object elem)</code> and <code>get(Object elem)</code>
 * fail, if elem is not an instance of AudioFormat.
 * <p>
 * You shouldn't use the ArrayList specific functions
 * like those that take index parameters.
 * <p>
 * It is not possible to add <code>null</code> elements.
 * <p>
 * Currently, the methods equals(.,.) and matches(.,.) of
 * class AudioFormats are used. Let's hope that they will
 * be integrated into AudioFormat.
 */

public class AudioFormatSet extends ArraySet<AudioFormat>
{
	protected static final AudioFormat[]		EMPTY_FORMAT_ARRAY = new AudioFormat[0];

	public AudioFormatSet() {
		super();
	}

	public AudioFormatSet(Collection<AudioFormat> c) {
		super(c);
	}

	public boolean add(AudioFormat elem) {
		if (elem==null || !(elem instanceof AudioFormat)) {
			return false;
		}
		return super.add(elem);
	}

	public boolean contains(AudioFormat elem) {
		if (elem==null || !(elem instanceof AudioFormat)) {
			return false;
		}
		AudioFormat comp=(AudioFormat) elem;
		Iterator it=iterator();
		while (it.hasNext()) {
			if (AudioFormats.equals(comp, (AudioFormat) it.next())) {
				return true;
			}
		}
		return false;
	}

	public AudioFormat get(AudioFormat elem) {
		if (elem==null || !(elem instanceof AudioFormat)) {
			return null;
		}
		AudioFormat comp=(AudioFormat) elem;
		Iterator it=iterator();
		while (it.hasNext()) {
			AudioFormat thisElem=(AudioFormat) it.next();
			if (AudioFormats.equals(comp, thisElem)) {
				return thisElem;
			}
		}
		return null;
	}

	public AudioFormat getAudioFormat(AudioFormat elem) {
		return (AudioFormat) get(elem);
	}

	/**
	 * Checks whether this Set contains an AudioFormat
	 * that matches <code>elem</code>.
	 * The first matching format is returned. If no element
	 * matches <code>elem</code>, <code>null</code> is returned.
	 * <p>
	 * @see AudioFormats#matches(AudioFormat, AudioFormat)
	 */
	public AudioFormat matches(AudioFormat elem) {
		if (elem==null) {
			return null;
		}
		Iterator it=iterator();
		while (it.hasNext()) {
			AudioFormat thisElem=(AudioFormat) it.next();
			if (AudioFormats.matches(elem, thisElem)) {
				return thisElem;
			}
		}
		return null;
	}


	// $$mp: TODO: remove; should be obsolete
	public AudioFormat[] toAudioFormatArray() {
		return (AudioFormat[]) toArray(EMPTY_FORMAT_ARRAY);
	}


	public void add(int index, AudioFormat element) {
		throw new UnsupportedOperationException("unsupported");
	}

	public AudioFormat set(int index, AudioFormat element) {
		throw new UnsupportedOperationException("unsupported");
	}
}

/*** AudioFormatSet.java ***/
