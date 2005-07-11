/*
 * IcyListener.
 * 
 * JavaZOOM : mp3spi@javazoom.net
 * 			  http://www.javazoom.net
 * 
 *-----------------------------------------------------------------------
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
 *----------------------------------------------------------------------
 */

package javazoom.spi.mpeg.sampled.file;

import javazoom.spi.mpeg.sampled.file.tag.MP3Tag;
import javazoom.spi.mpeg.sampled.file.tag.TagParseEvent;
import javazoom.spi.mpeg.sampled.file.tag.TagParseListener;

/**
 * This class (singleton) allow to be notified on shoutcast meta data
 * while playing the stream (such as song title).
 */
public class IcyListener implements TagParseListener
{
	private static IcyListener instance = null;
	private MP3Tag lastTag = null;
	private String streamTitle = null;
	private String streamUrl = null;
	
		
	private IcyListener()
	{
		super();
	}

	public static synchronized IcyListener getInstance()
	{
		if (instance == null)
		{
			instance = new IcyListener();
		}
		return instance;		
	}
	
	/* (non-Javadoc)
	 * @see javazoom.spi.mpeg.sampled.file.tag.TagParseListener#tagParsed(javazoom.spi.mpeg.sampled.file.tag.TagParseEvent)
	 */
	public void tagParsed(TagParseEvent tpe)
	{		
		lastTag = tpe.getTag();
		String name = lastTag.getName();
		if ((name != null) && (name.equalsIgnoreCase("streamtitle")))
		{
			streamTitle = (String) lastTag.getValue();
		}
		else if ((name != null) && (name.equalsIgnoreCase("streamurl")))
		{
			streamUrl = (String) lastTag.getValue();
		}	
	}

	/**
	 * @return
	 */
	public MP3Tag getLastTag()
	{
		return lastTag;
	}

	/**
	 * @param tag
	 */
	public void setLastTag(MP3Tag tag)
	{
		lastTag = tag;
	}

	/**
	 * @return
	 */
	public String getStreamTitle()
	{
		return streamTitle;
	}

	/**
	 * @return
	 */
	public String getStreamUrl()
	{
		return streamUrl;
	}

	/**
	 * @param string
	 */
	public void setStreamTitle(String string)
	{
		streamTitle = string;
	}

	/**
	 * @param string
	 */
	public void setStreamUrl(String string)
	{
		streamUrl = string;
	}
	
	/**
	 * Reset all properties.
	 */
	public void reset()
	{
		lastTag = null;
		streamTitle = null;
		streamUrl = null;		
	}

}
