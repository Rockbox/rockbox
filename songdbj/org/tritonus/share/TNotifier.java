/*
 *	TNotifier.java
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

package org.tritonus.share;

import java.util.EventObject;
import java.util.Collection;
import java.util.ArrayList;
import java.util.List;
import java.util.Iterator;

import javax.sound.sampled.LineListener;
import javax.sound.sampled.LineEvent;



public class TNotifier
extends	Thread
{
	public static class NotifyEntry
	{
		private EventObject	m_event;
		private List<LineListener>	m_listeners;



		public NotifyEntry(EventObject event, Collection<LineListener> listeners)
		{
			m_event = event;
			m_listeners = new ArrayList<LineListener>(listeners);
		}


		public void deliver()
		{
			// TDebug.out("%% TNotifier.NotifyEntry.deliver(): called.");
			Iterator<LineListener>	iterator = m_listeners.iterator();
			while (iterator.hasNext())
			{
				LineListener	listener = iterator.next();
				listener.update((LineEvent) m_event);
			}
		}
	}


	public static TNotifier	notifier = null;

	static
	{
		notifier = new TNotifier();
		notifier.setDaemon(true);
		notifier.start();
	}



	/**	The queue of events to deliver.
	 *	The entries are of class NotifyEntry.
	 */
	private List<NotifyEntry>	m_entries;


	public TNotifier()
	{
		super("Tritonus Notifier");
		m_entries = new ArrayList<NotifyEntry>();
	}



	public void addEntry(EventObject event, Collection<LineListener> listeners)
	{
		// TDebug.out("%% TNotifier.addEntry(): called.");
		synchronized (m_entries)
		{
			m_entries.add(new NotifyEntry(event, listeners));
			m_entries.notifyAll();
		}
		// TDebug.out("%% TNotifier.addEntry(): completed.");
	}


	public void run()
	{
		while (true)
		{
			NotifyEntry	entry = null;
			synchronized (m_entries)
			{
				while (m_entries.size() == 0)
				{
					try
					{
						m_entries.wait();
					}
					catch (InterruptedException e)
					{
						if (TDebug.TraceAllExceptions)
						{
							TDebug.out(e);
						}
					}
				}
				entry = m_entries.remove(0);
			}
			entry.deliver();
		}
	}
}


/*** TNotifier.java ***/
