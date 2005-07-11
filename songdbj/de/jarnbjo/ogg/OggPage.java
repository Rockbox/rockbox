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
 * Revision 1.3  2003/04/10 19:48:22  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/31 00:23:04  jarnbjo
 * no message
 *
 * Revision 1.1  2003/03/03 21:02:20  jarnbjo
 * no message
 *
 */

package de.jarnbjo.ogg;

import java.io.*;

import de.jarnbjo.util.io.*;

/**
 * <p>An instance of this class represents an ogg page read from an ogg file
 * or network stream. It has no public constructor, but instances can be
 * created by the <code>create</code> methods, supplying a JMF stream or
 * a <code>RandomAccessFile</code>
 * which is positioned at the beginning of an Ogg page.</p>
 *
 * <p>Furtheron, the class provides methods for accessing the raw page data,
 * as well as data attributes like segmenting information, sequence number,
 * stream serial number, chechsum and wether this page is the beginning or
 * end of a logical bitstream (BOS, EOS) and if the page data starts with a
 * continued packet or a fresh data packet.</p>
 */

public class OggPage {

   private int version;
   private boolean continued, bos, eos;
   private long absoluteGranulePosition;
   private int streamSerialNumber, pageSequenceNumber, pageCheckSum;
   private int[] segmentOffsets;
   private int[] segmentLengths;
   private int totalLength;
   private byte[] header, segmentTable, data;

   protected OggPage() {
   }

   private OggPage(
      int version,
      boolean continued,
      boolean bos,
      boolean eos,
      long absoluteGranulePosition,
      int streamSerialNumber,
      int pageSequenceNumber,
      int pageCheckSum,
      int[] segmentOffsets,
      int[] segmentLengths,
      int totalLength,
      byte[] header,
      byte[] segmentTable,
      byte[] data) {

      this.version=version;
      this.continued=continued;
      this.bos=bos;
      this.eos=eos;
      this.absoluteGranulePosition=absoluteGranulePosition;
      this.streamSerialNumber=streamSerialNumber;
      this.pageSequenceNumber=pageSequenceNumber;
      this.pageCheckSum=pageCheckSum;
      this.segmentOffsets=segmentOffsets;
      this.segmentLengths=segmentLengths;
      this.totalLength=totalLength;
      this.header=header;
      this.segmentTable=segmentTable;
      this.data=data;
   }

	/**
	 *  this method equals to create(RandomAccessFile source, false)
	 *
	 *  @see #create(RandomAccessFile, boolean)
	 */

   public static OggPage create(RandomAccessFile source) throws IOException, EndOfOggStreamException, OggFormatException  {
      return create(source, false);
   }

   /**
    * This method is called to read data from the current position in the
    * specified RandomAccessFile and create a new OggPage instance based on the data
    * read. If the parameter <code>skipData</code> is set to <code>true</code>,
    * the actual page segments (page data) is skipped and not read into
    * memory. This mode is useful when scanning through an ogg file to build
    * a seek table.
    *
    * @param source the source from which the ogg page is generated
    * @param skipData if set to <code>true</code>, the actual page data is not read into memory
    * @return an ogg page created by reading data from the specified source, starting at the current position
    * @throws FormatException if the data read from the specified source is not matching the specification for an ogg page
    * @throws EndOfStreamException if it is not possible to read an entire ogg page from the specified source
    * @throws IOException if some other I/O error is detected when reading from the source
    *
    * @see #create(RandomAccessFile)
    */

   public static OggPage create(RandomAccessFile source, boolean skipData) throws IOException, EndOfOggStreamException, OggFormatException  {
      return create((Object)source, skipData);
   }

	/**
	 *  this method equals to create(InputStream source, false)
	 *
	 *  @see #create(InputStream, boolean)
	 */

   public static OggPage create(InputStream source) throws IOException, EndOfOggStreamException, OggFormatException  {
      return create(source, false);
   }

   /**
    * This method is called to read data from the current position in the
    * specified InpuStream and create a new OggPage instance based on the data
    * read. If the parameter <code>skipData</code> is set to <code>true</code>,
    * the actual page segments (page data) is skipped and not read into
    * memory. This mode is useful when scanning through an ogg file to build
    * a seek table.
    *
    * @param source the source from which the ogg page is generated
    * @param skipData if set to <code>true</code>, the actual page data is not read into memory
    * @return an ogg page created by reading data from the specified source, starting at the current position
    * @throws FormatException if the data read from the specified source is not matching the specification for an ogg page
    * @throws EndOfStreamException if it is not possible to read an entire ogg page from the specified source
    * @throws IOException if some other I/O error is detected when reading from the source
    *
    * @see #create(InputStream)
    */

   public static OggPage create(InputStream source, boolean skipData) throws IOException, EndOfOggStreamException, OggFormatException  {
      return create((Object)source, skipData);
   }

	/**
	 *  this method equals to create(byte[] source, false)
	 *
	 *  @see #create(byte[], boolean)
	 */

   public static OggPage create(byte[] source) throws IOException, EndOfOggStreamException, OggFormatException  {
      return create(source, false);
   }

   /**
    * This method is called to
    * create a new OggPage instance based on the specified byte array.
    *
    * @param source the source from which the ogg page is generated
    * @param skipData if set to <code>true</code>, the actual page data is not read into memory
    * @return an ogg page created by reading data from the specified source, starting at the current position
    * @throws FormatException if the data read from the specified source is not matching the specification for an ogg page
    * @throws EndOfStreamException if it is not possible to read an entire ogg page from the specified source
    * @throws IOException if some other I/O error is detected when reading from the source
    *
    * @see #create(byte[])
    */

   public static OggPage create(byte[] source, boolean skipData) throws IOException, EndOfOggStreamException, OggFormatException  {
      return create((Object)source, skipData);
   }

   private static OggPage create(Object source, boolean skipData) throws IOException, EndOfOggStreamException, OggFormatException  {

      try {
         int sourceOffset=27;

         byte[] header=new byte[27];
         if(source instanceof RandomAccessFile) {
            RandomAccessFile raf=(RandomAccessFile)source;
            if(raf.getFilePointer()==raf.length()) {
               return null;
            }
            raf.readFully(header);
         }
         else if(source instanceof InputStream) {
            readFully((InputStream)source, header);
         }
         else if(source instanceof byte[]) {
            System.arraycopy((byte[])source, 0, header, 0, 27);
         }

         BitInputStream bdSource=new ByteArrayBitInputStream(header);

         int capture=bdSource.getInt(32);

         if(capture!=0x5367674f) {
            //throw new FormatException("Ogg page does not start with 'OggS' (0x4f676753)");

            /*
            ** This condition is IMHO an error, but older Ogg files often contain
            ** pages with a different capture than OggS. I am not sure how to
            ** manage these pages, but the decoder seems to work properly, if
            ** the incorrect capture is simply ignored.
            */

            String cs=Integer.toHexString(capture);
            while(cs.length()<8) {
               cs="0"+cs;
            }
            cs=cs.substring(6, 8)+cs.substring(4, 6)+cs.substring(2, 4)+cs.substring(0, 2);
            char c1=(char)(Integer.valueOf(cs.substring(0, 2), 16).intValue());
            char c2=(char)(Integer.valueOf(cs.substring(2, 4), 16).intValue());
            char c3=(char)(Integer.valueOf(cs.substring(4, 6), 16).intValue());
            char c4=(char)(Integer.valueOf(cs.substring(6, 8), 16).intValue());
            System.out.println("Ogg packet header is 0x"+cs+" ("+c1+c2+c3+c4+"), should be 0x4f676753 (OggS)");
         }

         int version=bdSource.getInt(8);
         byte tmp=(byte)bdSource.getInt(8);
         boolean bf1=(tmp&1)!=0;
         boolean bos=(tmp&2)!=0;
         boolean eos=(tmp&4)!=0;
         long absoluteGranulePosition=bdSource.getLong(64);
         int streamSerialNumber=bdSource.getInt(32);
         int pageSequenceNumber=bdSource.getInt(32);
         int pageCheckSum=bdSource.getInt(32);
         int pageSegments=bdSource.getInt(8);

         //System.out.println("OggPage: "+streamSerialNumber+" / "+absoluteGranulePosition+" / "+pageSequenceNumber);

         int[] segmentOffsets=new int[pageSegments];
         int[] segmentLengths=new int[pageSegments];
         int totalLength=0;

         byte[] segmentTable=new byte[pageSegments];
         byte[] tmpBuf=new byte[1];

         for(int i=0; i<pageSegments; i++) {
            int l=0;
            if(source instanceof RandomAccessFile) {
               l=((int)((RandomAccessFile)source).readByte()&0xff);
            }
            else if(source instanceof InputStream) {
               l=(int)((InputStream)source).read();
            }
            else if(source instanceof byte[]) {
               l=(int)((byte[])source)[sourceOffset++];
               l&=255;
            }
            segmentTable[i]=(byte)l;
            segmentLengths[i]=l;
            segmentOffsets[i]=totalLength;
            totalLength+=l;
         }

         byte[] data=null;

         if(!skipData) {

            //System.out.println("createPage: "+absoluteGranulePosition*1000/44100);

            data=new byte[totalLength];
            //source.read(data, 0, totalLength);
            if(source instanceof RandomAccessFile) {
               ((RandomAccessFile)source).readFully(data);
            }
            else if(source instanceof InputStream) {
               readFully((InputStream)source, data);
            }
            else if(source instanceof byte[]) {
               System.arraycopy(source, sourceOffset, data, 0, totalLength);
            }
         }

         return new OggPage(version, bf1, bos, eos, absoluteGranulePosition, streamSerialNumber, pageSequenceNumber, pageCheckSum, segmentOffsets, segmentLengths, totalLength, header, segmentTable, data);
      }
      catch(EOFException e) {
         throw new EndOfOggStreamException();
      }
   }

   private static void readFully(InputStream source, byte[] buffer) throws IOException {
      int total=0;
      while(total<buffer.length) {
         int read=source.read(buffer, total, buffer.length-total);
         if(read==-1) {
            throw new EndOfOggStreamException();
         }
         total+=read;
      }
   }

   /**
    * Returns the absolute granule position of the last complete
    * packet contained in this Ogg page, or -1 if the page contains a single
    * packet, which is not completed on this page. For pages containing Vorbis
    * data, this value is the sample index within the Vorbis stream. The Vorbis
    * stream does not necessarily start with sample index 0.
    *
    * @return the absolute granule position of the last packet completed on
    *         this page
    */


   public long getAbsoluteGranulePosition() {
      return absoluteGranulePosition;
   }

   /**
    * Returns the stream serial number of this ogg page.
    *
    * @return this page's serial number
    */

   public int getStreamSerialNumber() {
      return streamSerialNumber;
   }

   /**
    * Return the sequnce number of this ogg page.
    *
    * @return this page's sequence number
    */

   public int getPageSequenceNumber() {
      return pageSequenceNumber;
   }

   /**
    * Return the check sum of this ogg page.
    *
    * @return this page's check sum
    */

   public int getPageCheckSum() {
      return pageCheckSum;
   }

   /**
    * @return the total number of bytes in the page data
    */


   public int getTotalLength() {
      if(data!=null) {
         return 27+segmentTable.length+data.length;
      }
      else {
         return totalLength;
      }
   }

   /**
    * @return a ByteBuffer containing the page data
    */


   public byte[] getData() {
      return data;
   }

   public byte[] getHeader() {
      return header;
   }

   public byte[] getSegmentTable() {
      return segmentTable;
   }

   public int[] getSegmentOffsets() {
      return segmentOffsets;
   }

   public int[] getSegmentLengths() {
      return segmentLengths;
   }

   /**
    * @return <code>true</code> if this page begins with a continued packet
    */

   public boolean isContinued() {
      return continued;
   }

   /**
    * @return <code>true</code> if this page begins with a fresh packet
    */

   public boolean isFresh() {
      return !continued;
   }

   /**
    * @return <code>true</code> if this page is the beginning of a logical stream
    */

   public boolean isBos() {
      return bos;
   }

   /**
    * @return <code>true</code> if this page is the end of a logical stream
    */

   public boolean isEos() {
      return eos;
   }

}