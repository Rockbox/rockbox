/*
 *   VorbisEncoding.
 * 
 *   JavaZOOM : vorbisspi@javazoom.net
 *              http://www.javazoom.net
 * 
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
 *
 */

package javazoom.spi.vorbis.sampled.file;

import javax.sound.sampled.AudioFormat;

/**
 * Encodings used by the VORBIS audio decoder.
 */
public class VorbisEncoding extends AudioFormat.Encoding
{
  public static final AudioFormat.Encoding VORBISENC = new VorbisEncoding("VORBISENC");

  /**
   * Constructors.
   */
  public VorbisEncoding(String name)
  {
    super(name);
  }
}
