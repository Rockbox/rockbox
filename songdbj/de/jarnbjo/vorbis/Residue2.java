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

class Residue2 extends Residue {

   private double[][] decodedVectors;

   private Residue2() {
   }

   protected Residue2(BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {
      super(source, header);
   }

   protected int getType() {
      return 2;
   }

   protected void decodeResidue(VorbisStream vorbis, BitInputStream source, Mode mode, int ch, boolean[] doNotDecodeFlags, float[][] vectors) throws VorbisFormatException, IOException {

      Look look=getLook(vorbis, mode);

      CodeBook codeBook=vorbis.getSetupHeader().getCodeBooks()[getClassBook()];

      int classvalsPerCodeword=codeBook.getDimensions();
      int nToRead=getEnd()-getBegin();
      int partitionsToRead=nToRead/getPartitionSize(); // partvals

      int samplesPerPartition=getPartitionSize();
      int partitionsPerWord=look.getPhraseBook().getDimensions();

      int partWords=(partitionsToRead+partitionsPerWord-1)/partitionsPerWord;

      int realCh=0;
      for(int i=0; i<doNotDecodeFlags.length; i++) {
         if(!doNotDecodeFlags[i]) {
            realCh++;
         }
      }

      float[][] realVectors=new float[realCh][];

      realCh=0;
      for(int i=0; i<doNotDecodeFlags.length; i++) {
         if(!doNotDecodeFlags[i]) {
            realVectors[realCh++]=vectors[i];
         }
      }

      int[][] partword=new int[partWords][];
      for(int s=0;s<look.getStages();s++){
         for(int i=0,l=0;i<partitionsToRead;l++){
            if(s==0){
	            //int temp=look.getPhraseBook().readInt(source);
	            int temp=source.getInt(look.getPhraseBook().getHuffmanRoot());
	            if(temp==-1){
	               throw new VorbisFormatException("");
	            }
	            partword[l]=look.getDecodeMap()[temp];
	            if(partword[l]==null){
	               throw new VorbisFormatException("");
	            }
	         }

            for(int k=0;k<partitionsPerWord && i<partitionsToRead;k++,i++){
               int offset=begin+i*samplesPerPartition;
	            if((cascade[partword[l][k]]&(1<<s))!=0){
                  CodeBook stagebook=vorbis.getSetupHeader().getCodeBooks()[look.getPartBooks()[partword[l][k]][s]];
	               if(stagebook!=null){
                     stagebook.readVvAdd(realVectors, source, offset, samplesPerPartition);
                  }
	            }
	         }
         }
      }
   }


   public Object clone() {
      Residue2 clone=new Residue2();
      fill(clone);
      return clone;
   }

   protected double[][] getDecodedVectors() {
      return decodedVectors;
   }
}