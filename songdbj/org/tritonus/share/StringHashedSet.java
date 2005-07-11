/*
 *	StringHashedSet.java
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

package org.tritonus.share;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.tritonus.share.ArraySet;


/**
 * A set where the elements are uniquely referenced by their
 * string representation as given by the objects toString()
 * method. No 2 objects with the same toString() can
 * be in the set.
 * <p>
 * The <code>contains(Object elem)</code> and <code>get(Object elem)</code>
 * methods can be called with Strings as <code>elem</code> parameter.
 * For <code>get(Object elem)</code>, the object that has been added
 * is returned, and not its String representation.
 * <p>
 * Though it's possible to store
 * Strings as objects in this class, it doesn't make sense
 * as you could use ArraySet for that equally well.
 * <p>
 * You shouldn't use the ArrayList specific functions
 * like those that take index parameters.
 * <p>
 * It is not possible to add <code>null</code> elements.
 */

public class StringHashedSet<E> extends ArraySet<E>
{
	public StringHashedSet()
	{
		super();
	}

	public StringHashedSet(Collection<E> c)
	{
		super(c);
	}

	public boolean add(E elem)
	{
		if (elem==null) {
			return false;
		}
		return super.add(elem);
	}

	public boolean contains(Object elem)
	{
		if (elem==null) {
			return false;
		}
		String comp=elem.toString();
		Iterator<E> it=iterator();
		while (it.hasNext()) {
			if (comp.equals(it.next().toString())) {
				return true;
			}
		}
		return false;
	}

	public E get(E elem) {
		if (elem==null) {
			return null;
		}
		String comp=elem.toString();
		Iterator<E> it=iterator();
		while (it.hasNext()) {
			E thisElem=it.next();
			if (comp.equals(thisElem.toString())) {
				return thisElem;
			}
		}
		return null;
	}
}

/*** StringHashedSet.java ***/
