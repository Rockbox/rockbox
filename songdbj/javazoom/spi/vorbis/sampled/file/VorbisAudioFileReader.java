/*
 *   VorbisAudioFileReader.
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

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.HashMap;
import java.util.StringTokenizer;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.tritonus.share.TDebug;
import org.tritonus.share.sampled.file.TAudioFileReader;

import com.jcraft.jogg.Packet;
import com.jcraft.jogg.Page;
import com.jcraft.jogg.StreamState;
import com.jcraft.jogg.SyncState;
import com.jcraft.jorbis.Block;
import com.jcraft.jorbis.Comment;
import com.jcraft.jorbis.DspState;
import com.jcraft.jorbis.Info;
import com.jcraft.jorbis.JOrbisException;
import com.jcraft.jorbis.VorbisFile;

/**
 * This class implements the AudioFileReader class and provides an
 * Ogg Vorbis file reader for use with the Java Sound Service Provider Interface.
 */
public class VorbisAudioFileReader extends TAudioFileReader
{
  private SyncState oggSyncState_ = null;
  private StreamState oggStreamState_ = null;
  private Page oggPage_ = null;
  private Packet oggPacket_ = null;
  private Info vorbisInfo = null;
  private Comment vorbisComment = null;
  private DspState vorbisDspState = null;
  private Block vorbisBlock = null;
  private int bufferMultiple_ = 4;
  private int bufferSize_ = bufferMultiple_ * 256 * 2;
  private int convsize = bufferSize_ * 2;
  private byte[] convbuffer = new byte[convsize];
  private byte[] buffer = null;
  private int bytes = 0;
  private int rate = 0;
  private int channels = 0;

  private int index = 0;
  private InputStream oggBitStream_ = null;

  private static final int	INITAL_READ_LENGTH = 64000;
  private static final int	MARK_LIMIT = INITAL_READ_LENGTH + 1;
  
  public VorbisAudioFileReader()
  {
	  super(MARK_LIMIT, true);
  }

  /**
   * Return the AudioFileFormat from the given file.
   */
  public AudioFileFormat getAudioFileFormat(File file) throws UnsupportedAudioFileException, IOException
  {
	if (TDebug.TraceAudioFileReader) TDebug.out("getAudioFileFormat(File file)");
    InputStream inputStream = null;
    try
    {	  
	  inputStream = new BufferedInputStream(new FileInputStream(file));	
	  inputStream.mark(MARK_LIMIT);	    
	  AudioFileFormat aff = getAudioFileFormat(inputStream);
	  inputStream.reset();
      // Get Vorbis file info such as length in seconds.
      VorbisFile vf = new VorbisFile(file.getAbsolutePath());      
      return getAudioFileFormat(inputStream,(int) file.length(), (int) Math.round((vf.time_total(-1))*1000));
    }
	catch (JOrbisException e)
	{
		throw new IOException(e.getMessage());
	}
   finally
   {
     if (inputStream != null) inputStream.close();
   }
  }

  /**
   * Return the AudioFileFormat from the given URL.
   */
  public AudioFileFormat getAudioFileFormat(URL url) throws UnsupportedAudioFileException, IOException
  {
	if (TDebug.TraceAudioFileReader) TDebug.out("getAudioFileFormat(URL url)");
    InputStream inputStream = url.openStream();
    try
    {
      return getAudioFileFormat(inputStream);
    }
    finally
    {
		if (inputStream != null) inputStream.close();
    }
  }

  /**
   * Return the AudioFileFormat from the given InputStream.
   */
  public AudioFileFormat getAudioFileFormat(InputStream inputStream) throws UnsupportedAudioFileException, IOException
  {
	if (TDebug.TraceAudioFileReader) TDebug.out("getAudioFileFormat(InputStream inputStream)");
	try
	{
		if (!inputStream.markSupported()) inputStream = new BufferedInputStream(inputStream);
		inputStream.mark(MARK_LIMIT);
		return getAudioFileFormat(inputStream, AudioSystem.NOT_SPECIFIED, AudioSystem.NOT_SPECIFIED);
	}
	finally
	{
		inputStream.reset();
	}    
  }

  /**
   * Return the AudioFileFormat from the given InputStream and length in bytes.
   */
  public AudioFileFormat getAudioFileFormat(InputStream inputStream, long medialength) throws UnsupportedAudioFileException, IOException
  {
	return getAudioFileFormat(inputStream, (int) medialength, AudioSystem.NOT_SPECIFIED);
  }

  
  /**
   * Return the AudioFileFormat from the given InputStream, length in bytes and length in milliseconds.
   */
  protected AudioFileFormat getAudioFileFormat(InputStream bitStream, int mediaLength, int totalms) throws UnsupportedAudioFileException, IOException
  {
	HashMap aff_properties = new HashMap();
	HashMap af_properties = new HashMap();
    if (totalms == AudioSystem.NOT_SPECIFIED)
    {
      totalms = 0;
    }
    if (totalms <= 0)
    {
      totalms = 0;
    }
    else
    {
		aff_properties.put("duration",new Long(totalms*1000));    	
    }
    oggBitStream_ = bitStream;
    init_jorbis();
    index = 0;
    try
    {
      readHeaders(aff_properties, af_properties);
    }
    catch (IOException ioe)
    {
      if (TDebug.TraceAudioFileReader)
      {
		TDebug.out(ioe.getMessage());
      }
      throw new UnsupportedAudioFileException(ioe.getMessage());
    }

    String dmp = vorbisInfo.toString();
    if (TDebug.TraceAudioFileReader)
    {
		TDebug.out(dmp);
    }
    int ind = dmp.lastIndexOf("bitrate:");    
    int minbitrate = -1;
	int nominalbitrate = -1;
	int maxbitrate = -1;
    if (ind != -1)
    {
      dmp = dmp.substring(ind + 8, dmp.length());
      StringTokenizer st = new StringTokenizer(dmp, ",");
	  if (st.hasMoreTokens())
	  {
		minbitrate = Integer.parseInt(st.nextToken());
	  }
	  if (st.hasMoreTokens())
	  {
		nominalbitrate = Integer.parseInt(st.nextToken());
	  }
	  if (st.hasMoreTokens())
	  {
		maxbitrate = Integer.parseInt(st.nextToken());
	  }
    }
	if (nominalbitrate > 0) af_properties.put("bitrate",new Integer(nominalbitrate));
	af_properties.put("vbr",new Boolean(true));
	
	if (minbitrate > 0)  aff_properties.put("ogg.bitrate.min.bps",new Integer(minbitrate));
	if (maxbitrate > 0)  aff_properties.put("ogg.bitrate.max.bps",new Integer(maxbitrate));
	if (nominalbitrate > 0) aff_properties.put("ogg.bitrate.nominal.bps",new Integer(nominalbitrate));
	if (vorbisInfo.channels > 0) aff_properties.put("ogg.channels",new Integer(vorbisInfo.channels));
	if (vorbisInfo.rate > 0) aff_properties.put("ogg.frequency.hz",new Integer(vorbisInfo.rate));
	if (mediaLength > 0) aff_properties.put("ogg.length.bytes",new Integer(mediaLength));
	aff_properties.put("ogg.version",new Integer(vorbisInfo.version));
	
    AudioFormat.Encoding encoding = VorbisEncoding.VORBISENC;
    AudioFormat format = new VorbisAudioFormat(encoding, vorbisInfo.rate, AudioSystem.NOT_SPECIFIED, vorbisInfo.channels, AudioSystem.NOT_SPECIFIED, AudioSystem.NOT_SPECIFIED, true,af_properties);
    AudioFileFormat.Type type = VorbisFileFormatType.OGG;
	return new VorbisAudioFileFormat(VorbisFileFormatType.OGG, format, AudioSystem.NOT_SPECIFIED, mediaLength,aff_properties);
  }

  /**
   * Return the AudioInputStream from the given InputStream.
   */
  public AudioInputStream getAudioInputStream(InputStream inputStream) throws UnsupportedAudioFileException, IOException
  {
   if (TDebug.TraceAudioFileReader) TDebug.out("getAudioInputStream(InputStream inputStream)");
   return getAudioInputStream(inputStream, AudioSystem.NOT_SPECIFIED, AudioSystem.NOT_SPECIFIED);
  }

  /**
   * Return the AudioInputStream from the given InputStream.
   */
  public AudioInputStream getAudioInputStream(InputStream inputStream, int medialength, int totalms) throws UnsupportedAudioFileException, IOException
  {
	if (TDebug.TraceAudioFileReader) TDebug.out("getAudioInputStream(InputStream inputStreamint medialength, int totalms)");
	try
	{		
		if (!inputStream.markSupported()) inputStream = new BufferedInputStream(inputStream);
		inputStream.mark(MARK_LIMIT);
		AudioFileFormat audioFileFormat = getAudioFileFormat(inputStream, medialength, totalms);
		inputStream.reset();
		return new AudioInputStream(inputStream, audioFileFormat.getFormat(), audioFileFormat.getFrameLength());
	}
	catch (UnsupportedAudioFileException e)
	{
		inputStream.reset();
		throw e;
	}
	catch (IOException e)
	{
		inputStream.reset();
		throw e;
	}
  }

  /**
   * Return the AudioInputStream from the given File.
   */
  public AudioInputStream getAudioInputStream(File file) throws UnsupportedAudioFileException, IOException
  {
	if (TDebug.TraceAudioFileReader) TDebug.out("getAudioInputStream(File file)");
    InputStream inputStream = new FileInputStream(file);
    try
    {
      return getAudioInputStream(inputStream);
    }
    catch (UnsupportedAudioFileException e)
    {
	  if (inputStream != null) inputStream.close();
      throw e;
    }
    catch (IOException e)
    {
	  if (inputStream != null) inputStream.close();
      throw e;
    }
  }

  /**
   * Return the AudioInputStream from the given URL.
   */
  public AudioInputStream getAudioInputStream(URL url) throws UnsupportedAudioFileException, IOException
  {
	if (TDebug.TraceAudioFileReader) TDebug.out("getAudioInputStream(URL url)");
    InputStream inputStream = url.openStream();
    try
    {
      return getAudioInputStream(inputStream);
    }
    catch (UnsupportedAudioFileException e)
    {
	  if (inputStream != null) inputStream.close();
      throw e;
    }
    catch (IOException e)
    {
	  if (inputStream != null) inputStream.close();
      throw e;
    }
  }

  /**
   * Reads headers and comments.
   */
  private void readHeaders(HashMap aff_properties, HashMap af_properties) throws IOException
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
	aff_properties.put("ogg.serial",new Integer(oggPage_.serialno()));
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
	int i = 0;
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
	// Read Ogg Vorbis comments.
	byte[][] ptr = vorbisComment.user_comments;
	String currComment = "";
	int c = 0;
	for(int j = 0; j < ptr.length; j++)
	{
	  if(ptr[j] == null)
	  {
		break;
	  }
	  currComment = (new String(ptr[j], 0, ptr[j].length - 1)).trim();
	  if(TDebug.TraceAudioConverter) TDebug.out(currComment);
	  if (currComment.toLowerCase().startsWith("artist"))
	  {
		aff_properties.put("author",currComment.substring(7));	  	
	  }
	  else if (currComment.toLowerCase().startsWith("title"))
	  {
		aff_properties.put("title",currComment.substring(6));	  	
	  }
	  else if (currComment.toLowerCase().startsWith("album"))
	  {
		aff_properties.put("album",currComment.substring(6));	  	
	  }
	  else if (currComment.toLowerCase().startsWith("date"))
	  {
		aff_properties.put("date",currComment.substring(5));
	  }
	  else if (currComment.toLowerCase().startsWith("copyright"))
	  {
		aff_properties.put("copyright",currComment.substring(10));	  	
	  }
	  else if (currComment.toLowerCase().startsWith("comment"))
	  {
		aff_properties.put("comment",currComment.substring(8));
	  }
	  else if (currComment.toLowerCase().startsWith("genre"))
	  {
		aff_properties.put("ogg.comment.genre",currComment.substring(6));	
	  }
	  else if (currComment.toLowerCase().startsWith("tracknumber"))
	  {
		aff_properties.put("ogg.comment.track",currComment.substring(12));	
	  }
	  else
	  {
		c++;
		aff_properties.put("ogg.comment.ext."+c,currComment);
	  }
	  aff_properties.put("ogg.comment.encodedby",new String(vorbisComment.vendor, 0, vorbisComment.vendor.length - 1));
	}
  }

  /**
   * Reads from the oggBitStream_ a specified number of Bytes(bufferSize_) worth
   * starting at index and puts them in the specified buffer[].
   *
   * @return the number of bytes read or -1 if error.
   */
  private int readFromStream(byte[] buffer, int index, int bufferSize_)
  {
    int bytes = 0;
    try
    {
      bytes = oggBitStream_.read(buffer, index, bufferSize_);
    }
    catch (Exception e)
    {
      if (TDebug.TraceAudioFileReader)
      {
        TDebug.out("Cannot Read Selected Song");
      }
      bytes = -1;
    }
    return bytes;
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
    oggSyncState_.init();
  }
}
