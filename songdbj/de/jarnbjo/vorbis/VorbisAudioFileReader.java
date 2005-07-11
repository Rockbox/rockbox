package de.jarnbjo.vorbis;

/*
 * $ProjectName$
 * $ProjectRevision$
 * -----------------------------------------------------------
 * $Id$
 * -----------------------------------------------------------
 *
 * $Author$
 *
 * Description:
 *
 * Copyright 2002-2003 Tor-Einar Jarnbjo
 * -----------------------------------------------------------
 *
 * Change History
 * -----------------------------------------------------------
 * $Log$
 * Revision 1.1  2005/07/11 15:42:36  hcl
 * Songdb java version, source. only 1.5 compatible
 *
 * Revision 1.2  2004/09/21 06:39:06  shred
 * Importe reorganisiert, damit Eclipse Ruhe gibt. ;-)
 *
 * Revision 1.1.1.1  2004/04/04 22:09:12  shred
 * First Import
 *
 *
 */
 
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.net.URL;
import java.util.Collection;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import javax.sound.sampled.spi.AudioFileReader;

import de.jarnbjo.ogg.BasicStream;
import de.jarnbjo.ogg.EndOfOggStreamException;
import de.jarnbjo.ogg.FileStream;
import de.jarnbjo.ogg.LogicalOggStream;
import de.jarnbjo.ogg.OggFormatException;
import de.jarnbjo.ogg.PhysicalOggStream;
import de.jarnbjo.ogg.UncachedUrlStream;

public class VorbisAudioFileReader extends AudioFileReader {
	
	public VorbisAudioFileReader() {
	}
	
	public AudioFileFormat getAudioFileFormat(File file) throws IOException, UnsupportedAudioFileException {
		try {
			return getAudioFileFormat(new FileStream(new RandomAccessFile(file, "r")));	
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());	
		}
	}
	
	public AudioFileFormat getAudioFileFormat(InputStream stream) throws IOException, UnsupportedAudioFileException {
		try {
			return getAudioFileFormat(new BasicStream(stream));	
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());	
		}		
	}
	
	public AudioFileFormat getAudioFileFormat(URL url) throws IOException, UnsupportedAudioFileException {
		try {
			return getAudioFileFormat(new UncachedUrlStream(url));	
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());	
		}		
	}
	
	private AudioFileFormat getAudioFileFormat(PhysicalOggStream oggStream)  throws IOException, UnsupportedAudioFileException {
		try {
			Collection streams=oggStream.getLogicalStreams();
			if(streams.size()!=1) {
				throw new UnsupportedAudioFileException("Only Ogg files with one logical Vorbis stream are supported.");	
			}
			
			LogicalOggStream los=(LogicalOggStream)streams.iterator().next();
			if(los.getFormat()!=LogicalOggStream.FORMAT_VORBIS) {
				throw new UnsupportedAudioFileException("Only Ogg files with one logical Vorbis stream are supported.");	
			}

			VorbisStream vs=new VorbisStream(los);
			
	        AudioFormat audioFormat=new AudioFormat(
            	(float)vs.getIdentificationHeader().getSampleRate(),
            	16,
            	vs.getIdentificationHeader().getChannels(),
            	true, true);
			
			return new AudioFileFormat(VorbisFormatType.getInstance(), audioFormat, AudioSystem.NOT_SPECIFIED);
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());
		}
		catch(VorbisFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());
		}
	}
	
	
	
	public AudioInputStream getAudioInputStream(File file) throws IOException, UnsupportedAudioFileException {
		try {
			return getAudioInputStream(new FileStream(new RandomAccessFile(file, "r")));	
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());	
		}
	}
	
	public AudioInputStream getAudioInputStream(InputStream stream) throws IOException, UnsupportedAudioFileException {
		try {
			return getAudioInputStream(new BasicStream(stream));	
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());	
		}		
	}
	
	public AudioInputStream getAudioInputStream(URL url) throws IOException, UnsupportedAudioFileException {
		try {
			return getAudioInputStream(new UncachedUrlStream(url));	
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());	
		}		
	}
	
	private AudioInputStream getAudioInputStream(PhysicalOggStream oggStream)  throws IOException, UnsupportedAudioFileException {
		try {
			Collection streams=oggStream.getLogicalStreams();
			if(streams.size()!=1) {
				throw new UnsupportedAudioFileException("Only Ogg files with one logical Vorbis stream are supported.");	
			}
			
			LogicalOggStream los=(LogicalOggStream)streams.iterator().next();
			if(los.getFormat()!=LogicalOggStream.FORMAT_VORBIS) {
				throw new UnsupportedAudioFileException("Only Ogg files with one logical Vorbis stream are supported.");	
			}

			VorbisStream vs=new VorbisStream(los);

	        AudioFormat audioFormat=new AudioFormat(
            	(float)vs.getIdentificationHeader().getSampleRate(),
            	16,
            	vs.getIdentificationHeader().getChannels(),
            	true, true);
			
         	return new AudioInputStream(new VorbisInputStream(vs), audioFormat, -1);
		}
		catch(OggFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());
		}
		catch(VorbisFormatException e) {
			throw new UnsupportedAudioFileException(e.getMessage());
		}
	}
	
	
	public static class VorbisFormatType extends AudioFileFormat.Type {
		
		private static final VorbisFormatType instance=new VorbisFormatType();
		
		private VorbisFormatType() {
			super("VORBIS", "ogg");
		}
		
		public static AudioFileFormat.Type getInstance() {
			return instance;
		}
	}
	
   	public static class VorbisInputStream extends InputStream {

      	private VorbisStream source;
      	private byte[] buffer=new byte[8192];

      	public VorbisInputStream(VorbisStream source) {
         	this.source=source;
      	}

      	public int read() throws IOException {
         	return 0;
      	}

      	public int read(byte[] buffer) throws IOException {
         	return read(buffer, 0, buffer.length);
      	}

      	public int read(byte[] buffer, int offset, int length) throws IOException {
         	try {
            	return source.readPcm(buffer, offset, length);
         	}
         	catch(EndOfOggStreamException e) {
            	return -1;
         	}
      	}
   	}

	
}