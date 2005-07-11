/*
 * MpegInfo.
 * 
 * JavaZOOM : jlgui@javazoom.net
 *            http://www.javazoom.net
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

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.tritonus.share.sampled.file.TAudioFileFormat;

/**
 * This class gives information (audio format and comments) about MPEG file or URL.
 */
public class MpegInfo implements TagInfo
{
  protected int channels = -1;
  protected String channelsMode = null;
  protected String version = null;
  protected int rate = 0;
  protected String layer = null;
  protected String emphasis = null;
  protected int nominalbitrate = 0;
  protected long total = 0;
  protected String vendor = null;
  protected String location = null;
  protected long size = 0;
  protected boolean copyright = false;
  protected boolean crc = false;
  protected boolean original = false;
  protected boolean priv = false;
  protected boolean vbr = false;
  protected int track = -1;
  protected int offset = 0;
  protected String year = null;
  protected String genre = null;
  protected String title = null;
  protected String artist = null;
  protected String album = null;
  protected Vector comments = null;
  
  /**
   * Constructor.
   */
  public MpegInfo()
  {
  	super();
  }
  
  /**
   * Load and parse MPEG info from File.
   * @param input
   * @throws IOException
   */
  public void load(File input) throws IOException, UnsupportedAudioFileException
  {
    size = input.length();
    location = input.getPath();
    loadInfo(input);
  }

  /**
   * Load and parse MPEG info from URL.
   * @param input
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  public void load(URL input) throws IOException, UnsupportedAudioFileException
  {
	location = input.toString();
	loadInfo(input);
  }

  /**
   * Load and parse MPEG info from InputStream.
   * @param input
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  public void load(InputStream input) throws IOException, UnsupportedAudioFileException
  {
	loadInfo(input);
  }

  /**
   * Load info from input stream.
   * @param input
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  protected void loadInfo(InputStream input) throws IOException, UnsupportedAudioFileException
  {
	AudioFileFormat aff = AudioSystem.getAudioFileFormat(input);
	loadInfo(aff);
  }

  /**
   * Load MP3 info from file.
   * @param file
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  protected void loadInfo(File file) throws IOException, UnsupportedAudioFileException
  {
 		InputStream in = new BufferedInputStream(new FileInputStream(file));
		loadInfo(in);
		in.close();
  }
  
  /**
   * Load info from AudioFileFormat.
   * @param aff
   */
  protected void loadInfo(AudioFileFormat aff) throws UnsupportedAudioFileException
  {
		String type = aff.getType().toString();
		if (!type.equalsIgnoreCase("mp3")) throw new UnsupportedAudioFileException("Not MP3 audio format");
		if (aff instanceof TAudioFileFormat)
		{
			Map props = ((TAudioFileFormat) aff).properties();
			if (props.containsKey("mp3.channels")) channels = ((Integer)props.get("mp3.channels")).intValue();
			if (props.containsKey("mp3.frequency.hz")) rate = ((Integer)props.get("mp3.frequency.hz")).intValue();
			if (props.containsKey("mp3.bitrate.nominal.bps")) nominalbitrate = ((Integer)props.get("mp3.bitrate.nominal.bps")).intValue();
			if (props.containsKey("mp3.version.layer")) layer = "Layer "+(String)props.get("mp3.version.layer");
			if (props.containsKey("mp3.version.mpeg"))
			{
				version = (String)props.get("mp3.version.mpeg");
				if (version.equals("1")) version = "MPEG1"; 
				else if (version.equals("2")) version = "MPEG2-LSF";
				else if (version.equals("2.5")) version = "MPEG2.5-LSF";
			}
			if (props.containsKey("mp3.mode"))
			{
				int mode = ((Integer)props.get("mp3.mode")).intValue();
				if (mode==0) channelsMode = "Stereo";
				else if (mode==1) channelsMode = "Joint Stereo"; 
				else if (mode==2) channelsMode = "Dual Channel";
				else if (mode==3) channelsMode = "Single Channel";
			}
			if (props.containsKey("mp3.crc")) crc = ((Boolean)props.get("mp3.crc")).booleanValue();
			if (props.containsKey("mp3.vbr")) vbr = ((Boolean)props.get("mp3.vbr")).booleanValue();
			if (props.containsKey("mp3.copyright")) copyright = ((Boolean)props.get("mp3.copyright")).booleanValue();
			if (props.containsKey("mp3.original")) original = ((Boolean)props.get("mp3.original")).booleanValue();
			emphasis="none";

			if (props.containsKey("title")) title = (String)props.get("title");
			if (props.containsKey("author")) artist = (String)props.get("author");
			if (props.containsKey("album")) album = (String)props.get("album");
			if (props.containsKey("date")) year = (String)props.get("date");
			if (props.containsKey("duration")) total = (long) Math.round((((Long)props.get("duration")).longValue())/1000000);
			if (props.containsKey("mp3.id3tag.genre")) genre = (String)props.get("mp3.id3tag.genre");
			
			if (props.containsKey("mp3.header.pos")) {
			  offset = ((Integer)props.get("mp3.header.pos")).intValue();
			}
			else
				offset = 0;
			if (props.containsKey("mp3.id3tag.track"))
			{
				try
				{
					track = Integer.parseInt((String)props.get("mp3.id3tag.track"));
				}
				catch (NumberFormatException e1)
				{
					// Not a number
				}
			}		 
		} 
  }

  /**
   * Load MP3 info from URL.
   * @param input
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  protected void loadInfo(URL input) throws IOException, UnsupportedAudioFileException
  {
		AudioFileFormat aff = AudioSystem.getAudioFileFormat(input);
		loadInfo(aff);
		loadShoutastInfo(aff);
  }
  
  /**
   * Load Shoutcast info from AudioFileFormat.
   * @param aff
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  protected void loadShoutastInfo(AudioFileFormat aff) throws IOException, UnsupportedAudioFileException
  {
		String type = aff.getType().toString();
		if (!type.equalsIgnoreCase("mp3")) throw new UnsupportedAudioFileException("Not MP3 audio format");
		if (aff instanceof TAudioFileFormat)
		{
			Map props = ((TAudioFileFormat) aff).properties();
			// Try shoutcast meta data (if any).
			Iterator it = props.keySet().iterator();
			comments = new Vector();
			while (it.hasNext())
			{
				String key = (String) it.next();
				if (key.startsWith("mp3.shoutcast.metadata."))
				{
					String value = (String) props.get(key);
					key = key.substring(23,key.length());
					if (key.equalsIgnoreCase("icy-name"))
					{
						title = value;
					}
					else if (key.equalsIgnoreCase("icy-genre"))
					{
						genre = value;
					}
					else
					{
						comments.add(key+"="+value);
					}
				}
			}
		} 
  }

  public boolean getVBR()
  {
    return vbr;
  }

  public int getChannels()
  {
    return channels;
  }

  public String getVersion()
  {
    return version;
  }

  public String getEmphasis()
  {
    return emphasis;
  }

  public boolean getCopyright()
  {
    return copyright;
  }

  public boolean getCRC()
  {
    return crc;
  }

  public boolean getOriginal()
  {
    return original;
  }

  public String getLayer()
  {
    return layer;
  }

  public long getSize()
  {
    return size;
  }

  public String getLocation()
  {
    return location;
  }

  /*-- TagInfo Implementation --*/

  public int getSamplingRate()
  {
    return rate;
  }

  public int getBitRate()
  {
    return nominalbitrate;
  }

  public long getPlayTime()
  {
    return total;
  }

  public String getTitle()
  {
    return title;
  }

  public String getArtist()
  {
    return artist;
  }

  public String getAlbum()
  {
    return album;
  }

  public int getTrack()
  {
    return track;
  }

  public String getGenre()
  {
    return genre;
  }

  public Vector getComment()
  {
    return comments;
  }

  public String getYear()
  {
    return year;
  }

  /**
   * Get channels mode.
   * @return
   */
  public String getChannelsMode()
  {
	return channelsMode;
  }
  
  public int getFirstFrameOffset() {
  	return offset;
  }

}