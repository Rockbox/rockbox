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
 */

package de.jarnbjo.ogg;

import java.io.*;
import java.net.*;
import java.util.*;

/**
 *  Implementation of the <code>PhysicalOggStream</code> interface for reading
 *  an Ogg stream from a URL. This class performs only the necessary caching
 *  to provide continous playback. Seeking within the stream is not supported.
 */

public class UncachedUrlStream implements PhysicalOggStream {

   private boolean closed=false;
   private URLConnection source;
   private InputStream sourceStream;
   private Object drainLock=new Object();
   private LinkedList pageCache=new LinkedList();
   private long numberOfSamples=-1;

   private HashMap logicalStreams=new HashMap();

   private LoaderThread loaderThread;

   private static final int PAGECACHE_SIZE = 10;

	/** Creates an instance of the <code>PhysicalOggStream</code> interface
	 *  suitable for reading an Ogg stream from a URL. 
	 */

   public UncachedUrlStream(URL source) throws OggFormatException, IOException {

      this.source=source.openConnection();
      this.sourceStream=this.source.getInputStream();

      loaderThread=new LoaderThread(sourceStream, pageCache);
      new Thread(loaderThread).start();

      while(!loaderThread.isBosDone() || pageCache.size()<PAGECACHE_SIZE) {
         try {
            Thread.sleep(200);
         }
         catch (InterruptedException ex) {
         }
         //System.out.print("caching "+pageCache.size()+"/"+PAGECACHE_SIZE+" pages\r");
      }
      //System.out.println();
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

   /*
   public long getCacheLength() {
      return cacheLength;
   }
   */

   /*
   private OggPage getNextPage() throws EndOfOggStreamException, IOException, OggFormatException  {
      return getNextPage(false);
   }

   private OggPage getNextPage(boolean skipData) throws EndOfOggStreamException, IOException, OggFormatException  {
      return OggPage.create(sourceStream, skipData);
   }
   */

   public OggPage getOggPage(int index) throws IOException {
      while(pageCache.size()==0) {
         try {
            Thread.sleep(100);
         }
         catch (InterruptedException ex) {
         }
      }
      synchronized(drainLock) {
         //OggPage page=(OggPage)pageCache.getFirst();
         //pageCache.removeFirst();
         //return page;
         return (OggPage)pageCache.removeFirst();
      }
   }

   private LogicalOggStream getLogicalStream(int serialNumber) {
      return (LogicalOggStream)logicalStreams.get(new Integer(serialNumber));
   }

   public void setTime(long granulePosition) throws IOException {
      throw new UnsupportedOperationException("Method not supported by this class");
   }

   public class LoaderThread implements Runnable {

      private InputStream source;
      private LinkedList pageCache;
      private RandomAccessFile drain;
      private byte[] memoryCache;

      private boolean bosDone=false;

      private int pageNumber;

      public LoaderThread(InputStream source, LinkedList pageCache) {
         this.source=source;
         this.pageCache=pageCache;
      }

      public void run() {
         try {
            boolean eos=false;
            byte[] buffer=new byte[8192];
            while(!eos) {
               OggPage op=OggPage.create(source);
               synchronized (drainLock) {
                  pageCache.add(op);
               }

               if(!op.isBos()) {
                  bosDone=true;
               }
               if(op.isEos()) {
                  eos=true;
               }

               LogicalOggStreamImpl los=(LogicalOggStreamImpl)getLogicalStream(op.getStreamSerialNumber());
               if(los==null) {
                  los=new LogicalOggStreamImpl(UncachedUrlStream.this, op.getStreamSerialNumber());
                  logicalStreams.put(new Integer(op.getStreamSerialNumber()), los);
                  los.checkFormat(op);
               }

               //los.addPageNumberMapping(pageNumber);
               //los.addGranulePosition(op.getAbsoluteGranulePosition());

               pageNumber++;

               while(pageCache.size()>PAGECACHE_SIZE) {
                  try {
                     Thread.sleep(200);
                  }
                  catch (InterruptedException ex) {
                  }
               }
            }
         }
         catch(EndOfOggStreamException e) {
            // ok
         }
         catch(IOException e) {
            e.printStackTrace();
         }
      }

      public boolean isBosDone() {
         return bosDone;
      }
   }

	/** 
	 *  @return always <code>false</code>
	 */

   public boolean isSeekable() {
      return false;
   }

}