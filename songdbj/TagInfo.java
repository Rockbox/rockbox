/*
 * TagInfo.
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
import java.util.Vector;

import javax.sound.sampled.UnsupportedAudioFileException;

/**
 * This interface define needed features for song information.
 * Adapted from Scott Pennell interface.
 */
public interface TagInfo
{
  
  public void load(InputStream input) throws IOException, UnsupportedAudioFileException;
  
  public void load(URL input) throws IOException, UnsupportedAudioFileException;
  
  public void load(File input) throws IOException, UnsupportedAudioFileException;
  
  /**
   * Get Sampling Rate
   * @return
   */
  public int getSamplingRate();

  /**
   * Get Nominal Bitrate
   * @return bitrate in bps
   */
  public int getBitRate();

  /**
   * Get channels.
   * @return channels
   */
  public int getChannels();
  
  /**
   * Get play time in seconds.
   * @return
   */
  public long getPlayTime();

  /**
   * Get the title of the song.
   * @return the title of the song
   */
  public String getTitle();

  /**
   * Get the artist that performed the song
   * @return the artist that performed the song
   */
  public String getArtist();

  /**
   * Get the name of the album upon which the song resides
   * @return the album name
   */
  public String getAlbum();

  /**
   * Get the track number of this track on the album
   * @return the track number
   */
  public int getTrack();

  /**
   * Get the genre string of the music
   * @return the genre string
   */
  public String getGenre();

  /**
   * Get the year the track was released
   * @return the year the track was released
   */
  public String getYear();

  /**
   * Get any comments provided about the song
   * @return the comments
   */
  public Vector getComment();
  
  public int getFirstFrameOffset();
}