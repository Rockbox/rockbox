/*
 *	TCircularBuffer.java
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

import org.tritonus.share.TDebug;



public class TCircularBuffer
{
	private boolean		m_bBlockingRead;
	private boolean		m_bBlockingWrite;
	private byte[]		m_abData;
	private int			m_nSize;
	private long		m_lReadPos;
	private long		m_lWritePos;
	private Trigger		m_trigger;
	private boolean		m_bOpen;



	public TCircularBuffer(int nSize, boolean bBlockingRead, boolean bBlockingWrite, Trigger trigger)
	{
		m_bBlockingRead = bBlockingRead;
		m_bBlockingWrite = bBlockingWrite;
		m_nSize = nSize;
		m_abData = new byte[m_nSize];
		m_lReadPos = 0;
		m_lWritePos = 0;
		m_trigger = trigger;
		m_bOpen = true;
	}



	public void close()
	{
		m_bOpen = false;
		// TODO: call notify() ?
	}



	private boolean isOpen()
	{
		return m_bOpen;
	}


	public int availableRead()
	{
		return (int) (m_lWritePos - m_lReadPos);
	}



	public int availableWrite()
	{
		return m_nSize - availableRead();
	}



	private int getReadPos()
	{
		return (int) (m_lReadPos % m_nSize);
	}



	private int getWritePos()
	{
		return (int) (m_lWritePos % m_nSize);
	}



	public int read(byte[] abData)
	{
		return read(abData, 0, abData.length);
	}



	public int read(byte[] abData, int nOffset, int nLength)
	{
		if (TDebug.TraceCircularBuffer)
		{
			TDebug.out(">TCircularBuffer.read(): called.");
			dumpInternalState();
		}
		if (! isOpen())
		{
			if (availableRead() > 0)
			{
				nLength = Math.min(nLength, availableRead());
				if (TDebug.TraceCircularBuffer) { TDebug.out("reading rest in closed buffer, length: " + nLength); }
			}
			else
			{
				if (TDebug.TraceCircularBuffer) { TDebug.out("< not open. returning -1."); }
				return -1;
			}
		}
		synchronized (this)
		{
			if (m_trigger != null && availableRead() < nLength)
			{
				if (TDebug.TraceCircularBuffer) { TDebug.out("executing trigger."); }
				m_trigger.execute();
			}
			if (!m_bBlockingRead)
			{
				nLength = Math.min(availableRead(), nLength);
			}
			int nRemainingBytes = nLength;
			while (nRemainingBytes > 0)
			{
				while (availableRead() == 0)
				{
					try
					{
						wait();
					}
					catch (InterruptedException e)
					{
						if (TDebug.TraceAllExceptions)
						{
							TDebug.out(e);
						}
					}
				}
				int	nAvailable = Math.min(availableRead(), nRemainingBytes);
				while (nAvailable > 0)
				{
					int	nToRead = Math.min(nAvailable, m_nSize - getReadPos());
					System.arraycopy(m_abData, getReadPos(), abData, nOffset, nToRead);
					m_lReadPos += nToRead;
					nOffset += nToRead;
					nAvailable -= nToRead;
					nRemainingBytes -= nToRead;
				}
				notifyAll();
			}
			if (TDebug.TraceCircularBuffer)
			{
				TDebug.out("After read:");
				dumpInternalState();
				TDebug.out("< completed. Read " + nLength + " bytes");
			}
			return nLength;
		}
	}


	public int write(byte[] abData)
	{
		return write(abData, 0, abData.length);
	}



	public int write(byte[] abData, int nOffset, int nLength)
	{
		if (TDebug.TraceCircularBuffer)
		{
			TDebug.out(">TCircularBuffer.write(): called; nLength: " + nLength);
			dumpInternalState();
		}
		synchronized (this)
		{
			if (TDebug.TraceCircularBuffer) { TDebug.out("entered synchronized block."); }
			if (!m_bBlockingWrite)
			{
				nLength = Math.min(availableWrite(), nLength);
			}
			int nRemainingBytes = nLength;
			while (nRemainingBytes > 0)
			{
				while (availableWrite() == 0)
				{
					try
					{
						wait();
					}
					catch (InterruptedException e)
					{
						if (TDebug.TraceAllExceptions)
						{
							TDebug.out(e);
						}
					}
				}
				int	nAvailable = Math.min(availableWrite(), nRemainingBytes);
				while (nAvailable > 0)
				{
					int	nToWrite = Math.min(nAvailable, m_nSize - getWritePos());
					//TDebug.out("src buf size= " + abData.length + ", offset = " + nOffset + ", dst buf size=" + m_abData.length + " write pos=" + getWritePos() + " len=" + nToWrite);
					System.arraycopy(abData, nOffset, m_abData, getWritePos(), nToWrite);
					m_lWritePos += nToWrite;
					nOffset += nToWrite;
					nAvailable -= nToWrite;
					nRemainingBytes -= nToWrite;
				}
				notifyAll();
			}
			if (TDebug.TraceCircularBuffer)
			{
				TDebug.out("After write:");
				dumpInternalState();
				TDebug.out("< completed. Wrote "+nLength+" bytes");
			}
			return nLength;
		}
	}



	private void dumpInternalState()
	{
		TDebug.out("m_lReadPos  = " + m_lReadPos + " ^= "+getReadPos());
		TDebug.out("m_lWritePos = " + m_lWritePos + " ^= "+getWritePos());
		TDebug.out("availableRead()  = " + availableRead());
		TDebug.out("availableWrite() = " + availableWrite());
	}



	public static interface Trigger
	{
		public void execute();
	}


}



/*** TCircularBuffer.java ***/

