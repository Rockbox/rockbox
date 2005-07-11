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

/**
 * Decodes an MP3 file with ID3v1 tag. The file is compliant to the
 * specifications found at <a href="http://www.id3.org">www.id3.org</a>.
 *
 * @author    Richard Körber &lt;dev@shredzone.de&gt;
 * @version   $Id$
 */
public class TagMp3v1 extends LTRmp3 {
  private String artist  = "";
  private String comment = "";
  private String title   = "";
  private String album   = "";
  private String year    = "";
  private String track   = "";
  private String genre   = "";

  /**
   * Create a new TagMp3v1 object.
   *
   * @param   in        File to be read
   * @throws  FormatDecodeException  Description of the Exception
   */
  public TagMp3v1( RandomAccessFile in )
  throws FormatDecodeException {
    super( in );

    try {
      //--- Decode header ---
      in.seek( in.length() - 128 );     // To the place where the tag lives
      if( !readStringLen( 3 ).equals( "TAG" ) ) {
        throw new FormatDecodeException( "not an id3v1 tag" );
      }

      title   = readStringLen( 30, charsetV1 ).trim();
      artist  = readStringLen( 30, charsetV1 ).trim();
      album   = readStringLen( 30, charsetV1 ).trim();
      year    = readStringLen( 4,  charsetV1 ).trim();
      comment = readStringLen( 28, charsetV1 );

      byte[] sto  = new byte[2];
      in.readFully( sto );
      if( sto[0] == 0x00 ) {
        // ID3v1.1
        track = ( sto[1]>0 ? String.valueOf(sto[1]) : "" );
      } else {
        // ID3v1.0
        comment += new String( sto, charsetV1 );
      }
      comment = comment.trim();

      genre = decodeGenre( in.readUnsignedByte() );
      if( genre==null ) genre="";

    } catch( IOException e ) {
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
   * @return   The type
   */
  public String getType() {
    return "MP3/id3v1";
  }

  /**
   * Get the artist.
   *
   * @return   The artist (never null)
   */
  public String getArtist() {
    return artist;
  }

  /**
   * Get the album.
   *
   * @return   The album (never null)
   */
  public String getAlbum() {
    return album;
  }

  /**
   * Get the title.
   *
   * @return   The title (never null)
   */
  public String getTitle() {
    return title;
  }

  /**
   * Get the genre.
   *
   * @return   The genre (never null)
   */
  public String getGenre() {
    return genre;
  }

  /**
   * Get the year.
   *
   * @return   The year (never null)
   */
  public String getYear() {
    return year;
  }

  /**
   * Get the comment.
   *
   * @return   The comment (never null)
   */
  public String getComment() {
    return comment;
  }

  /**
   * Get the track.
   *
   * @return   The track (never null)
   */
  public String getTrack() {
    return track;
  }

}
