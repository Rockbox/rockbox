/*
 *	TLine.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 - 2004 by Matthias Pfisterer
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

import org.tritonus.share.TDebug;
import org.tritonus.share.TNotifier;




/**	Base class for classes implementing Line.
 */
public abstract class TLine
implements Line
{
	private static final Control[]	EMPTY_CONTROL_ARRAY = new Control[0];

	private Line.Info	m_info;
	private boolean		m_bOpen;
	private List<Control>	m_controls;
	private Set<LineListener>	m_lineListeners;
	private TMixer		m_mixer;



	protected TLine(TMixer mixer,
			Line.Info info)
	{
		setLineInfo(info);
		setOpen(false);
		m_controls = new ArrayList<Control>();
		m_lineListeners = new HashSet<LineListener>();
		m_mixer = mixer;
	}



	protected TLine(TMixer mixer,
					Line.Info info,
					Collection<Control> controls)
	{
		this (mixer, info);
		m_controls.addAll(controls);
	}


	protected TMixer getMixer()
	{
		return m_mixer;
	}


	public Line.Info getLineInfo()
	{
		return m_info;
	}



	protected void setLineInfo(Line.Info info)
	{
		if (TDebug.TraceLine)
		{
			TDebug.out("TLine.setLineInfo(): setting: " + info);
		}
		synchronized (this)
		{
			m_info = info;
		}
	}



	public void open()
		throws LineUnavailableException
	{
		if (TDebug.TraceLine)
		{
			TDebug.out("TLine.open(): called");
		}
		if (! isOpen())
		{
			if (TDebug.TraceLine)
			{
				TDebug.out("TLine.open(): opening");
			}
			openImpl();
			if (getMixer() != null)
			{
				getMixer().registerOpenLine(this);
			}
			setOpen(true);
		}
		else
		{
			if (TDebug.TraceLine)
			{
				TDebug.out("TLine.open(): already open");
			}
		}
	}



	/**
	 *	Subclasses should override this method.
	 */
	protected void openImpl()
		throws LineUnavailableException
	{
		if (TDebug.TraceLine)
		{
			TDebug.out("TLine.openImpl(): called");
		}
	}



	public void close()
	{
		if (TDebug.TraceLine)
		{
			TDebug.out("TLine.close(): called");
		}
		if (isOpen())
		{
			if (TDebug.TraceLine)
			{
				TDebug.out("TLine.close(): closing");
			}
			if (getMixer() != null)
			{
				getMixer().unregisterOpenLine(this);
			}
			closeImpl();
			setOpen(false);
		}
		else
		{
			if (TDebug.TraceLine)
			{
				TDebug.out("TLine.close(): not open");
			}
		}
	}



	/**
	 *	Subclasses should override this method.
	 */
	protected void closeImpl()
	{
		if (TDebug.TraceLine)
		{
			TDebug.out("TLine.closeImpl(): called");
		}
	}





	public boolean isOpen()
	{
		return m_bOpen;
	}




	protected void setOpen(boolean bOpen)
	{
		if (TDebug.TraceLine)
		{
			TDebug.out("TLine.setOpen(): called, value: " + bOpen);
		}
		boolean	bOldValue = isOpen();
		m_bOpen = bOpen;
		if (bOldValue != isOpen())
		{
			if (isOpen())
			{
				if (TDebug.TraceLine)
				{
					TDebug.out("TLine.setOpen(): opened");
				}
				notifyLineEvent(LineEvent.Type.OPEN);
			}
			else
			{
				if (TDebug.TraceLine)
				{
					TDebug.out("TLine.setOpen(): closed");
				}
				notifyLineEvent(LineEvent.Type.CLOSE);
			}
		}
	}



	protected void addControl(Control control)
	{
		synchronized (m_controls)
		{
			m_controls.add(control);
		}
	}



	protected void removeControl(Control control)
	{
		synchronized (m_controls)
		{
			m_controls.remove(control);
		}
	}



	public Control[] getControls()
	{
		synchronized (m_controls)
		{
			return m_controls.toArray(EMPTY_CONTROL_ARRAY);
		}
	}



	public Control getControl(Control.Type controlType)
	{
		synchronized (m_controls)
		{
			Iterator<Control>	it = m_controls.iterator();
			while (it.hasNext())
			{
				Control	control = it.next();
				if (control.getType().equals(controlType))
				{
					return control;
				}
			}
			throw new IllegalArgumentException("no control of type " + controlType);
		}
	}



	public boolean isControlSupported(Control.Type controlType)
	{
		// TDebug.out("TLine.isSupportedControl(): called");
		try
		{
			return getControl(controlType) != null;
		}
		catch (IllegalArgumentException e)
		{
			if (TDebug.TraceAllExceptions)
			{
				TDebug.out(e);
			}
			// TDebug.out("TLine.isSupportedControl(): returning false");
			return false;
		}
	}



	public void addLineListener(LineListener listener)
	{
		// TDebug.out("%% TChannel.addListener(): called");
		synchronized (m_lineListeners)
		{
			m_lineListeners.add(listener);
		}
	}



	public void removeLineListener(LineListener listener)
	{
		synchronized (m_lineListeners)
		{
			m_lineListeners.remove(listener);
		}
	}



	private Set<LineListener> getLineListeners()
	{
		synchronized (m_lineListeners)
		{
			return new HashSet<LineListener>(m_lineListeners);
		}
	}


	// is overridden in TDataLine to provide a position
	protected void notifyLineEvent(LineEvent.Type type)
	{
		notifyLineEvent(new LineEvent(this, type, AudioSystem.NOT_SPECIFIED));
	}



	protected void notifyLineEvent(LineEvent event)
	{
		// TDebug.out("%% TChannel.notifyChannelEvent(): called");
		// Channel.Event	event = new Channel.Event(this, type, getPosition());
		TNotifier.notifier.addEntry(event, getLineListeners());
	}
}



/*** TLine.java ***/
