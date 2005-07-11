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
 * Decodes an MP3 file with ID3v2 tag. The file is compliant to the
 * specifications found at <a href="http://www.id3.org">www.id3.org</a>,
 * V2.4.0 and V2.3.0. ID3 V2.2.0 is handled in a separate tag handler.
 * Anyhow it has certain limitations regarding tag frames which could
 * not be used or entirely used to iFish, e.g. multiple genres.
 *
 * @author    Richard Körber &lt;dev@shredzone.de&gt;
 * @version   $Id$
 */
public class TagMp3v2 extends LTRmp3 {
  private int     globalSize;
  private Pattern patGenre;
  private boolean v230 = false;     // V2.3.0 detected

  private String  artist  = "";
  private String  comment = "";
  private String  title   = "";
  private String  album   = "";
  private String  year    = "";
  private String  track   = "";
  private String  genre   = "";

  /**
   * Create a new TagMp3v2 instance.
   *
   * @param   in      File to be read
   * @throws  FormatDecodeException     Couldn't decode this file
   */
  public TagMp3v2( RandomAccessFile in )
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

      if( version==0xFF || revision==0xFF ) {
        throw new FormatDecodeException( "not an id3v2 tag" );
      }

      if( version<=0x02 || version>0x04 ) {
        throw new FormatDecodeException( "unable to decode ID3v2."+version+"."+revision );
      }

      byte flags  = in.readByte();
      v230 = (version==0x03);
      globalSize = readSyncsafeInt() + 10;

      //--- Skip extended header ---
      if( ( flags & 0x40 ) != 0 ) {
        int ehsize  = readAutoInt();
        if( ehsize < 6 ) {
          throw new FormatDecodeException( "extended header too small" );
        }
        in.skipBytes( ehsize - 4 );
      }

      //--- Read all frames ---
      Frame frm;
      while( ( frm = readFrame() ) != null ) {
        String type  = frm.getType();
        String[] answer;

        if( type.equals( "TOPE" ) || type.equals( "TPE1" ) ) {

          answer = frm.getAsStringEnc();
          artist = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "COMM" ) ) {

          answer = frm.getAsStringEnc();
          comment = (answer.length>1 ? answer[1].trim() : "");

        } else if( type.equals( "TIT2" ) ) {

          answer = frm.getAsStringEnc();
          title =(answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TALB" ) ) {

          answer = frm.getAsStringEnc();
          album = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TYER" ) ) {

          answer = frm.getAsStringEnc();
          year = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TRCK" ) ) {

          answer = frm.getAsStringEnc();
          track = (answer.length>0 ? answer[0].trim() : "");

        } else if( type.equals( "TCON" ) ) {

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

      //--- Footer frame? ---
      if( ( flags & 0x10 ) != 0 ) {
        in.skipBytes( 10 );             // Then skip it
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
    return ( v230 ? "MP3/id3v2.3.0" : "MP3/id3v2.4.0" );
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
   * Read an ID3v2 integer. This is a 4 byte big endian value, which is
   * only syncsafe for ID3v2.4 or higher, but not for ID3v2.3.
   *
   * @return    The integer read.
   * @throws    IOException  If there were not enough bytes in the file.
   */
  protected int readAutoInt()
  throws IOException {
    return( v230 ? readInt() : readSyncsafeInt() );
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
    String type  = readStringLen( 4 );
    if( type.charAt(0)==0 ) {   // Optional padding frame
      in.skipBytes( (int) ( globalSize - in.getFilePointer() ) );// Skip it...
      return null;                      // Return null
    }

    //--- Read the frame ---
    int size     = readAutoInt();
    byte flag1   = in.readByte();
    byte flag2   = in.readByte();
    
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
    return new Frame( type, size, flag1, flag2, data );
  }

/*--------------------------------------------------------------------*/
  
  /**
   * This class contains a ID3v2 frame.
   */
  private static class Frame {
    private final String  charset;
    private final String  type;
//    private final int     size;
//    private final byte    flag1;
    private final byte    flag2;
    private       byte[]  data;
    private       boolean decoded = false;

    /**
     * Constructor for the Frame object
     *
     * @param type   Frame type
     * @param size   Frame size
     * @param flag1  Flag 1
     * @param flag2  Flag 2
     * @param data   Frame content, may be null
     */
    public Frame( String type, int size, byte flag1, byte flag2, byte[] data ) {
      charset = "ISO-8859-1";
      this.type  = type;
// Currently unused...
//      this.size  = size;
//      this.flag1 = flag1;
      this.flag2 = flag2;
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
      decode();
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
      if( data==null ) return new String[0];
      decode();
      if( data.length==0 ) return new String[0];
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
          case 0x02:
            result = new String( data, 1, len, "UTF-16BE" );
            break;
          case 0x03:
            result = new String( data, 1, len, "UTF-8" );
            break;
          default:
            throw new FormatDecodeException( "unknown encoding of frame " + type );
        }
      }catch( UnsupportedEncodingException e ) {
        throw new FormatDecodeException( "Java misses a basic encoding!?" );
      }
      return result.split( "\0" );
    }

    /**
     * Decode a frame, unless already decoded. If the frame was
     * unsynchronized, it will be synchronized here. Compression and
     * encryption is not supported yet. You can invoke this method
     * several times without any effect.
     *
     * @throws  FormatDecodeException  Description of the Exception
     */
    private void decode()
    throws FormatDecodeException {
      if( decoded ) return;
      decoded = true;

      if( ( flag2 & 0x02 ) != 0 ) {     // Unsynchronize
        byte decoded[]  = new byte[data.length];
        int pos       = 0;
        for( int ix = 0; ix < data.length - 1; ix++ ) {
          decoded[pos++] = data[ix];
          if( data[ix] == 0xFF && data[ix + 1] == 0x00 ) {
            ix++;
          }
        }
        data = new byte[pos];
        System.arraycopy( decoded, 0, data, 0, pos );
      }

      if( ( flag2 & 0x08 ) != 0 ) {     // Compression
        throw new FormatDecodeException( "sorry, compression is not yet supported" );
      }

      if( ( flag2 & 0x04 ) != 0 ) {     // Encryption
        throw new FormatDecodeException( "sorry, encryption is not yet supported" );
      }
    }

  }

}
