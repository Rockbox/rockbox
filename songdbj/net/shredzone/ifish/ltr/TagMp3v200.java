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
import java.util.regex.*;

/**
 * Decodes an MP3 file with old ID3v2.00 tag. The file is compliant to the
 * specifications found at <a href="http://www.id3.org/id3v2-00.txt">www.id3.org</a>.
 * Only ID3 V2.00 up to v2.2.0 is handled here. Newer versions are separately
 * handled in TagMp3v2.
 *
 * @author    Richard Körber &lt;dev@shredzone.de&gt;
 * @version   $Id$
 */
public class TagMp3v200 extends LTRmp3 {
  private int     globalSize;
  private Pattern patGenre;

  private String  artist  = "";
  private String  comment = "";
  private String  title   = "";
  private String  album   = "";
  private String  year    = "";
  private String  track   = "";
  private String  genre   = "";

  /**
   * Create a new TagMp3v200 instance.
   *
   * @param   in      File to be read
   * @throws  FormatDecodeException     Couldn't decode this file
   */
  public TagMp3v200( RandomAccessFile in )
  throws FormatDecodeException {
    super( in );

    patGenre = Pattern.compile( "\\((\\d{1,3})\\).*" );

    try {
      //--- Decode header ---
      if( !readStringLen( 3 ).equals( "ID3" ) ) {
        throw new FormatDecodeException( "not an id3v2 tag" );
      }

      byte version  = in.readByte();
      byte revision = in.readByte();

      if( version!=0x02 || revision==0xFF ) {
        throw new FormatDecodeException( "not an id3v2.2.0 tag" );
      }

      byte flags  = in.readByte();
      globalSize = readSyncsafeInt() + 10;

      //--- Read all frames ---
      Frame frm;
      while( ( frm = readFrame() ) != null ) {
        String type  = frm.getType();
        String[] answer;

        if( type.equals( "TOA" ) || type.equals( "TP1" ) ) {

          answer = frm.getAsStringEnc();
          artist = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "COM" ) ) {

          answer = frm.getAsStringEnc();
          comment = (answer.length>1 ? answer[1].trim() : "");

        } else if( type.equals( "TT2" ) ) {

          answer = frm.getAsStringEnc();
          title =(answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TAL" ) ) {

          answer = frm.getAsStringEnc();
          album = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TYE" ) ) {

          answer = frm.getAsStringEnc();
          year = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TRK" ) ) {

          answer = frm.getAsStringEnc();
          track = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TCO" ) ) {

          answer = frm.getAsStringEnc();
          if( answer.length>0 ) {
            genre = answer[0].trim();
            Matcher mat  = patGenre.matcher( genre );
            if( mat.matches() ) {
              genre = decodeGenre( Integer.parseInt( mat.group( 1 ) ) );
              if( genre==null ) genre="";
            }
          }
        }
      }

      // Position is now the start of the MP3 data

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
    return "MP3/id3v2.2.0";
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
   * @return    The track (never null)
   */
  public String getTrack() {
    return track;
  }

  /**
   * Read a tag frame. A Frame object will be returned, or null if no
   * more frames were available.
   *
   * @return    The next frame, or null
   * @throws    IOException  If premature EOF was reached.
   */
  protected Frame readFrame()
  throws IOException {
    if( in.getFilePointer() >= globalSize ) {
      return null;
    }

    //--- Get the type ---
    String type  = readStringLen( 3 );
    if( type.charAt(0)==0 ) {   // Optional padding frame
      in.skipBytes( (int) ( globalSize - in.getFilePointer() ) );// Skip it...
      return null;                      // Return null
    }

    //--- Read the frame ---
    int size     = read3Int();
    
    //--- Read the content ---
    // Stay within reasonable boundaries. If the data part is bigger than
    // 16K, it's not really useful for us, so we will keep the frame empty.
    byte[] data = null;
    if( size<=16384 ) {
      data  = new byte[size];
      int rlen     = in.read( data );
      if( rlen != size ) {
        throw new IOException( "unexpected EOF" );
      }
    }else {
      in.skipBytes( size );
    }

    //--- Return the frame ---
    return new Frame( type, size, data );
  }

  /**
   * Read an ID3v2 3 byte integer. This is a 3 byte big endian value, which is
   * always not syncsafe.
   *
   * @return    The integer read.
   * @throws    IOException  If there were not enough bytes in the file.
   */
  protected int read3Int()
  throws IOException {
    int val  = 0;
    for( int cnt = 3; cnt > 0; cnt-- ) {
      val <<= 8;
      val |= ( in.readByte() & 0xFF );
    }
    return val;
  }

/*--------------------------------------------------------------------*/
  
  /**
   * This class contains a ID3v2.2.0 frame.
   */
  private static class Frame {
    private final String  charset;
    private final String  type;
//    private final int     size;
    private       byte[]  data;

    /**
     * Constructor for the Frame object
     *
     * @param type   Frame type
     * @param size   Frame size
     * @param data   Frame content, may be null
     */
    public Frame( String type, int size, byte[] data ) {
      this.charset = "ISO-8859-1";
      this.type  = type;
// Currently unused...
//      this.size  = size;
      this.data  = data;
    }

    /**
     * Get the type.
     *
     * @return   The type of this Frame
     */
    public String getType() {
      return type;
    }

    /**
     * Return the Frame content as String. This method is to be used for
     * strings without a leading encoding byte. This machine's default
     * encoding will be used instead.
     *
     * @return  Encoded string
     * @throws  FormatDecodeException  Could not read or decode frame
     */
    public String getAsString()
    throws FormatDecodeException {
      if(data==null) return null;
      return new String( data );
    }

    /**
     * Return the Frame content as encoded String. The first byte will
     * contain the encoding type, following the string itself. Multiple
     * strings are null-terminated. An array of all strings found, will
     * be returned.
     *
     * @return  Array of all strings. Might be an empty array!
     * @throws  FormatDecodeException  Could not read or decode frame
     */
    public String[] getAsStringEnc()
    throws FormatDecodeException {
      if(data==null) return new String[0];
      int len        = data.length - 1;
      String result  = "";
      try {
        switch ( data[0] ) {
          case 0x00:
            result = new String( data, 1, len, charset );
            break;
          case 0x01:
            result = new String( data, 1, len, "UTF-16" );
            break;
          default:
            throw new FormatDecodeException( "unknown encoding of frame " + type );
        }
      }catch( UnsupportedEncodingException e ) {
        throw new FormatDecodeException( "Java misses a basic encoding!?" );
      }
      return result.split( "\0" );
    }

  }

}
