/*
 *	TBaseDataLine.java
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
 */

/*
|<---            this code is formatted to fit into 80 columns             --->|
*/

package org.tritonus.share.sampled.mixer;

import java.util.Collection;
import java.util.EventListener;
import java.util.EventObject;
import java.util.HashSet;
import java.util.Set;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Control;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineEvent;
import javax.sound.sampled.Line;
import javax.sound.sampled.LineUnavailableException;

import org.tritonus.share.TDebug;



/**	Base class for implementing SourceDataLine or TargetDataLine.
 */
public abstract class TBaseDataLine
extends	TDataLine
{
	public TBaseDataLine(TMixer mixer,
			     DataLine.Info info)
	{
		super(mixer,
		      info);
	}



	public TBaseDataLine(TMixer mixer,
						 DataLine.Info info,
						 Collection<Control> controls)
	{
		super(mixer,
		      info,
		      controls);
	}



	public void open(AudioFormat format, int nBufferSize)
		throws LineUnavailableException
	{
		if (TDebug.TraceDataLine) { TDebug.out("TBaseDataLine.open(AudioFormat, int): called with buffer size: " + nBufferSize); }
		setBufferSize(nBufferSize);
		open(format);
	}



	public void open(AudioFormat format)
		throws LineUnavailableException
	{
		if (TDebug.TraceDataLine) { TDebug.out("TBaseDataLine.open(AudioFormat): called"); }
		setFormat(format);
		open();
	}


	// IDEA: move to TDataLine or TLine?
	// necessary and wise at all?
	protected void finalize()
	{
		if (isOpen())
		{
			close();
		}
	}
}



/*** TBaseDataLine.java ***/

