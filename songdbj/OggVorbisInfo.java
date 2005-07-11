/*
 * OggVorbisInfo.
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
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.net.URL;
import java.util.Map;
import java.util.Vector;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.tritonus.share.sampled.file.TAudioFileFormat;

/**
 * This class gives information (audio format and comments) about Ogg Vorbis file or URL.
 */
public class OggVorbisInfo implements TagInfo
{
	protected int serial = 0;
	protected int channels = 0;
	protected int version = 0;
	protected int rate = 0;
	protected int minbitrate = 0;
	protected int maxbitrate = 0;
	protected int averagebitrate = 0;
	protected int nominalbitrate = 0;
	protected long totalms = 0;
	protected String vendor = "";
	protected String location = null;

	protected long size = 0;
	protected int track = -1;
	protected String year = null;
	protected String genre = null;
	protected String title = null;
	protected String artist = null;
	protected String album = null;
	protected Vector comments = new Vector();

  
  /***
   * Constructor.
   */
  public OggVorbisInfo()
  {
  	super();
  }
  
  /**
   * Load and parse Ogg Vorbis info from File.
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
   * Load and parse Ogg Vorbis info from URL.
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
   * Load and parse Ogg Vorbis info from InputStream.
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
   * Load Ogg Vorbis info from file.
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
   * Load Ogg Vorbis info from URL.
   * @param input
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  protected void loadInfo(URL input) throws IOException, UnsupportedAudioFileException
  {
		AudioFileFormat aff = AudioSystem.getAudioFileFormat(input);
		loadInfo(aff);
		loadExtendedInfo(aff);			
  }

  /**
   * Load info from AudioFileFormat.
   * @param aff
   * @throws UnsupportedAudioFileException
   */
  protected void loadInfo(AudioFileFormat aff) throws UnsupportedAudioFileException
  {
		String type = aff.getType().toString();
		if (!type.equalsIgnoreCase("ogg")) throw new UnsupportedAudioFileException("Not Ogg Vorbis audio format");
		if (aff instanceof TAudioFileFormat)
		{
			Map props = ((TAudioFileFormat) aff).properties();
			if (props.containsKey("ogg.channels")) channels = ((Integer)props.get("ogg.channels")).intValue();
			if (props.containsKey("ogg.frequency.hz")) rate = ((Integer)props.get("ogg.frequency.hz")).intValue();
			if (props.containsKey("ogg.bitrate.nominal.bps")) nominalbitrate = ((Integer)props.get("ogg.bitrate.nominal.bps")).intValue();
			averagebitrate = nominalbitrate;
			if (props.containsKey("ogg.bitrate.max.bps")) maxbitrate = ((Integer)props.get("ogg.bitrate.max.bps")).intValue();
			if (props.containsKey("ogg.bitrate.min.bps")) minbitrate = ((Integer)props.get("ogg.bitrate.min.bps")).intValue();
			if (props.containsKey("ogg.version")) version = ((Integer)props.get("ogg.version")).intValue();
			if (props.containsKey("ogg.serial")) serial = ((Integer)props.get("ogg.serial")).intValue();
			if (props.containsKey("ogg.comment.encodedby")) vendor = (String)props.get("ogg.comment.encodedby");			
			
			if (props.containsKey("copyright")) comments.add((String)props.get("copyright"));
			if (props.containsKey("title")) title = (String)props.get("title");
			if (props.containsKey("author")) artist = (String)props.get("author");
			if (props.containsKey("album")) album = (String)props.get("album");
			if (props.containsKey("date")) year = (String)props.get("date");
			if (props.containsKey("comment")) comments.add((String)props.get("comment"));
			if (props.containsKey("duration")) totalms = (long) Math.round((((Long)props.get("duration")).longValue())/1000000);
			if (props.containsKey("ogg.comment.genre")) genre = (String)props.get("ogg.comment.genre");
			if (props.containsKey("ogg.comment.track"))
			{
				try
				{
					track = Integer.parseInt((String)props.get("ogg.comment.track"));
				}
				catch (NumberFormatException e1)
				{
					// Not a number
				}
			}
			if (props.containsKey("ogg.comment.ext.1")) comments.add((String)props.get("ogg.comment.ext.1"));		 
			if (props.containsKey("ogg.comment.ext.2")) comments.add((String)props.get("ogg.comment.ext.2"));
			if (props.containsKey("ogg.comment.ext.3")) comments.add((String)props.get("ogg.comment.ext.3"));			
		} 
  }

  /**
   * Load extended info from AudioFileFormat.
   * @param aff
   * @throws IOException
   * @throws UnsupportedAudioFileException
   */
  protected void loadExtendedInfo(AudioFileFormat aff) throws IOException, UnsupportedAudioFileException
  {
		String type = aff.getType().toString();
		if (!type.equalsIgnoreCase("ogg")) throw new UnsupportedAudioFileException("Not Ogg Vorbis audio format");
		if (aff instanceof TAudioFileFormat)
		{
			Map props = ((TAudioFileFormat) aff).properties();
			// How to load icecast meta data (if any) ??
		} 
  }

  public int getSerial()
  {
	return serial;
  }

  public int getChannels()
  {
	return channels;
  }

  public int getVersion()
  {
	return version;
  }

  public int getMinBitrate()
  {
	return minbitrate;
  }

  public int getMaxBitrate()
  {
	return maxbitrate;
  }

  public int getAverageBitrate()
  {
	return averagebitrate;
  }

  public long getSize()
  {
	return size;
  }

  public String getVendor()
  {
	return vendor;
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
	return totalms;
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
  
  public int getFirstFrameOffset() {
  	return 0;
  }
}