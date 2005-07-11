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
 * The Base Class for the Lightweight Tag Reader. LTR was made to read
 * the basic tag information of MP3 (IDv1, IDv2), Ogg Vorbis and ASF/WMA
 * files. It is lightweight because it is optimized for speed, and is
 * only able to read the tag information, but unable to edit them.
 *
 * @author    Richard Körber &lt;dev@shredzone.de&gt;
 * @version   $Id$
 */
public abstract class LTR {
  protected RandomAccessFile in;        // The file to be checked

  /**
   * Create a new LTR object. It is connected to the file to be decoded,
   * by a RandomAccessFile. The cursor position will be changed during
   * recognizing and decoding.
   *
   * @param   in        RandomAccessFile to be used
   * @throws  FormatDecodeException  Description of the Exception
   */
  public LTR( RandomAccessFile in )
  throws FormatDecodeException {
    this.in = in;
    try {
      in.seek( 0 );                     // To the beginning of file
    } catch( IOException e ) {
      throw new FormatDecodeException( "couldn't seek: " + e.toString() );
    }
  }

  /**
   * Create an LTR object for a file. If the file given, was not
   * recognized or did not contain any tags, null will be returned.
   *
   * @param file             File to open
   * @return                 LTR to this file, or null
   * @exception IOException  Description of the Exception
   */
  public static LTR create( File file ) {
    RandomAccessFile in  = null;
    LTR result           = null;
    
    try {
      in = new RandomAccessFile( file, "r" );
  
      try {
        result = new TagOggVorbis( in );
        return result;
      } catch( FormatDecodeException e ) {}
  
      try {
        result = new TagMp3v2( in );
        return result;
      } catch( FormatDecodeException e ) {}
  
      try {
        result = new TagMp3v200( in );
        return result;
      } catch( FormatDecodeException e ) {}
  
      try {
        result = new TagAsf( in, file );
        return result;
      }catch( FormatDecodeException e ) {}

      try {
        // Always check ID3v1 *after* ID3v2, because a lot of ID3v2
        // files also contain ID3v1 tags with limited content.
        result = new TagMp3v1( in );
        return result;
      } catch( FormatDecodeException e ) {}
    }catch(IOException e) {
    	return null;
    } finally {
    	try {
      	if( in!=null ) in.close();
      } catch(IOException e) {
      	System.out.println("Failed to close file.");
    	}
    }

    return null;
  }

  /**
   * Get the type of this file. This is usually the compression format
   * itself (e.g. "OGG" or "MP3"). If there are different taggings for
   * this format, the used tag format is appended after a slash (e.g.
   * "MP3/id3v2").
   *
   * @return   The type
   */
  public abstract String getType();

  /**
   * Get the artist.
   *
   * @return   The artist (never null)
   */
  public abstract String getArtist();

  /**
   * Get the album.
   *
   * @return   The album (never null)
   */
  public abstract String getAlbum();

  /**
   * Get the title.
   *
   * @return   The title (never null)
   */
  public abstract String getTitle();

  /**
   * Get the genre.
   *
   * @return   The genre (never null)
   */
  public abstract String getGenre();

  /**
   * Get the year.
   *
   * @return   The year (never null)
   */
  public abstract String getYear();

  /**
   * Get the comment.
   *
   * @return   The comment (never null)
   */
  public abstract String getComment();

  /**
   * Get the track.
   *
   * @return   The track (never null)
   */
  public abstract String getTrack();

  /**
   * Read a String of a certain length from the file. It will always use
   * "ISO-8859-1" encoding!
   *
   * @param   length        Maximum number of bytes to read
   * @return    String that was read
   * @throws  IOException   If EOF was already reached
   */
  protected String readStringLen( int length )
  throws IOException {
    return readStringLen( length, "ISO-8859-1" );
  }

  /**
   * Read a String of a certain length from the file. The length will
   * not be exceeded. No null termination is required. Anyhow an
   * IOException will be thrown if the EOF was reached before invocation.
   *
   * @param   length        Maximum number of bytes to read
   * @param   charset       Charset to be used
   * @return    String that was read
   * @throws  IOException   If EOF was already reached
   */
  protected String readStringLen( int length, String charset )
  throws IOException {
    byte[] buf      = new byte[length];
    int readlength  = in.read( buf );
    if( readlength < 0 ) {
      throw new IOException( "Unexpected EOF" );
    }
    return new String( buf, 0, readlength, charset );
  }
  
  /**
   * Return a string representation of the LTR content.
   * 
   * @return  String representation
   */
  public String toString() {
    StringBuffer buff = new StringBuffer();
    buff.append( getType() );
    buff.append( "[ART='" );
    buff.append( getArtist() );
    buff.append( "' ALB='" );
    buff.append( getAlbum() );
    buff.append( "' TIT='" );
    buff.append( getTitle() );
    buff.append( "' TRK='" );
    buff.append( getTrack() );
    buff.append( "' GEN='" );
    buff.append( getGenre() );
    buff.append( "' YR='" );
    buff.append( getYear() );
    buff.append( "' CMT='" );
    buff.append( getComment() );
    buff.append( "']" );
    return buff.toString();
  }

}
