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
 * Revision 1.3  2003/03/31 00:23:04  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/16 01:11:26  jarnbjo
 * no message
 *
 * Revision 1.1  2003/03/03 21:02:20  jarnbjo
 * no message
 *
 */

package de.jarnbjo.ogg;

import java.io.*;
import java.util.*;

public class LogicalOggStreamImpl implements LogicalOggStream {

   private PhysicalOggStream source;
   private int serialNumber;

   private ArrayList pageNumberMapping=new ArrayList();
   private ArrayList granulePositions=new ArrayList();

   private int pageIndex=0;
   private OggPage currentPage;
   private int currentSegmentIndex;

   private boolean open=true;

   private String format=FORMAT_UNKNOWN;

   public LogicalOggStreamImpl(PhysicalOggStream source, int serialNumber) {
      this.source=source;
      this.serialNumber=serialNumber;
   }

   public void addPageNumberMapping(int physicalPageNumber) {
      pageNumberMapping.add(new Integer(physicalPageNumber));
   }

   public void addGranulePosition(long granulePosition) {
      granulePositions.add(new Long(granulePosition));
   }

   public synchronized void reset() throws OggFormatException, IOException {
      currentPage=null;
      currentSegmentIndex=0;
      pageIndex=0;
   }

   public synchronized OggPage getNextOggPage() throws EndOfOggStreamException, OggFormatException, IOException {
      if(source.isSeekable()) {
         currentPage=source.getOggPage(((Integer)pageNumberMapping.get(pageIndex++)).intValue());
      }
      else {
         currentPage=source.getOggPage(-1);
      }
      return currentPage;
   }

   public synchronized byte[] getNextOggPacket() throws EndOfOggStreamException, OggFormatException, IOException {
      ByteArrayOutputStream res=new ByteArrayOutputStream();
      int segmentLength=0;

      if(currentPage==null) {
         currentPage=getNextOggPage();
      }

      do {
         if(currentSegmentIndex>=currentPage.getSegmentOffsets().length) {
            currentSegmentIndex=0;

            if(!currentPage.isEos()) {
               if(source.isSeekable() && pageNumberMapping.size()<=pageIndex) {
                  while(pageNumberMapping.size()<=pageIndex+10) {
                     try {
                        Thread.sleep(1000);
                     }
                     catch (InterruptedException ex) {
                     }
                  }
               }
               currentPage=getNextOggPage();

               if(res.size()==0 && currentPage.isContinued()) {
                  boolean done=false;
                  while(!done) {
                     if(currentPage.getSegmentLengths()[currentSegmentIndex++]!=255) {
                        done=true;
                     }
                     if(currentSegmentIndex>currentPage.getSegmentTable().length) {
                        currentPage=source.getOggPage(((Integer)pageNumberMapping.get(pageIndex++)).intValue());
                     }
                  }
               }
            }
            else {
               throw new EndOfOggStreamException();
            }
         }
         segmentLength=currentPage.getSegmentLengths()[currentSegmentIndex];
         res.write(currentPage.getData(), currentPage.getSegmentOffsets()[currentSegmentIndex], segmentLength);
         currentSegmentIndex++;
      } while(segmentLength==255);

      return res.toByteArray();
   }

   public boolean isOpen() {
      return open;
   }

   public void close() throws IOException {
      open=false;
   }

   public long getMaximumGranulePosition() {
      Long mgp=(Long)granulePositions.get(granulePositions.size()-1);
      return mgp.longValue();
   }

   public synchronized long getTime() {
      return currentPage!=null?currentPage.getAbsoluteGranulePosition():-1;
   }

   public synchronized void setTime(long granulePosition) throws IOException {

      int page=0;
      for(page=0; page<granulePositions.size(); page++) {
         Long gp=(Long)granulePositions.get(page);
         if(gp.longValue()>granulePosition) {
            break;
         }
      }

      pageIndex=page;
      currentPage=source.getOggPage(((Integer)pageNumberMapping.get(pageIndex++)).intValue());
      currentSegmentIndex=0;
      int segmentLength=0;
      do {
         if(currentSegmentIndex>=currentPage.getSegmentOffsets().length) {
            currentSegmentIndex=0;
            if(pageIndex>=pageNumberMapping.size()) {
               throw new EndOfOggStreamException();
            }
            currentPage=source.getOggPage(((Integer)pageNumberMapping.get(pageIndex++)).intValue());
         }
         segmentLength=currentPage.getSegmentLengths()[currentSegmentIndex];
         currentSegmentIndex++;
      } while(segmentLength==255);
   }

   public void checkFormat(OggPage page) {
      byte[] data=page.getData();

      if(data.length>=7 &&
         data[1]==0x76 &&
         data[2]==0x6f &&
         data[3]==0x72 &&
         data[4]==0x62 &&
         data[5]==0x69 &&
         data[6]==0x73) {

         format=FORMAT_VORBIS;
      }
      else if(data.length>=7 &&
         data[1]==0x74 &&
         data[2]==0x68 &&
         data[3]==0x65 &&
         data[4]==0x6f &&
         data[5]==0x72 &&
         data[6]==0x61) {

         format=FORMAT_THEORA;
      }
      else if (data.length==4 &&
         data[0]==0x66 &&
         data[1]==0x4c &&
         data[2]==0x61 &&
         data[3]==0x43) {

         format=FORMAT_FLAC;
      }
   }

   public String getFormat() {
      return format;
   }
}