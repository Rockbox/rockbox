/*
 *	TSoftClip.java
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

package org.tritonus.share.sampled.mixer;

import java.io.IOException;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Clip;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.SourceDataLine;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.Mixer;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.mixer.TDataLine;



public class TSoftClip
extends TClip
implements Runnable
{
	private static final Class[]	CONTROL_CLASSES = {/*GainControl.class*/};
	private static final int	BUFFER_SIZE = 16384;


	private Mixer			m_mixer;
	private SourceDataLine		m_line;
	private byte[]			m_abClip;
	private int			m_nRepeatCount;
	private Thread			m_thread;

	public TSoftClip(Mixer mixer, AudioFormat format)
		throws LineUnavailableException
	{
		// TODO: info object
/*
		DataLine.Info	info = new DataLine.Info(Clip.class,
							 audioFormat, -1);
*/
		super(null);
		m_mixer = mixer;
		DataLine.Info	info = new DataLine.Info(
			SourceDataLine.class,
			// TODO: should pass a real AudioFormat object that isn't too restrictive
			format);
		m_line = (SourceDataLine) AudioSystem.getLine(info);
	}



	public void open(AudioInputStream audioInputStream)
		throws LineUnavailableException, IOException
	{
		AudioFormat	audioFormat = audioInputStream.getFormat();
		setFormat(audioFormat);
		int	nFrameSize = audioFormat.getFrameSize();
		if (nFrameSize < 1)
		{
			throw new IllegalArgumentException("frame size must be positive");
		}
		if (TDebug.TraceClip)
		{
			TDebug.out("TSoftClip.open(): format: " + audioFormat);
			// TDebug.out("sample rate: " + audioFormat.getSampleRate());
		}
		byte[]	abData = new byte[BUFFER_SIZE];
		ByteArrayOutputStream	baos = new ByteArrayOutputStream();
		int	nBytesRead = 0;
		while (nBytesRead != -1)
		{
			try
			{
				nBytesRead = audioInputStream.read(abData, 0, abData.length);
			}
			catch (IOException e)
			{
				if (TDebug.TraceClip || TDebug.TraceAllExceptions)
				{
					TDebug.out(e);
				}
			}
			if (nBytesRead >= 0)
			{
				if (TDebug.TraceClip)
				{
					TDebug.out("TSoftClip.open(): Trying to write: " + nBytesRead);
				}
				baos.write(abData, 0, nBytesRead);
				if (TDebug.TraceClip)
				{
					TDebug.out("TSoftClip.open(): Written: " + nBytesRead);
				}
			}
		}
		m_abClip = baos.toByteArray();
		setBufferSize(m_abClip.length);
		// open the line
		m_line.open(getFormat());
		// to trigger the events
		// open();
	}



	public int getFrameLength()
	{
		if (isOpen())
		{
			return getBufferSize() / getFormat().getFrameSize();
		}
		else
		{
			return AudioSystem.NOT_SPECIFIED;
		}
	}



	public long getMicrosecondLength()
	{
		if (isOpen())
		{
			return (long) (getFrameLength() * getFormat().getFrameRate() * 1000000);
		}
		else
		{
			return AudioSystem.NOT_SPECIFIED;
		}
	}



	public void setFramePosition(int nPosition)
	{
		// TOOD:
	}



	public void setMicrosecondPosition(long lPosition)
	{
		// TOOD:
	}



	public int getFramePosition()
	{
		// TOOD:
		return -1;
	}



	public long getMicrosecondPosition()
	{
		// TOOD:
		return -1;
	}



	public void setLoopPoints(int nStart, int nEnd)
	{
		// TOOD:
	}



	public void loop(int nCount)
	{
		if (TDebug.TraceClip)
		{
			TDebug.out("TSoftClip.loop(int): called; count = " + nCount);
		}
		if (false/*isStarted()*/)
		{
			/*
			 *	only allow zero count to stop the looping
			 *	at the end of an iteration.
			 */
			if (nCount == 0)
			{
				if (TDebug.TraceClip)
				{
					TDebug.out("TSoftClip.loop(int): stopping sample");
				}
				// m_esdSample.stop();
			}
		}
		else
		{
			m_nRepeatCount = nCount;
			m_thread = new Thread(this);
			m_thread.start();
		}
		// TOOD:
	}



	public void flush()
	{
		// TOOD:
	}



	public void drain()
	{
		// TOOD:
	}



	public void close()
	{
		// m_esdSample.free();
		// m_esdSample.close();
		// TOOD:
	}




	public void open()
	{
		// TODO:
	}



	public void start()
	{
		if (TDebug.TraceClip)
		{
			TDebug.out("TSoftClip.start(): called");
		}
		/*
		 *	This is a hack. What start() really should do is
		 *	start playing at the position playback was stopped.
		 */
		if (TDebug.TraceClip)
		{
			TDebug.out("TSoftClip.start(): calling 'loop(0)' [hack]");
		}
		loop(0);
	}



	public void stop()
	{
		// TODO:
		// m_esdSample.kill();
	}



	/*
	 *	This method is enforced by DataLine, but doesn't make any
	 *	sense for Clips.
	 */
	public int available()
	{
		return -1;
	}



	public void run()
	{
		while (m_nRepeatCount >= 0)
		{
			m_line.write(m_abClip, 0, m_abClip.length);
			m_nRepeatCount--;
		}
	}

}



/*** TSoftClip.java ***/

