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
 * The Base Class for mp3 decoding of the Lightweight Tag Reader.
 *
 * @author    Richard Körber &lt;dev@shredzone.de&gt;
 * @version   $Id$
 */
public abstract class LTRmp3 extends LTR {
  
  private final static String[] genres  = {
    //--- Genres as specified in ID3v1 ---
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other",
    "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial",
    "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
    "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
    "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream",
    "Southern Rock", "Comedy", "Cult", "Gangsta", "Top 40",
    "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock",

    //--- This are WinAmp extensions ---
    "Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebop",
    "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
    "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass",
    "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
    "Duet", "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
  };
  
  protected final String charsetV1 = "ISO-8859-1";
  protected final String charsetV2 = "ISO-8859-1";
  
  /**
   * Constructor for the LTRmp3 object
   *
   * @param   in        File to be used
   * @throws  FormatDecodeException  Could not decode file
   */
  public LTRmp3( RandomAccessFile in )
  throws FormatDecodeException {
    super( in );
  }

  /**
   * Decode the mp3 numerical Genre code and convert it to a human
   * readable string. The genre is decoded according to the
   * specifications found at <a href="http://www.id3.org">www.id3.org</a>,
   * as well as the WinAmp extensions.
   *
   * @param   id        Genre ID
   * @return  ID String, null if the genre ID was unknown
   */
  protected String decodeGenre( int id ) {
    if( id>=genres.length ) return null;
    return genres[id];
  }
  
  /**
   * Read an ID3v2 integer. This is a 4 byte big endian value, which is
   * always not syncsafe.
   *
   * @return    The integer read.
   * @throws    IOException  If there were not enough bytes in the file.
   */
  protected int readInt()
  throws IOException {
    int val  = 0;
    for( int cnt = 4; cnt > 0; cnt-- ) {
      val <<= 8;
      val |= ( in.readByte() & 0xFF );
    }
    return val;
  }
  
  /**
   * Read an ID3v2 syncsafe integer. This is a 4 byte big endian value
   * with the bit 7 of each byte always being 0.
   *
   * @return    The syncsafe integer read.
   * @throws    IOException  If there were not enough bytes in the file.
   */
  protected int readSyncsafeInt()
  throws IOException {
    int val  = 0;
    for( int cnt = 4; cnt > 0; cnt-- ) {
      val <<= 7;
      val |= ( readSyncsafe() & 0x7F );
    }
    return val;
  }

  /**
   * Read a syncsafe byte. It is made sure that a byte is available in
   * the file, and that bit 7 is 0. An IOException is thrown otherwise.
   *
   * @return    The byte read.
   * @throws    IOException  If premature EOF was reached or byte was not
   *      syncsafe.
   */
  protected byte readSyncsafe()
  throws IOException {
    byte read  = in.readByte();
    if(( read & 0x80 ) != 0 ) {
      throw new IOException( "not syncsafe" );
    }
    return read;
  }

}
