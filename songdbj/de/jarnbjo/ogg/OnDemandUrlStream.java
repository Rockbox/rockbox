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
 * Revision 1.1  2003/03/31 00:23:04  jarnbjo
 * no message
 *
 */

package de.jarnbjo.ogg;

import java.io.*;
import java.net.*;
import java.util.*;

/**
 * Implementation of the <code>PhysicalOggStream</code> interface for reading
 * an Ogg stream from a URL. This class performs
 *  no internal caching, and will not read data from the network before
 *  requested to do so. It is intended to be used in non-realtime applications
 *  like file download managers or similar.
 */

public class OnDemandUrlStream implements PhysicalOggStream {

   private boolean closed=false;
   private URLConnection source;
   private InputStream sourceStream;
   private Object drainLock=new Object();
   private LinkedList pageCache=new LinkedList();
   private long numberOfSamples=-1;
   private int contentLength=0;
   private int position=0;

   private HashMap logicalStreams=new HashMap();
   private OggPage firstPage;

   private static final int PAGECACHE_SIZE = 20;

   public OnDemandUrlStream(URL source) throws OggFormatException, IOException {
      this.source=source.openConnection();
      this.sourceStream=this.source.getInputStream();

      contentLength=this.source.getContentLength();

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
      return contentLength;
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