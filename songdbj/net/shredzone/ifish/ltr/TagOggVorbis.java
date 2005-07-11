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
import de.jarnbjo.ogg.*;
import de.jarnbjo.vorbis.*;

/**
 * Decodes an Ogg Vorbis stream. It uses the
 * <a href="http://www.j-ogg.de/">J-Ogg</a> library, which is
 * copyrighted by Tor-Einar Jarnbjo. His licence says that may use and
 * modify it as will (even commercial), as long as a reference to his
 * library is stated in the software.
 * <p>
 * <b>NOTE:</b> Due to a bug, there is a patch required in the J-Ogg
 * library. In class de.jarnbjo.vorbis.CommentHeader, private method
 * <tt>addComment()</tt>, add a line
 * <pre>
 *   key = key.toUpperCase();
 * </pre>
 * after the method declaration header.
 *
 * @author  Richard Körber &lt;dev@shredzone.de&gt;
 * @version $Id$
 */
public class TagOggVorbis extends LTR {
  private CommentHeader cmt;
  
  /**
   * Create a new TagOggVorbis object.
   *
   * @param   in        File to read
   * @throws  FormatDecodeException     Couldn't decode this file
   */
  public TagOggVorbis( RandomAccessFile in ) 
  throws FormatDecodeException {
    super( in );
    
    try {
      //--- Check for Ogg Signature ---
      // I expected J-Ogg to do this check, but it has been commented
      // out because very old ogg files do not seem to have this header.
      // We will ignore those files.
      if( !readStringLen(4).equals("OggS") )
        throw new FormatDecodeException( "not an Ogg file" );
      in.seek(0);

      //--- Get the Comment Header ---
      OggFastFileStream fs = new OggFastFileStream(in);
      LogicalOggStream los = (LogicalOggStream) fs.getLogicalStreams().iterator().next();
      if( los.getFormat() != LogicalOggStream.FORMAT_VORBIS )
        throw new FormatDecodeException( "not a plain Ogg Vorbis file" );
      VorbisStream vos = new VorbisStream( los );
      cmt = vos.getCommentHeader();
      
    }catch( OggFormatException e ) {
      throw new FormatDecodeException( "not an Ogg file" );
    }catch( VorbisFormatException e ) {
      throw new FormatDecodeException( "not an Ogg Vorbis file" );
    }catch( IOException e ) {
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
    return "OGG";
  }
  
  /**
   * Get the artist.
   *
   * @return   The artist (never null)
   */
  public String getArtist() {
    String val = cmt.getArtist();
    if( val==null ) val="";
    return val.trim();
  }
         
  /**
   * Get the album.
   *
   * @return   The album (never null)
   */
  public String getAlbum() {
    String val = cmt.getAlbum();
    if( val==null ) val="";
    return val.trim();
  }
         
  /**
   * Get the title.
   *
   * @return   The title (never null)
   */
  public String getTitle() {
    String val = cmt.getTitle();
    if( val==null ) val="";
    return val.trim();
  }
         
  /**
   * Get the genre.
   *
   * @return   The genre (never null)
   */
  public String getGenre() {
    String val = cmt.getGenre();
    if( val==null ) val="";
    return val.trim();
  }
         
  /**
   * Get the year.
   *
   * @return   The year (never null)
   */
  public String getYear() {
    String val = cmt.getDate();
    if( val==null ) val="";
    return val.trim();
  }
         
  /**
   * Get the comment.
   *
   * @return   The comment (never null)
   */
  public String getComment() {
    String val = cmt.getDescription();
    if( val==null ) {
      // *sigh* The Ogg Vorbis documentation does not explicitely
      //   state the comment types. So there are some ogg writers
      //   around that use COMMENT instead of DESCRIPTION. We will
      //   use COMMENT if DESCRIPTION was empty.
      val = cmt.getComment("COMMENT");
    }
    if( val==null )
      val="";
    return val.trim();
  }
         
  /**
   * Get the track.
   *
   * @return   The track (never null)
   */
  public String getTrack() {
    String val = cmt.getTrackNumber();
    if( val==null ) val="";
    return val.trim();
  }
  
}
