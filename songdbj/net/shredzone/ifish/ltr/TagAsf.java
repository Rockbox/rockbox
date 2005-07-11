/*
 * iFish -- An iRiver iHP jukebox database creation tool
 *
 * Copyright (c) 2004 Richard "Shred" Körber
 *   http://www.shredzone.net/go/ifish
 *
 *-----------------------------------------------------------------------
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is IFISH.
 *
 * The Initial Developer of the Original Code is
 * Richard "Shred" Körber.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK *****
 */
 
package net.shredzone.ifish.ltr;

import java.io.*;
import java.util.List;

import de.jarnbjo.ogg.*;
import de.jarnbjo.vorbis.*;
import entagged.audioformats.AudioFile;
import entagged.audioformats.Tag;
import entagged.audioformats.asf.AsfFileReader;
import entagged.audioformats.exceptions.CannotReadException;

/**
 * Decodes an ASF/WMA stream. It uses parts of the
 * <a href="http://entagged.sf.net/">Entagged</a> software, which is
 * copyrighted by the Entagged Development Team, and published under
 * GPL.
 * <p>
 * <em>NOTE</em> that due to the fact that Entagged is GPL, you <em>MUST</em>
 * remove all entagged sources and this class file if you decide to use
 * the LGPL or MPL part of the iFish licence!
 *
 * @author  Richard Körber &lt;dev@shredzone.de&gt;
 * @version $Id$
 */
public class TagAsf extends LTR {
  private final Tag tag;
  
  /**
   * Create a new TagAsf object.
   *
   * @param   in        File to read
   * @param   file      Reference to the file itself
   * @throws  FormatDecodeException     Couldn't decode this file
   */
  public TagAsf( RandomAccessFile in, File file ) 
  throws FormatDecodeException {
    super( in );
    
    try {
      final AsfFileReader afr = new AsfFileReader();
      final AudioFile af = afr.read( file, in );
      tag = af.getTag();
    }catch( CannotReadException e ) {
      throw new FormatDecodeException( "could not decode file: " + e.toString() );
    }catch( RuntimeException e ) {
      throw new FormatDecodeException( "error decoding file: " + e.toString() );
    }
  }

  /**
   * Get the type of this file. This is usually the compression format
   * itself (e.g. "OGG" or "MP3"). If there are different taggings for
   * this format, the used tag format is appended after a slash (e.g.
   * "MP3/id3v2").
   *
   * @return    The type
   */
  public String getType() {
    return "ASF";
  }
  
  /**
   * Get the artist.
   *
   * @return   The artist (never null)
   */
  public String getArtist() {
    return tag.getFirstArtist().trim();
  }
         
  /**
   * Get the album.
   *
   * @return   The album (never null)
   */
  public String getAlbum() {
    return tag.getFirstAlbum().trim();
  }
         
  /**
   * Get the title.
   *
   * @return   The title (never null)
   */
  public String getTitle() {
    return tag.getFirstTitle().trim();
  }
         
  /**
   * Get the genre.
   *
   * @return   The genre (never null)
   */
  public String getGenre() {
    return tag.getFirstGenre().trim();
  }
         
  /**
   * Get the year.
   *
   * @return   The year (never null)
   */
  public String getYear() {
    return tag.getFirstYear().trim();
  }
         
  /**
   * Get the comment.
   *
   * @return   The comment (never null)
   */
  public String getComment() {
    return tag.getFirstComment().trim();
  }
         
  /**
   * Get the track.
   *
   * @return   The track (never null)
   */
  public String getTrack() {
    return tag.getFirstTrack().trim();
  }
  
}
