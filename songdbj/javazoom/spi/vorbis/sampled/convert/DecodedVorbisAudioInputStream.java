/*
 *   DecodedVorbisAudioInputStream
 *   
 *    JavaZOOM : vorbisspi@javazoom.net
 *               http://www.javazoom.net
 *
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
 */
 
package javazoom.spi.vorbis.sampled.convert;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;

import javazoom.spi.PropertiesContainer;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.convert.TAsynchronousFilteredAudioInputStream;

import com.jcraft.jogg.Packet;
import com.jcraft.jogg.Page;
import com.jcraft.jogg.StreamState;
import com.jcraft.jogg.SyncState;
import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.Comment;
import com.jcraft.jorbis.DspState;
import com.jcraft.jorbis.Info;

/**
 * This class implements the Vorbis decoding.
 */
public class DecodedVorbisAudioInputStream extends TAsynchronousFilteredAudioInputStream implements PropertiesContainer
{
  private InputStream oggBitStream_ = null;

  private SyncState oggSyncState_ = null;
  private StreamState oggStreamState_ = null;
  private Page oggPage_ = null;
  private Packet oggPacket_ = null;
  private Info vorbisInfo = null;
  private Comment vorbisComment = null;
  private DspState vorbisDspState = null;
  private Block vorbisBlock = null;

  static final int playState_NeedHeaders = 0;
  static final int playState_ReadData = 1;
  static final int playState_WriteData = 2;
  static final int playState_Done = 3;
  static final int playState_BufferFull = 4;
  static final int playState_Corrupt = -1;
  private int playState;

  private int bufferMultiple_ = 4;
  private int bufferSize_ = bufferMultiple_ * 256 * 2;
  private int convsize = bufferSize_ * 2;
  private byte[] convbuffer = new byte[convsize];
  private byte[] buffer = null;
  private int bytes = 0;
  private float[][][] _pcmf = null;
  private int[] _index = null;
  private int index = 0;
  private int i = 0;
  // bout is now a global so that we can continue from when we have a buffer full.
  int bout = 0;
  
  private HashMap properties = null;
  private long currentBytes = 0;

  /**
   * Constructor.
   */
  public DecodedVorbisAudioInputStream(AudioFormat outputFormat, AudioInputStream bitStream)
  {
    super(outputFormat, -1);
    this.oggBitStream_ = bitStream;
    init_jorbis();
    index = 0;
    playState = playState_NeedHeaders;
	properties = new HashMap();
  }

  /**
   * Initializes all the jOrbis and jOgg vars that are used for song playback.
   */
  private void init_jorbis()
  {
    oggSyncState_ = new SyncState();
    oggStreamState_ = new StreamState();
    oggPage_ = new Page();
    oggPacket_ = new Packet();
    vorbisInfo = new Info();
    vorbisComment = new Comment();
    vorbisDspState = new DspState();
    vorbisBlock = new Block(vorbisDspState);
    buffer = null;
    bytes = 0;
	currentBytes = 0L;
    oggSyncState_.init();
  }

  /**
   * Return dynamic properties.
   * 
   * <ul>
   * <li><b>ogg.position.byte</b> [Long], current position in bytes in the stream.
   *</ul>
   */
  public Map properties()
  {
	  properties.put("ogg.position.byte",new Long(currentBytes));
	  return properties;	 
  }
  /**
   * Main loop.
   */
  public void execute()
  {
    if(TDebug.TraceAudioConverter)
    {
      switch(playState)
      {
        case playState_NeedHeaders:
          TDebug.out("playState = playState_NeedHeaders");
          break;
        case playState_ReadData:
          TDebug.out("playState = playState_ReadData");
          break;
        case playState_WriteData:
          TDebug.out("playState = playState_WriteData");
          break;
        case playState_Done:
          TDebug.out("playState = playState_Done");
          break;
        case playState_BufferFull:
          TDebug.out("playState = playState_BufferFull");
          break;
        case playState_Corrupt:
          TDebug.out("playState = playState_Corrupt");
          break;
      }
    }
    // This code was developed by the jCraft group, as JOrbisPlayer.java,  slightly
    // modified by jOggPlayer developer and adapted by JavaZOOM to suit the JavaSound
    // SPI. Then further modified by Tom Kimpton to correctly play ogg files that
    // would hang the player.
    switch(playState)
    {
      case playState_NeedHeaders:
        try
        {
          // Headers (+ Comments).
          readHeaders();
        }
        catch(IOException ioe)
        {
          playState = playState_Corrupt;
          return;
        }
        playState = playState_ReadData;
        break;

      case playState_ReadData:
        int result;
        index = oggSyncState_.buffer(bufferSize_);
        buffer = oggSyncState_.data;
        bytes = readFromStream(buffer, index, bufferSize_);
        if(TDebug.TraceAudioConverter) TDebug.out("More data : " + bytes);
        if(bytes == -1)
        {
          playState = playState_Done;
          if(TDebug.TraceAudioConverter) TDebug.out("Ogg Stream empty. Settings playState to playState_Done.");
          break;
        }
        else
        {
          oggSyncState_.wrote(bytes);
          if(bytes == 0)
          {
            if((oggPage_.eos() != 0) || (oggStreamState_.e_o_s != 0) || (oggPacket_.e_o_s != 0))
            {
              if(TDebug.TraceAudioConverter) TDebug.out("oggSyncState wrote 0 bytes: settings playState to playState_Done.");
              playState = playState_Done;
            }
              if(TDebug.TraceAudioConverter) TDebug.out("oggSyncState wrote 0 bytes: but stream not yet empty.");
            break;
          }
        }

        result = oggSyncState_.pageout(oggPage_);
        if(result == 0)
        {
          if(TDebug.TraceAudioConverter) TDebug.out("Setting playState to playState_ReadData.");
          playState = playState_ReadData;
          break;
        } // need more data
        if(result == -1)
        { // missing or corrupt data at this page position
          if(TDebug.TraceAudioConverter) TDebug.out("Corrupt or missing data in bitstream; setting playState to playState_ReadData");
          playState = playState_ReadData;
          break;
        }

        oggStreamState_.pagein(oggPage_);

        if(TDebug.TraceAudioConverter) TDebug.out("Setting playState to playState_WriteData.");
        playState = playState_WriteData;
        break;

      case playState_WriteData:
        // Decoding !
        if(TDebug.TraceAudioConverter) TDebug.out("Decoding");
        while(true)
        {
          result = oggStreamState_.packetout(oggPacket_);
          if(result == 0)
          {
            if(TDebug.TraceAudioConverter) TDebug.out("Packetout returned 0, going to read state.");
            playState = playState_ReadData;
            break;
          } // need more data
          else if(result == -1)
          { 
          	// missing or corrupt data at this page position
            // no reason to complain; already complained above
			if(TDebug.TraceAudioConverter) TDebug.out("Corrupt or missing data in packetout bitstream; going to read state...");
			// playState = playState_ReadData;
			// break;
            continue;
          }
          else
          {
            // we have a packet.  Decode it
            if(vorbisBlock.synthesis(oggPacket_) == 0)
            { // test for success!
              vorbisDspState.synthesis_blockin(vorbisBlock);
            }
            else
            {
              //if(TDebug.TraceAudioConverter) TDebug.out("vorbisBlock.synthesis() returned !0, going to read state");
              if(TDebug.TraceAudioConverter) TDebug.out("VorbisBlock.synthesis() returned !0, continuing.");
              continue;
            }

            outputSamples();
            if(playState == playState_BufferFull)
              return;

          } // else result != -1
        } // while(true)
        if(oggPage_.eos() != 0)
        {
          if(TDebug.TraceAudioConverter) TDebug.out("Settings playState to playState_Done.");
          playState = playState_Done;
        }
        break;
      case playState_BufferFull:
        continueFromBufferFull();
        break;

      case playState_Corrupt:
        if(TDebug.TraceAudioConverter) TDebug.out("Corrupt Song.");
        // drop through to playState_Done...
      case playState_Done:
        oggStreamState_.clear();
        vorbisBlock.clear();
        vorbisDspState.clear();
        vorbisInfo.clear();
        oggSyncState_.clear();
        if(TDebug.TraceAudioConverter) TDebug.out("Done Song.");
        try
        {
          if(oggBitStream_ != null)
          {
            oggBitStream_.close();
          }
          getCircularBuffer().close();
        }
        catch(Exception e)
        {
          if(TDebug.TraceAudioConverter) TDebug.out(e.getMessage());
        }
        break;
    } // switch
  }

  /**
   * This routine was extracted so that when the output buffer fills up,
   * we can break out of the loop, let the music channel drain, then
   * continue from where we were.
   */
  private void outputSamples()
  {
    int samples;
    while((samples = vorbisDspState.synthesis_pcmout(_pcmf, _index)) > 0)
    {
      float[][] pcmf = _pcmf[0];
      bout = (samples < convsize ? samples : convsize);
      double fVal = 0.0;
      // convert doubles to 16 bit signed ints (host order) and
      // interleave
      for(i = 0; i < vorbisInfo.channels; i++)
      {
        int pointer = i * 2;
        //int ptr=i;
        int mono = _index[i];
        for(int j = 0; j < bout; j++)
        {
          fVal = pcmf[i][mono + j] * 32767.;
          int val = (int) (fVal);
          if(val > 32767)
          {
            val = 32767;
          }
          if(val < -32768)
          {
            val = -32768;
          }
          if(val < 0)
          {
            val = val | 0x8000;
          }
          convbuffer[pointer] = (byte) (val);
          convbuffer[pointer + 1] = (byte) (val >>> 8);
          pointer += 2 * (vorbisInfo.channels);
        }
      }
      if(TDebug.TraceAudioConverter) TDebug.out("about to write: " + 2 * vorbisInfo.channels * bout);
      if(getCircularBuffer().availableWrite() < 2 * vorbisInfo.channels * bout)
      {
        if(TDebug.TraceAudioConverter) TDebug.out("Too much data in this data packet, better return, let the channel drain, and try again...");
        playState = playState_BufferFull;
        return;
      }
      getCircularBuffer().write(convbuffer, 0, 2 * vorbisInfo.channels * bout);
      if(bytes < bufferSize_)
        if(TDebug.TraceAudioConverter) TDebug.out("Finished with final buffer of music?");
      if(vorbisDspState.synthesis_read(bout) != 0)
      {
        if(TDebug.TraceAudioConverter) TDebug.out("VorbisDspState.synthesis_read returned -1.");
      }
    } // while(samples...)
    playState = playState_ReadData;
  }

  private void continueFromBufferFull()
  {
    if(getCircularBuffer().availableWrite() < 2 * vorbisInfo.channels * bout)
    {
      if(TDebug.TraceAudioConverter) TDebug.out("Too much data in this data packet, better return, let the channel drain, and try again...");
      // Don't change play state.
      return;
    }
    getCircularBuffer().write(convbuffer, 0, 2 * vorbisInfo.channels * bout);
    // Don't change play state. Let outputSamples change play state, if necessary.
    outputSamples();
  }
  /**
   * Reads headers and comments.
   */
  private void readHeaders() throws IOException
  {
    if(TDebug.TraceAudioConverter) TDebug.out("readHeaders(");
    index = oggSyncState_.buffer(bufferSize_);
    buffer = oggSyncState_.data;
    bytes = readFromStream(buffer, index, bufferSize_);
    if(bytes == -1)
    {
      if(TDebug.TraceAudioConverter) TDebug.out("Cannot get any data from selected Ogg bitstream.");
      throw new IOException("Cannot get any data from selected Ogg bitstream.");
    }
    oggSyncState_.wrote(bytes);
    if(oggSyncState_.pageout(oggPage_) != 1)
    {
      if(bytes < bufferSize_)
      {
        throw new IOException("EOF");
      }
      if(TDebug.TraceAudioConverter) TDebug.out("Input does not appear to be an Ogg bitstream.");
      throw new IOException("Input does not appear to be an Ogg bitstream.");
    }
    oggStreamState_.init(oggPage_.serialno());
    vorbisInfo.init();
    vorbisComment.init();
    if(oggStreamState_.pagein(oggPage_) < 0)
    {
      // error; stream version mismatch perhaps
      if(TDebug.TraceAudioConverter) TDebug.out("Error reading first page of Ogg bitstream data.");
      throw new IOException("Error reading first page of Ogg bitstream data.");
    }
    if(oggStreamState_.packetout(oggPacket_) != 1)
    {
      // no page? must not be vorbis
      if(TDebug.TraceAudioConverter) TDebug.out("Error reading initial header packet.");
      throw new IOException("Error reading initial header packet.");
    }
    if(vorbisInfo.synthesis_headerin(vorbisComment, oggPacket_) < 0)
    {
      // error case; not a vorbis header
      if(TDebug.TraceAudioConverter) TDebug.out("This Ogg bitstream does not contain Vorbis audio data.");
      throw new IOException("This Ogg bitstream does not contain Vorbis audio data.");
    }
    //int i = 0;
    i = 0;
    while(i < 2)
    {
      while(i < 2)
      {
        int result = oggSyncState_.pageout(oggPage_);
        if(result == 0)
        {
          break;
        } // Need more data
        if(result == 1)
        {
          oggStreamState_.pagein(oggPage_);
          while(i < 2)
          {
            result = oggStreamState_.packetout(oggPacket_);
            if(result == 0)
            {
              break;
            }
            if(result == -1)
            {
              if(TDebug.TraceAudioConverter) TDebug.out("Corrupt secondary header.  Exiting.");
              throw new IOException("Corrupt secondary header.  Exiting.");
            }
            vorbisInfo.synthesis_headerin(vorbisComment, oggPacket_);
            i++;
          }
        }
      }
      index = oggSyncState_.buffer(bufferSize_);
      buffer = oggSyncState_.data;
      bytes = readFromStream(buffer, index, bufferSize_);
      if(bytes == -1)
      {
        break;
      }
      if(bytes == 0 && i < 2)
      {
        if(TDebug.TraceAudioConverter) TDebug.out("End of file before finding all Vorbis headers!");
        throw new IOException("End of file before finding all Vorbis  headers!");
      }
      oggSyncState_.wrote(bytes);
    }

    byte[][] ptr = vorbisComment.user_comments;
    String currComment = "";

    for(int j = 0; j < ptr.length; j++)
    {
      if(ptr[j] == null)
      {
        break;
      }
      currComment = (new String(ptr[j], 0, ptr[j].length - 1)).trim();
      if(TDebug.TraceAudioConverter) TDebug.out("Comment: " + currComment);
    }
    convsize = bufferSize_ / vorbisInfo.channels;
    vorbisDspState.synthesis_init(vorbisInfo);
    vorbisBlock.init(vorbisDspState);
    _pcmf = new float[1][][];
    _index = new int[vorbisInfo.channels];
  }

  /**
   * Reads from the oggBitStream_ a specified number of Bytes(bufferSize_) worth
   * starting at index and puts them in the specified buffer[].
   *
   * @param buffer
   * @param index
   * @param bufferSize_
   * @return             the number of bytes read or -1 if error.
   */
  private int readFromStream(byte[] buffer, int index, int bufferSize_)
  {
    int bytes = 0;
    try
    {
      bytes = oggBitStream_.read(buffer, index, bufferSize_);
    }
    catch(Exception e)
    {
      if(TDebug.TraceAudioConverter) TDebug.out("Cannot Read Selected Song");
      bytes = -1;
    }
    currentBytes = currentBytes + bytes;
    return bytes;
  }

  /**
   * Close the stream.
   */
  public void close() throws IOException
  {
    super.close();
    oggBitStream_.close();
  }
}
