/*
 *	TControlController.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2001 by Matthias Pfisterer
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

package org.tritonus.share.sampled.mixer;

import java.util.Collection;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Control;
import javax.sound.sampled.Line;
import javax.sound.sampled.LineEvent;
import javax.sound.sampled.LineListener;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.Port;

import org.tritonus.share.TDebug;




/**	Base class for classes implementing Line.
 */
public class TControlController
implements TControllable
{
	/**	The parent (compound) control.
		In case this control is part of a compound control, the parentControl
		property is set to a value other than null.
	 */
	private TCompoundControl	m_parentControl;


	public TControlController()
	{
	}



	public void setParentControl(TCompoundControl compoundControl)
	{
		m_parentControl = compoundControl;
	}


	public TCompoundControl getParentControl()
	{
		return m_parentControl;
	}


	public void commit()
	{
		if (TDebug.TraceControl)
		{
			TDebug.out("TControlController.commit(): called [" + this.getClass().getName() + "]");
		}
		if (getParentControl() != null)
		{
			getParentControl().commit();
		}
	}
}



/*** TControlController.java ***/
