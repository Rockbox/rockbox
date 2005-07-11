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
 * Revision 1.1.1.1  2004/04/04 22:09:12  shred
 * First Import
 *
 * Revision 1.2  2003/04/10 19:48:22  jarnbjo
 * no message
 *
 * Revision 1.1  2003/03/03 21:02:20  jarnbjo
 * no message
 *
 */

package de.jarnbjo.ogg;

import java.io.IOException;

/**
 * Interface providing access to a logical Ogg stream as part of a
 * physical Ogg stream.
 */


public interface LogicalOggStream {

   public static final String FORMAT_UNKNOWN = "application/octet-stream";

   public static final String FORMAT_VORBIS  = "audio/x-vorbis";
   public static final String FORMAT_FLAC    = "audio/x-flac";
   public static final String FORMAT_THEORA  = "video/x-theora";

   /**
    * <i>Note:</i> To read from the stream, you must use either
    * this method or the method <code>getNextOggPacket</code>.
    * Mixing calls to the two methods will cause data corruption.
    *
    * @return the next Ogg page
    *
    * @see #getNextOggPacket()
    *
    * @throws OggFormatException if the ogg stream is corrupted
    * @throws IOException if some other IO error occurs
    */

   public OggPage getNextOggPage() throws OggFormatException, IOException;

   /**
    * <i>Note:</i> To read from the stream, you must use either
    * this method or the method <code>getNextOggPage</code>.
    * Mixing calls to the two methods will cause data corruption.
    *
    * @return the next packet as a byte array
    *
    * @see #getNextOggPage()
    *
    * @throws OggFormatException if the ogg stream is corrupted
    * @throws IOException if some other IO error occurs
    */

   public byte[] getNextOggPacket() throws OggFormatException, IOException;

   /**
    * Checks if this stream is open for reading.
    *
    * @return <code>true</code> if this stream is open for reading,
    *         <code>false</code> otherwise
    */

   public boolean isOpen();

   /**
    * Closes this stream. After invoking this method, no further access
    * to the streams data is possible.
    *
    * @throws IOException if an IO error occurs
    */

   public void close() throws IOException;

   /**
    * Sets the stream's position to the beginning of the stream.
    * This method does not work if the physical Ogg stream is not
    * seekable.
    *
    * @throws OggFormatException if the ogg stream is corrupted
    * @throws IOException if some other IO error occurs
    */

   public void reset() throws OggFormatException, IOException;

   /**
    * This method does not work if the physical Ogg stream is not
    * seekable.
    *
    * @return the granule position of the last page within
    *         this stream
    */

   public long getMaximumGranulePosition();

   /**
    * This method is invoked on all logical streams when
    * calling the same method on the physical stream. The
    * same restrictions as mentioned there apply.
    * This method does not work if the physical Ogg stream is not
    * seekable.
    *
    * @param granulePosition
    *
    * @see PhysicalOggStream#setTime(long)
    *
    * @throws IOException if an IO error occurs
    */

   public void setTime(long granulePosition) throws IOException;

   /**
    *  @return the last parsed granule position of this stream
    */
    
   public long getTime();

   /**
    *  @return the content type of this stream
    *
    *  @see #FORMAT_UNKNOWN
    *  @see #FORMAT_VORBIS
    *  @see #FORMAT_FLAC
    *  @see #FORMAT_THEORA
    */

   public String getFormat();
}