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
 * Revision 1.1  2003/04/10 19:48:22  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.ogg;

import java.io.*;
import java.util.*;

/**
 * Implementation of the <code>PhysicalOggStream</code> interface for accessing
 * normal disk files.
 */

public class FileStream implements PhysicalOggStream {

   private boolean closed=false;
   private RandomAccessFile source;
   private long[] pageOffsets;
   private long numberOfSamples=-1;

   private HashMap logicalStreams=new HashMap();

   /**
    * Creates access to the specified file through the <code>PhysicalOggStream</code> interface.
    * The specified source file must have been opened for reading.
    *
    * @param source the file to read from
    *
    * @throws OggFormatException if the stream format is incorrect
    * @throws IOException if some other IO error occurs when reading the file
    */

   public FileStream(RandomAccessFile source) throws OggFormatException, IOException {
      this.source=source;

      ArrayList po=new ArrayList();
      int pageNumber=0;
      try {
         while(true) {
            po.add(new Long(this.source.getFilePointer()));

            // skip data if pageNumber>0
            OggPage op=getNextPage(pageNumber>0);
            if(op==null) {
               break;
            }

            LogicalOggStreamImpl los=(LogicalOggStreamImpl)getLogicalStream(op.getStreamSerialNumber());
            if(los==null) {
               los=new LogicalOggStreamImpl(this, op.getStreamSerialNumber());
               logicalStreams.put(new Integer(op.getStreamSerialNumber()), los);
            }

            if(pageNumber==0) {
               los.checkFormat(op);
            }

            los.addPageNumberMapping(pageNumber);
            los.addGranulePosition(op.getAbsoluteGranulePosition());

            if(pageNumber>0) {
               this.source.seek(this.source.getFilePointer()+op.getTotalLength());
            }

            pageNumber++;
         }
      }
      catch(EndOfOggStreamException e) {
         // ok
      }
      catch(IOException e) {
         throw e;
      }
      //System.out.println("pageNumber: "+pageNumber);
      this.source.seek(0L);
      pageOffsets=new long[po.size()];
      int i=0;
      Iterator iter=po.iterator();
      while(iter.hasNext()) {
         pageOffsets[i++]=((Long)iter.next()).longValue();
      }
   }

   public Collection getLogicalStreams() {
      return logicalStreams.values();
   }

   public boolean isOpen() {
      return !closed;
   }

   public void close() throws IOException {
      closed=true;
      source.close();
   }

   private OggPage getNextPage() throws EndOfOggStreamException, IOException, OggFormatException  {
      return getNextPage(false);
   }

   private OggPage getNextPage(boolean skipData) throws EndOfOggStreamException, IOException, OggFormatException  {
      return OggPage.create(source, skipData);
   }

   public OggPage getOggPage(int index) throws IOException {
      source.seek(pageOffsets[index]);
      return OggPage.create(source);
   }

   private LogicalOggStream getLogicalStream(int serialNumber) {
      return (LogicalOggStream)logicalStreams.get(new Integer(serialNumber));
   }

   public void setTime(long granulePosition) throws IOException {
      for(Iterator iter=logicalStreams.values().iterator(); iter.hasNext(); ) {
         LogicalOggStream los=(LogicalOggStream)iter.next();
         los.setTime(granulePosition);
      }
   }

	/**
	 *  @return always <code>true</code>
	 */

   public boolean isSeekable() {
      return true;
   }
}