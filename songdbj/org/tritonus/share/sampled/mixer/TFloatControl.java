/*
 *	TFloatControl.java
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

import javax.sound.sampled.FloatControl;

import org.tritonus.share.TDebug;




/**	Base class for classes implementing Line.
 */
public class TFloatControl
extends FloatControl
implements TControllable
{
	private TControlController	m_controller;



	public TFloatControl(FloatControl.Type type,
			     float fMinimum,
			     float fMaximum,
			     float fPrecision,
			     int nUpdatePeriod,
			     float fInitialValue,
			     String strUnits)
	{
		super(type,
		      fMinimum,
		      fMaximum,
		      fPrecision,
		      nUpdatePeriod,
		      fInitialValue,
		      strUnits);
		if (TDebug.TraceControl)
		{
			TDebug.out("TFloatControl.<init>: begin");
		}
		m_controller = new TControlController();
		if (TDebug.TraceControl)
		{
			TDebug.out("TFloatControl.<init>: end");
		}
	}



	public TFloatControl(FloatControl.Type type,
			     float fMinimum,
			     float fMaximum,
			     float fPrecision,
			     int nUpdatePeriod,
			     float fInitialValue,
			     String strUnits,
			     String strMinLabel,
			     String strMidLabel,
			     String strMaxLabel)
	{
		super(type,
		      fMinimum,
		      fMaximum,
		      fPrecision,
		      nUpdatePeriod,
		      fInitialValue,
		      strUnits,
		      strMinLabel,
		      strMidLabel,
		      strMaxLabel);
		if (TDebug.TraceControl)
		{
			TDebug.out("TFloatControl.<init>: begin");
		}
		m_controller = new TControlController();
		if (TDebug.TraceControl)
		{
			TDebug.out("TFloatControl.<init>: end");
		}
	}



	public void setParentControl(TCompoundControl compoundControl)
	{
		m_controller.setParentControl(compoundControl);
	}



	public TCompoundControl getParentControl()
	{
		return m_controller.getParentControl();
	}



	public void commit()
	{
		m_controller.commit();
	}
}



/*** TFloatControl.java ***/
