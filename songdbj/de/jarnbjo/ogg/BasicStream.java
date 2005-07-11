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
 * Revision 1.3  2004/09/21 12:09:45  shred
 * *** empty log message ***
 *
 * Revision 1.2  2004/09/21 06:38:45  shred
 * Importe reorganisiert, damit Eclipse Ruhe gibt. ;-)
 *
 * Revision 1.1.1.1  2004/04/04 22:09:12  shred
 * First Import
 *
 *
 */

package de.jarnbjo.ogg;

import java.io.IOException;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;

/**
 * Implementation of the <code>PhysicalOggStream</code> interface for reading
 * an Ogg stream from a URL. This class performs
 *  no internal caching, and will not read data from the network before
 *  requested to do so. It is intended to be used in non-realtime applications
 *  like file download managers or similar.
 */

public class BasicStream implements PhysicalOggStream {

   private boolean closed=false;
   private InputStream sourceStream;
   private Object drainLock=new Object();
   private LinkedList pageCache=new LinkedList();
   private long numberOfSamples=-1;
   private int position=0;

   private HashMap logicalStreams=new HashMap();
   private OggPage firstPage;

   public BasicStream(InputStream sourceStream) throws OggFormatException, IOException {
      firstPage=OggPage.create(sourceStream);
      position+=firstPage.getTotalLength();
      LogicalOggStreamImpl los=new LogicalOggStreamImpl(this, firstPage.getStreamSerialNumber());
      logicalStreams.put(new Integer(firstPage.getStreamSerialNumber()), los);
      los.checkFormat(firstPage);
   }

   public Collection getLogicalStreams() {
      return logicalStreams.values();
   }

   public boolean isOpen() {
      return !closed;
   }

   public void close() throws IOException {
      closed=true;
      sourceStream.close();
   }

   public int getContentLength() {
      return -1;
   }

   public int getPosition() {
      return position;
   }

   int pageNumber=2;

   public OggPage getOggPage(int index) throws IOException {
      if(firstPage!=null) {
         OggPage tmp=firstPage;
         firstPage=null;
         return tmp;
      }
      else {
         OggPage page=OggPage.create(sourceStream);
         position+=page.getTotalLength();
         return page;
      }
   }

   private LogicalOggStream getLogicalStream(int serialNumber) {
      return (LogicalOggStream)logicalStreams.get(new Integer(serialNumber));
   }

   public void setTime(long granulePosition) throws IOException {
      throw new UnsupportedOperationException("Method not supported by this class");
   }

  /** 
   *  @return always <code>false</code>
   */

   public boolean isSeekable() {
      return false;
   }

}