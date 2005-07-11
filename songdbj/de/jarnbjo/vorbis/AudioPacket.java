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
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.vorbis;

import java.io.IOException;

import de.jarnbjo.util.io.BitInputStream;

class AudioPacket {

   private int modeNumber;
   private Mode mode;
   private Mapping mapping;
   private int n; // block size
   private boolean blockFlag, previousWindowFlag, nextWindowFlag;

   private int windowCenter, leftWindowStart, leftWindowEnd, leftN, rightWindowStart, rightWindowEnd, rightN;
   private float[] window;
   private float[][] pcm;
   private int[][] pcmInt;

   private Floor[] channelFloors;
   private boolean[] noResidues;

   private final static float[][] windows=new float[8][];

   protected AudioPacket(final VorbisStream vorbis, final BitInputStream source) throws VorbisFormatException, IOException {

      final SetupHeader sHeader=vorbis.getSetupHeader();
      final IdentificationHeader iHeader=vorbis.getIdentificationHeader();
      final Mode[] modes=sHeader.getModes();
      final Mapping[] mappings=sHeader.getMappings();
      final Residue[] residues=sHeader.getResidues();
      final int channels=iHeader.getChannels();

      if(source.getInt(1)!=0) {
         throw new VorbisFormatException("Packet type mismatch when trying to create an audio packet.");
      }

      modeNumber=source.getInt(Util.ilog(modes.length-1));

      try {
         mode=modes[modeNumber];
      }
      catch(ArrayIndexOutOfBoundsException e) {
         throw new VorbisFormatException("Reference to invalid mode in audio packet.");
      }

      mapping=mappings[mode.getMapping()];

      final int[] magnitudes=mapping.getMagnitudes();
      final int[] angles=mapping.getAngles();

      blockFlag=mode.getBlockFlag();

      final int blockSize0=iHeader.getBlockSize0();
      final int blockSize1=iHeader.getBlockSize1();

      n=blockFlag?blockSize1:blockSize0;

      if(blockFlag) {
         previousWindowFlag=source.getBit();
         nextWindowFlag=source.getBit();
      }

      windowCenter=n/2;

      if(blockFlag && !previousWindowFlag) {
         leftWindowStart=n/4-blockSize0/4;
         leftWindowEnd=n/4+blockSize0/4;
         leftN=blockSize0/2;
      }
      else {
         leftWindowStart=0;
         leftWindowEnd=n/2;
         leftN=windowCenter;
      }

      if(blockFlag && !nextWindowFlag) {
         rightWindowStart=n*3/4-blockSize0/4;
         rightWindowEnd=n*3/4+blockSize0/4;
         rightN=blockSize0/2;
      }
      else {
         rightWindowStart=windowCenter;
         rightWindowEnd=n;
         rightN=n/2;
      }

      window=getComputedWindow();//new double[n];

      channelFloors=new Floor[channels];
      noResidues=new boolean[channels];

      pcm=new float[channels][n];
      pcmInt=new int[channels][n];

      boolean allFloorsEmpty=true;

      for(int i=0; i<channels; i++) {
         int submapNumber=mapping.getMux()[i];
         int floorNumber=mapping.getSubmapFloors()[submapNumber];
         Floor decodedFloor=sHeader.getFloors()[floorNumber].decodeFloor(vorbis, source);
         channelFloors[i]=decodedFloor;
         noResidues[i]=decodedFloor==null;
         if(decodedFloor!=null) {
            allFloorsEmpty=false;
         }
      }

      if(allFloorsEmpty) {
         return;
      }

      for(int i=0; i<magnitudes.length; i++) {
         if(!noResidues[magnitudes[i]] ||
            !noResidues[angles[i]]) {

            noResidues[magnitudes[i]]=false;
            noResidues[angles[i]]=false;
         }
      }

      Residue[] decodedResidues=new Residue[mapping.getSubmaps()];

      for(int i=0; i<mapping.getSubmaps(); i++) {
         int ch=0;
         boolean[] doNotDecodeFlags=new boolean[channels];
         for(int j=0; j<channels; j++) {
            if(mapping.getMux()[j]==i) {
               doNotDecodeFlags[ch++]=noResidues[j];
            }
         }
         int residueNumber=mapping.getSubmapResidues()[i];
         Residue residue=residues[residueNumber];

         residue.decodeResidue(vorbis, source, mode, ch, doNotDecodeFlags, pcm);
      }


      for(int i=mapping.getCouplingSteps()-1; i>=0; i--) {
         double newA=0, newM=0;
         final float[] magnitudeVector=pcm[magnitudes[i]];
         final float[] angleVector=pcm[angles[i]];
         for(int j=0; j<magnitudeVector.length; j++) {
            float a=angleVector[j];
            float m=magnitudeVector[j];
            if(a>0) {
               //magnitudeVector[j]=m;
               angleVector[j]=m>0?m-a:m+a;
            }
            else {
               magnitudeVector[j]=m>0?m+a:m-a;
               angleVector[j]=m;
            }
         }
      }

      for(int i=0; i<channels; i++) {
         if(channelFloors[i]!=null) {
            channelFloors[i].computeFloor(pcm[i]);
         }
      }

      // perform an inverse mdct to all channels

      for(int i=0; i<channels; i++) {
         MdctFloat mdct=blockFlag?iHeader.getMdct1():iHeader.getMdct0();
         mdct.imdct(pcm[i], window, pcmInt[i]);
      }

   }

   private float[] getComputedWindow() {
      int ix=(blockFlag?4:0)+(previousWindowFlag?2:0)+(nextWindowFlag?1:0);
      float[] w=windows[ix];
      if(w==null) {
         w=new float[n];

         for(int i=0;i<leftN;i++){
	         float x=(float)((i+.5)/leftN*Math.PI/2.);
	         x=(float)Math.sin(x);
	         x*=x;
	         x*=(float)Math.PI/2.;
	         x=(float)Math.sin(x);
	         w[i+leftWindowStart]=x;
	      }

         for(int i=leftWindowEnd; i<rightWindowStart; w[i++]=1.0f);

	      for(int i=0;i<rightN;i++){
	         float x=(float)((rightN-i-.5)/rightN*Math.PI/2.);
	         x=(float)Math.sin(x);
	         x*=x;
	         x*=(float)Math.PI/2.;
	         x=(float)Math.sin(x);
	         w[i+rightWindowStart]=x;
	      }

         windows[ix]=w;
      }
      return w;
   }

   protected int getNumberOfSamples() {
      return rightWindowStart-leftWindowStart;
   }

   protected int getPcm(final AudioPacket previousPacket, final int[][] buffer) {
      int channels=pcm.length;
      int val;

      // copy left window flank and mix with right window flank from
      // the previous audio packet
      for(int i=0; i<channels; i++) {
         int j1=0, j2=previousPacket.rightWindowStart;
         final int[] ppcm=previousPacket.pcmInt[i];
         final int[] tpcm=pcmInt[i];
         final int[] target=buffer[i];

         for(int j=leftWindowStart; j<leftWindowEnd; j++) {
            val=ppcm[j2++]+tpcm[j];
            if(val>32767) val=32767;
            if(val<-32768) val=-32768;
            target[j1++]=val;
         }
      }

      // use System.arraycopy to copy the middle part (if any)
      // of the window
      if(leftWindowEnd+1<rightWindowStart) {
         for(int i=0; i<channels; i++) {
            System.arraycopy(pcmInt[i], leftWindowEnd, buffer[i], leftWindowEnd-leftWindowStart, rightWindowStart-leftWindowEnd);
         }
      }

      return rightWindowStart-leftWindowStart;
   }

   protected void getPcm(final AudioPacket previousPacket, final byte[] buffer) {
      int channels=pcm.length;
      int val;

      // copy left window flank and mix with right window flank from
      // the previous audio packet
      for(int i=0; i<channels; i++) {
         int ix=0, j2=previousPacket.rightWindowStart;
         final int[] ppcm=previousPacket.pcmInt[i];
         final int[] tpcm=pcmInt[i];
         for(int j=leftWindowStart; j<leftWindowEnd; j++) {
            val=ppcm[j2++]+tpcm[j];
            if(val>32767) val=32767;
            if(val<-32768) val=-32768;
            buffer[ix+(i*2)+1]=(byte)(val&0xff);
            buffer[ix+(i*2)]=(byte)((val>>8)&0xff);
            ix+=channels*2;
         }

         ix=(leftWindowEnd-leftWindowStart)*channels*2;
         for(int j=leftWindowEnd; j<rightWindowStart; j++) {
            val=tpcm[j];
            if(val>32767) val=32767;
            if(val<-32768) val=-32768;
            buffer[ix+(i*2)+1]=(byte)(val&0xff);
            buffer[ix+(i*2)]=(byte)((val>>8)&0xff);
            ix+=channels*2;
         }
      }
   }

   protected float[] getWindow() {
      return window;
   }

   protected int getLeftWindowStart() {
      return leftWindowStart;
   }

   protected int getLeftWindowEnd() {
      return leftWindowEnd;
   }

   protected int getRightWindowStart() {
      return rightWindowStart;
   }

   protected int getRightWindowEnd() {
      return rightWindowEnd;
   }

   public int[][] getPcm() {
      return pcmInt;
   }

   public float[][] getFreqencyDomain() {
      return pcm;
   }
}
