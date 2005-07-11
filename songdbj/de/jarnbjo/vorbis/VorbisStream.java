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
 * Revision 1.4  2003/04/10 19:49:04  jarnbjo
 * no message
 *
 * Revision 1.3  2003/03/31 00:20:16  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.vorbis;

import java.io.*;
import java.util.*;

import de.jarnbjo.ogg.*;
import de.jarnbjo.util.io.*;

/**
 */

public class VorbisStream {

   private LogicalOggStream oggStream;
   private IdentificationHeader identificationHeader;
   private CommentHeader commentHeader;
   private SetupHeader setupHeader;

   private AudioPacket lastAudioPacket, nextAudioPacket;
   private LinkedList audioPackets=new LinkedList();
   private byte[] currentPcm;
   private int currentPcmIndex;
   private int currentPcmLimit;

   private static final int IDENTIFICATION_HEADER = 1;
   private static final int COMMENT_HEADER = 3;
   private static final int SETUP_HEADER = 5;

   private int bitIndex=0;
   private byte lastByte=(byte)0;
   private boolean initialized=false;

   private Object streamLock=new Object();
   private int pageCounter=0;

   private int currentBitRate=0;

   private long currentGranulePosition;

   public static final int BIG_ENDIAN = 0;
   public static final int LITTLE_ENDIAN = 1;

   public VorbisStream() {
   }

   public VorbisStream(LogicalOggStream oggStream) throws VorbisFormatException, IOException {
      this.oggStream=oggStream;

      for(int i=0; i<3; i++) {
         BitInputStream source=new ByteArrayBitInputStream(oggStream.getNextOggPacket());
         int headerType=source.getInt(8);
         switch(headerType) {
         case IDENTIFICATION_HEADER:
            identificationHeader=new IdentificationHeader(source);
            break;
         case COMMENT_HEADER:
            commentHeader=new CommentHeader(source);
            break;
         case SETUP_HEADER:
            setupHeader=new SetupHeader(this, source);
            break;
         }
      }

      if(identificationHeader==null) {
         throw new VorbisFormatException("The file has no identification header.");
      }

      if(commentHeader==null) {
         throw new VorbisFormatException("The file has no commentHeader.");
      }

      if(setupHeader==null) {
         throw new VorbisFormatException("The file has no setup header.");
      }

      //currentPcm=new int[identificationHeader.getChannels()][16384];
      currentPcm=new byte[identificationHeader.getChannels()*identificationHeader.getBlockSize1()*2];
      //new BufferThread().start();
   }

   public IdentificationHeader getIdentificationHeader() {
      return identificationHeader;
   }

   public CommentHeader getCommentHeader() {
      return commentHeader;
   }

   protected SetupHeader getSetupHeader() {
      return setupHeader;
   }

   public boolean isOpen() {
      return oggStream.isOpen();
   }

   public void close() throws IOException {
      oggStream.close();
   }


   public int readPcm(byte[] buffer, int offset, int length) throws IOException {
      synchronized (streamLock) {
         final int channels=identificationHeader.getChannels();

         if(lastAudioPacket==null) {
            lastAudioPacket=getNextAudioPacket();
         }
         if(currentPcm==null || currentPcmIndex>=currentPcmLimit) {
            AudioPacket ap=getNextAudioPacket();
            try {
               ap.getPcm(lastAudioPacket, currentPcm);
               currentPcmLimit=ap.getNumberOfSamples()*identificationHeader.getChannels()*2;
            }
            catch(ArrayIndexOutOfBoundsException e) {
               return 0;
            }
            currentPcmIndex=0;
            lastAudioPacket=ap;
         }
         int written=0;
         int i=0;
         int arrIx=0;
         for(i=currentPcmIndex; i<currentPcmLimit && arrIx<length; i++) {
            buffer[offset+arrIx++]=currentPcm[i];
            written++;
         }
         currentPcmIndex=i;
         return written;
      }
   }


   private AudioPacket getNextAudioPacket() throws VorbisFormatException, IOException {
      pageCounter++;
      byte[] data=oggStream.getNextOggPacket();
      AudioPacket res=null;
      while(res==null) {
         try {
            res=new AudioPacket(this, new ByteArrayBitInputStream(data));
         }
         catch(ArrayIndexOutOfBoundsException e) {
            // ignore and continue with next packet
         }
      }
      currentGranulePosition+=res.getNumberOfSamples();
      currentBitRate=data.length*8*identificationHeader.getSampleRate()/res.getNumberOfSamples();
      return res;
   }

   public long getCurrentGranulePosition() {
      return currentGranulePosition;
   }

   public int getCurrentBitRate() {
      return currentBitRate;
   }

   public byte[] processPacket(byte[] packet) throws VorbisFormatException, IOException {
      if(packet.length==0) {
         throw new VorbisFormatException("Cannot decode a vorbis packet with length = 0");
      }
      if(((int)packet[0]&1)==1) {
         // header packet
         BitInputStream source=new ByteArrayBitInputStream(packet);
         switch(source.getInt(8)) {
         case IDENTIFICATION_HEADER:
            identificationHeader=new IdentificationHeader(source);
            break;
         case COMMENT_HEADER:
            commentHeader=new CommentHeader(source);
            break;
         case SETUP_HEADER:
            setupHeader=new SetupHeader(this, source);
            break;
         }
         return null;
      }
      else {
         // audio packet
         if(identificationHeader==null ||
            commentHeader==null ||
            setupHeader==null) {

            throw new VorbisFormatException("Cannot decode audio packet before all three header packets have been decoded.");
         }

         AudioPacket ap=new AudioPacket(this, new ByteArrayBitInputStream(packet));
         currentGranulePosition+=ap.getNumberOfSamples();

         if(lastAudioPacket==null) {
            lastAudioPacket=ap;
            return null;
         }

         byte[] res=new byte[identificationHeader.getChannels()*ap.getNumberOfSamples()*2];

         try {
            ap.getPcm(lastAudioPacket, res);
         }
         catch(IndexOutOfBoundsException e) {
            java.util.Arrays.fill(res, (byte)0);
         }

         lastAudioPacket=ap;

         return res;
      }
   }

}