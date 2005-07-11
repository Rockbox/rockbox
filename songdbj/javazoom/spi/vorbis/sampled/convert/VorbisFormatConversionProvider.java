/*
 *   VorbisFormatConversionProvider.
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
 
package javazoom.spi.vorbis.sampled.convert;

import java.util.Arrays;
import javazoom.spi.vorbis.sampled.file.VorbisEncoding;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;

import org.tritonus.share.sampled.convert.TMatrixFormatConversionProvider;

/**
 * ConversionProvider for VORBIS files.
 */
public class VorbisFormatConversionProvider extends TMatrixFormatConversionProvider
{
  private static final AudioFormat[] INPUT_FORMATS =
  {
      new AudioFormat(VorbisEncoding.VORBISENC, 32000.0F, -1, 1, -1, -1, false), // 0
      new AudioFormat(VorbisEncoding.VORBISENC, 32000.0F, -1, 2, -1, -1, false), // 1
      new AudioFormat(VorbisEncoding.VORBISENC, 44100.0F, -1, 1, -1, -1, false), // 2
      new AudioFormat(VorbisEncoding.VORBISENC, 44100.0F, -1, 2, -1, -1, false), // 3
      new AudioFormat(VorbisEncoding.VORBISENC, 48000.0F, -1, 1, -1, -1, false), // 4
      new AudioFormat(VorbisEncoding.VORBISENC, 48000.0F, -1, 2, -1, -1, false), // 5

      new AudioFormat(VorbisEncoding.VORBISENC, 16000.0F, -1, 1, -1, -1, false), // 18
      new AudioFormat(VorbisEncoding.VORBISENC, 16000.0F, -1, 2, -1, -1, false), // 19
      new AudioFormat(VorbisEncoding.VORBISENC, 22050.0F, -1, 1, -1, -1, false), // 20
      new AudioFormat(VorbisEncoding.VORBISENC, 22050.0F, -1, 2, -1, -1, false), // 21
      new AudioFormat(VorbisEncoding.VORBISENC, 24000.0F, -1, 1, -1, -1, false), // 22
      new AudioFormat(VorbisEncoding.VORBISENC, 24000.0F, -1, 2, -1, -1, false), // 23

      new AudioFormat(VorbisEncoding.VORBISENC, 8000.0F, -1, 1, -1, -1, false), // 36
      new AudioFormat(VorbisEncoding.VORBISENC, 8000.0F, -1, 2, -1, -1, false), // 37
      new AudioFormat(VorbisEncoding.VORBISENC, 11025.0F, -1, 1, -1, -1, false), // 38
      new AudioFormat(VorbisEncoding.VORBISENC, 11025.0F, -1, 2, -1, -1, false), // 39
      new AudioFormat(VorbisEncoding.VORBISENC, 12000.0F, -1, 1, -1, -1, false), // 40
      new AudioFormat(VorbisEncoding.VORBISENC, 12000.0F, -1, 2, -1, -1, false), // 41
  };

  private static final AudioFormat[] OUTPUT_FORMATS =
  {
      new AudioFormat(8000.0F, 16, 1, true, false), // 0
      new AudioFormat(8000.0F, 16, 1, true, true), // 1
      new AudioFormat(8000.0F, 16, 2, true, false), // 2
      new AudioFormat(8000.0F, 16, 2, true, true), // 3
      /*	24 and 32 bit not yet possible
                      new AudioFormat(8000.0F, 24, 1, true, false),
                      new AudioFormat(8000.0F, 24, 1, true, true),
                      new AudioFormat(8000.0F, 24, 2, true, false),
                      new AudioFormat(8000.0F, 24, 2, true, true),
                      new AudioFormat(8000.0F, 32, 1, true, false),
                      new AudioFormat(8000.0F, 32, 1, true, true),
                      new AudioFormat(8000.0F, 32, 2, true, false),
                      new AudioFormat(8000.0F, 32, 2, true, true),
       */
      new AudioFormat(11025.0F, 16, 1, true, false), // 4
      new AudioFormat(11025.0F, 16, 1, true, true), // 5
      new AudioFormat(11025.0F, 16, 2, true, false), // 6
      new AudioFormat(11025.0F, 16, 2, true, true), // 7
      /*	24 and 32 bit not yet possible
                      new AudioFormat(11025.0F, 24, 1, true, false),
                      new AudioFormat(11025.0F, 24, 1, true, true),
                      new AudioFormat(11025.0F, 24, 2, true, false),
                      new AudioFormat(11025.0F, 24, 2, true, true),
                      new AudioFormat(11025.0F, 32, 1, true, false),
                      new AudioFormat(11025.0F, 32, 1, true, true),
                      new AudioFormat(11025.0F, 32, 2, true, false),
                      new AudioFormat(11025.0F, 32, 2, true, true),
       */
      new AudioFormat(12000.0F, 16, 1, true, false), // 8
      new AudioFormat(12000.0F, 16, 1, true, true), // 9
      new AudioFormat(12000.0F, 16, 2, true, false), // 10
      new AudioFormat(12000.0F, 16, 2, true, true), // 11
      /*	24 and 32 bit not yet possible
                      new AudioFormat(12000.0F, 24, 1, true, false),
                      new AudioFormat(12000.0F, 24, 1, true, true),
                      new AudioFormat(12000.0F, 24, 2, true, false),
                      new AudioFormat(12000.0F, 24, 2, true, true),
                      new AudioFormat(12000.0F, 32, 1, true, false),
                      new AudioFormat(12000.0F, 32, 1, true, true),
                      new AudioFormat(12000.0F, 32, 2, true, false),
                      new AudioFormat(12000.0F, 32, 2, true, true),
       */
      new AudioFormat(16000.0F, 16, 1, true, false), // 12
      new AudioFormat(16000.0F, 16, 1, true, true), // 13
      new AudioFormat(16000.0F, 16, 2, true, false), // 14
      new AudioFormat(16000.0F, 16, 2, true, true), // 15
      /*	24 and 32 bit not yet possible
                      new AudioFormat(16000.0F, 24, 1, true, false),
                      new AudioFormat(16000.0F, 24, 1, true, true),
                      new AudioFormat(16000.0F, 24, 2, true, false),
                      new AudioFormat(16000.0F, 24, 2, true, true),
                      new AudioFormat(16000.0F, 32, 1, true, false),
                      new AudioFormat(16000.0F, 32, 1, true, true),
                      new AudioFormat(16000.0F, 32, 2, true, false),
                      new AudioFormat(16000.0F, 32, 2, true, true),
       */
      new AudioFormat(22050.0F, 16, 1, true, false), // 16
      new AudioFormat(22050.0F, 16, 1, true, true), // 17
      new AudioFormat(22050.0F, 16, 2, true, false), // 18
      new AudioFormat(22050.0F, 16, 2, true, true), // 19
      /*	24 and 32 bit not yet possible
                      new AudioFormat(22050.0F, 24, 1, true, false),
                      new AudioFormat(22050.0F, 24, 1, true, true),
                      new AudioFormat(22050.0F, 24, 2, true, false),
                      new AudioFormat(22050.0F, 24, 2, true, true),
                      new AudioFormat(22050.0F, 32, 1, true, false),
                      new AudioFormat(22050.0F, 32, 1, true, true),
                      new AudioFormat(22050.0F, 32, 2, true, false),
                      new AudioFormat(22050.0F, 32, 2, true, true),
       */
      new AudioFormat(24000.0F, 16, 1, true, false), // 20
      new AudioFormat(24000.0F, 16, 1, true, true), // 21
      new AudioFormat(24000.0F, 16, 2, true, false), // 22
      new AudioFormat(24000.0F, 16, 2, true, true), // 23
      /*	24 and 32 bit not yet possible
                      new AudioFormat(24000.0F, 24, 1, true, false),
                      new AudioFormat(24000.0F, 24, 1, true, true),
                      new AudioFormat(24000.0F, 24, 2, true, false),
                      new AudioFormat(24000.0F, 24, 2, true, true),
                      new AudioFormat(24000.0F, 32, 1, true, false),
                      new AudioFormat(24000.0F, 32, 1, true, true),
                      new AudioFormat(24000.0F, 32, 2, true, false),
                      new AudioFormat(24000.0F, 32, 2, true, true),
       */
      new AudioFormat(32000.0F, 16, 1, true, false), // 24
      new AudioFormat(32000.0F, 16, 1, true, true), // 25
      new AudioFormat(32000.0F, 16, 2, true, false), // 26
      new AudioFormat(32000.0F, 16, 2, true, true), // 27
      /*	24 and 32 bit not yet possible
                      new AudioFormat(32000.0F, 24, 1, true, false),
                      new AudioFormat(32000.0F, 24, 1, true, true),
                      new AudioFormat(32000.0F, 24, 2, true, false),
                      new AudioFormat(32000.0F, 24, 2, true, true),
                      new AudioFormat(32000.0F, 32, 1, true, false),
                      new AudioFormat(32000.0F, 32, 1, true, true),
                      new AudioFormat(32000.0F, 32, 2, true, false),
                      new AudioFormat(32000.0F, 32, 2, true, true),
       */
      new AudioFormat(44100.0F, 16, 1, true, false), // 28
      new AudioFormat(44100.0F, 16, 1, true, true), // 29
      new AudioFormat(44100.0F, 16, 2, true, false), // 30
      new AudioFormat(44100.0F, 16, 2, true, true), // 31
      /*	24 and 32 bit not yet possible
                      new AudioFormat(44100.0F, 24, 1, true, false),
                      new AudioFormat(44100.0F, 24, 1, true, true),
                      new AudioFormat(44100.0F, 24, 2, true, false),
                      new AudioFormat(44100.0F, 24, 2, true, true),
                      new AudioFormat(44100.0F, 32, 1, true, false),
                      new AudioFormat(44100.0F, 32, 1, true, true),
                      new AudioFormat(44100.0F, 32, 2, true, false),
                      new AudioFormat(44100.0F, 32, 2, true, true),
       */
      new AudioFormat(48000.0F, 16, 1, true, false), // 32
      new AudioFormat(48000.0F, 16, 1, true, true), // 33
      new AudioFormat(48000.0F, 16, 2, true, false), // 34
      new AudioFormat(48000.0F, 16, 2, true, true), // 35
      /*	24 and 32 bit not yet possible
                      new AudioFormat(48000.0F, 24, 1, true, false),
                      new AudioFormat(48000.0F, 24, 1, true, true),
                      new AudioFormat(48000.0F, 24, 2, true, false),
                      new AudioFormat(48000.0F, 24, 2, true, true),
                      new AudioFormat(48000.0F, 32, 1, true, false),
                      new AudioFormat(48000.0F, 32, 1, true, true),
                      new AudioFormat(48000.0F, 32, 2, true, false),
                      new AudioFormat(48000.0F, 32, 2, true, true),
       */
  };

  private static final boolean t = true;
  private static final boolean f = false;

  /*
   *	One row for each source format.
   */
  private static final boolean[][] CONVERSIONS =
  {
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,t,t,f,f,f,f, f,f,f,f,f,f},	// 0
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,t,t,f,f, f,f,f,f,f,f},	// 1
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,t,t, f,f,f,f,f,f},	// 2
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, t,t,f,f,f,f},	// 3
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,t,t,f,f},	// 4
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,t,t},	// 5

      {f,f,f,f,f,f,f,f,f,f, f,f,t,t,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 18
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,t,t,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 19
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,t,t,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 20
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,t,t, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 21
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, t,t,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 22
      {f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,t,t,f,f,f,f,f,f, f,f,f,f,f,f},	// 23

      {t,t,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 36
      {f,f,t,t,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 37
      {f,f,f,f,t,t,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 38
      {f,f,f,f,f,f,t,t,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 39
      {f,f,f,f,f,f,f,f,t,t, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 40
      {f,f,f,f,f,f,f,f,f,f, t,t,f,f,f,f,f,f,f,f, f,f,f,f,f,f,f,f,f,f, f,f,f,f,f,f},	// 41

  };

  /**
   * Constructor.
   */
  public VorbisFormatConversionProvider()
  {
    super(Arrays.asList(INPUT_FORMATS), Arrays.asList(OUTPUT_FORMATS), CONVERSIONS);
  }

  /**
   * Returns converted AudioInputStream.
   */
  public AudioInputStream getAudioInputStream(AudioFormat targetFormat, AudioInputStream audioInputStream)
  {
    if (isConversionSupported(targetFormat, audioInputStream.getFormat()))
    {
      return new DecodedVorbisAudioInputStream(targetFormat, audioInputStream);
    }
    else
    {
      throw new IllegalArgumentException("conversion not supported");
    }
  }
}
