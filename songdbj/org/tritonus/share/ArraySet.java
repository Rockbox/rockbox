/*
 *	ArraySet.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 -2004 by Matthias Pfisterer
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
import java.util.Set;



public class ArraySet<E>
extends ArrayList<E>
implements Set<E>
{
	public ArraySet()
	{
		super();
	}



	public ArraySet(Collection<E> c)
	{
		this();
		addAll(c);
	}



	public boolean add(E element)
	{
		if (!contains(element))
		{
			super.add(element);
			return true;
		}
		else
		{
			return false;
		}
	}



	public void add(int index, E element)
	{
		throw new UnsupportedOperationException("ArraySet.add(int index, Object element) unsupported");
	}

	public E set(int index, E element)
	{
		throw new UnsupportedOperationException("ArraySet.set(int index, Object element) unsupported");
	}

}



/*** ArraySet.java ***/
