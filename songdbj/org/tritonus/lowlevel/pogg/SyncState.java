/*
 *	SyncState.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 - 2005 by Matthias Pfisterer
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

package org.tritonus.lowlevel.pogg;

import org.tritonus.share.TDebug;


/** Wrapper for ogg_sync_state.
 */
public class SyncState
{
	/** The stream buffer.
		This points to a buffer that holds the incoming data from a stream
		until the data is emitted in form of a page.
	*/
	private byte[]	m_abData;

	/** The number of bytes in the stream buffer that are used.  This
		always counts from the beginning of the buffer. So
		m_abData[m_nFill] is the first free byte in the buffer.
	*/
	private int		m_nFill;

	/** Number of bytes that have been used to construct a returned
		page.  This is counted from the beginning of the
		buffer. m_abData[m_nReturned] is the first valid byte in the
		buffer.
	 */
	private int		m_nReturned;

	/**
	 */
	private boolean	m_bUnsynced;

	/**
	 */
	private int		m_nHeaderBytes;

	/**
	 */
	private int		m_nBodyBytes;

	/** Page object for re-calculating the checksum.
	 */
	private Page	m_tmpPage;

	/** Storage for the checksum saved from the page.
	 */
	private byte[]	m_abChecksum;


	public SyncState()
	{
		if (TDebug.TraceOggNative) { TDebug.out("SyncState.<init>(): begin"); }
		m_tmpPage = new Page();
		m_abChecksum = new byte[4];
		if (TDebug.TraceOggNative) { TDebug.out("SyncState.<init>(): end"); }
	}


	// TODO: remove calls to this method
	public void free()
	{
	}



	/** Calls ogg_sync_init().
	 */
	public void init()
	{
		m_abData = null;
		m_nFill = 0;
		m_nReturned = 0;
		m_bUnsynced = false;
		m_nHeaderBytes = 0;
		m_nBodyBytes = 0;
	}


	/** Calls ogg_sync_clear().
	 */
	public void clear()
	{
		init();
	}


	/** Calls ogg_sync_reset().
	 */
	public void reset()
	{
		m_nFill = 0;
		m_nReturned = 0;
		m_bUnsynced = false;
		m_nHeaderBytes = 0;
		m_nBodyBytes = 0;
	}


	/** Writes to the stream buffer.
	 */
	public int write(byte[] abBuffer, int nBytes)
	{
		/* Clear out space that has been previously returned. */
		if (m_nReturned > 0)
		{
			m_nFill -= m_nReturned;
			if (m_nFill > 0)
			{
				System.arraycopy(m_abData, m_nReturned,
								 m_abData, 0,
								 m_nFill);
			}
			m_nReturned = 0;
		}

		/* Check for enough space in the stream buffer and if it is
		 * allocated at all. If there isn't sufficient space, extend
		 * the buffer. */
		if (m_abData == null || nBytes > m_abData.length - m_nFill)
		{
			int nNewSize = nBytes + m_nFill + 4096;
			byte[] abOldBuffer = m_abData;
			m_abData = new byte[nNewSize];
			if (abOldBuffer != null)
			{
				System.arraycopy(abOldBuffer, 0, m_abData, 0, m_nFill);
			}
		}

		/* Now finally fill with the new data. */
		System.arraycopy(abBuffer, 0, m_abData, m_nFill, nBytes);
		m_nFill += nBytes;
		return 0;
	}


	/** Synchronizes the stream.  This method looks if there is a
		complete, valid page in the stream buffer. If a page is found,
		it is returned in the passed Page object.

		@param page The Page to store the result of the page search
		in. The content is only changed if the return value is > 0.

		@return if a page has been found at the current location, the
		length of the page in bytes is returned. If not enough data
		for a page is available in the stream buffer, 0 is
		returned. If data in the stream buffer has been skipped
		because there is no page at the current position, the skip
		amount in bytes is returned as a negative number.
	 */
	public int pageseek(Page page)
	{
		int nPage = m_nReturned;
		int nBytes = m_nFill - m_nReturned;

		if (m_nHeaderBytes == 0)
		{
			if (nBytes < 27)
			{
				/* Not enough data for a header. */
				return 0;
			}
			/* Verify capture pattern. */
			if (m_abData[nPage] != (byte) 'O' ||
				m_abData[nPage + 1] != (byte) 'g' ||
				m_abData[nPage + 2] != (byte) 'g' ||
				m_abData[nPage + 3] != (byte) 'S')
			{
				TDebug.out("wrong capture pattern");
				return syncFailure();
			}
			int nHeaderBytes = (m_abData[nPage + 26] & 0xFF) + 27;
			if (nBytes < nHeaderBytes)
			{
				/* Not enough data for header + segment table. */
				return 0;
			}
			/* Count up body length in the segment table. */
			for (int i = 0; i < (m_abData[nPage + 26] & 0xFF); i++)
			{
				m_nBodyBytes += (m_abData[nPage + 27 + i] & 0xFF);
			}
			m_nHeaderBytes = nHeaderBytes;
		}

		if (m_nBodyBytes + m_nHeaderBytes > nBytes)
		{
			/* Not enough data for the whole packet. */
			return 0;
		}

		/* Save the original checksum, set it to zero and recalculate it. */
		System.arraycopy(m_abData, nPage + 22, m_abChecksum, 0, 4);
		m_abData[nPage + 22] = 0;
		m_abData[nPage + 23] = 0;
		m_abData[nPage + 24] = 0;
		m_abData[nPage + 25] = 0;

		m_tmpPage.setData(m_abData, nPage, m_nHeaderBytes,
					m_abData, nPage + m_nHeaderBytes, m_nBodyBytes);
// 		TDebug.out("temporary page:");
// 		byte[] abHeader = m_tmpPage.getHeader();
// 			TDebug.out("H0:" + m_abData[nPage + 0] + " - " + abHeader[0]);
// 			TDebug.out("H1:" + m_abData[nPage + 1] + " - " + abHeader[1]);
// 			TDebug.out("H2:" + m_abData[nPage + 2] + " - " + abHeader[2]);
// 			TDebug.out("H3:" + m_abData[nPage + 3] + " - " + abHeader[3]);
// 			TDebug.out("" + m_abChecksum[0] + " - " + abHeader[22]);
// 			TDebug.out("" + m_abChecksum[1] + " - " + abHeader[23]);
// 			TDebug.out("" + m_abChecksum[2] + " - " + abHeader[24]);
// 			TDebug.out("" + m_abChecksum[3] + " - " + abHeader[25]);
		m_tmpPage.setChecksum();
		byte[] abHeader = m_tmpPage.getHeader();
		//m_tmpPage.free();
		if (abHeader[22] != m_abChecksum[0] ||
			abHeader[23] != m_abChecksum[1] ||
			abHeader[24] != m_abChecksum[2] ||
			abHeader[25] != m_abChecksum[3])
		{
			TDebug.out("wrong checksum");
			TDebug.out("" + m_abChecksum[0] + " - " + abHeader[22]);
			TDebug.out("" + m_abChecksum[1] + " - " + abHeader[23]);
			TDebug.out("" + m_abChecksum[2] + " - " + abHeader[24]);
			TDebug.out("" + m_abChecksum[3] + " - " + abHeader[25]);
			/* Copy back the saved checksum. */
			System.arraycopy(m_abChecksum, 0, m_abData, nPage + 22, 4);
			return syncFailure();
		}

		/* Ok, we have a correct page to emit. */
		page.setData(m_abData, nPage, m_nHeaderBytes,
					 m_abData, nPage + m_nHeaderBytes, m_nBodyBytes);
		m_bUnsynced = false;
		nBytes = m_nHeaderBytes + m_nBodyBytes;
		m_nReturned += nBytes;
		m_nHeaderBytes = 0;
		m_nBodyBytes = 0;
		return nBytes;
	}


	/** Auxiliary method for pageseek().
	 */
	private int syncFailure()
	{
		int nPage = m_nReturned;
		int nBytes = m_nFill - m_nReturned;
		m_nHeaderBytes = 0;
		m_nBodyBytes = 0;
		int nNext = -1;
		for (int i = 0; i < nBytes - 1; i++)
		{
			if (m_abData[nPage + 1 + i] == (byte) 'O')
			{
				nNext = nPage + 1 + i;
				break;
			}
		}
		if (nNext == -1)
		{
			nNext = m_nFill;
		}
		m_nReturned = nNext;
		return -(nNext - nPage);
	}




	/** Returns a page from the stream buffer, if possible.  This
		method searches the current data in the stream buffer for the
		beginning of a page. If there is one, it is returned in the
		passed Page object. A synchronization error is signaled by a
		return value of -1. However, only the first synchronization
		error is reported. Consecutive sync errors are suppressed.

		@param page The Page to store the result of the page search
		in. The content is only changed if the return value is 1.

		@return 1 if a page has been found, 0 if there is not enough
		data, -1 if a synchronization error occured.
	 */
	public int pageOut(Page page)
	{
		while (true)
		{
			int nReturn = pageseek(page);
			if (nReturn > 0)
			{
				return 1;
			}
			else if (nReturn == 0)
			{
				return 0;
			}
			else // nReturn < 0
			{
				if (! m_bUnsynced)
				{
					m_bUnsynced = true;
					return -1;
				}
			}
		}
	}
}





/*** SyncState.java ***/
