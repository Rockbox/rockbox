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
 * Revision 1.3  2003/04/04 08:33:02  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */

package de.jarnbjo.vorbis;

import java.io.IOException;
import java.util.HashMap;

import de.jarnbjo.util.io.*;


abstract class Residue {

   protected int begin, end;
   protected int partitionSize; // grouping
   protected int classifications; // partitions
   protected int classBook; // groupbook
   protected int[] cascade; // secondstages
   protected int[][] books;
   protected HashMap looks=new HashMap();

   protected Residue() {
   }

   protected Residue(BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {
      begin=source.getInt(24);
      end=source.getInt(24);
      partitionSize=source.getInt(24)+1;
      classifications=source.getInt(6)+1;
      classBook=source.getInt(8);

      cascade=new int[classifications];

      int acc=0;

      for(int i=0; i<classifications; i++) {
         int highBits=0, lowBits=0;
         lowBits=source.getInt(3);
         if(source.getBit()) {
            highBits=source.getInt(5);
         }
         cascade[i]=(highBits<<3)|lowBits;
         acc+=Util.icount(cascade[i]);
      }

      books=new int[classifications][8];

      for(int i=0; i<classifications; i++) {
         for(int j=0; j<8; j++) {
            if((cascade[i]&(1<<j))!=0) {
               books[i][j]=source.getInt(8);
               if(books[i][j]>header.getCodeBooks().length) {
                  throw new VorbisFormatException("Reference to invalid codebook entry in residue header.");
               }
            }
         }
      }
   }


   protected static Residue createInstance(BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {

      int type=source.getInt(16);
      switch(type) {
      case 0:
         //System.out.println("residue type 0");
         return new Residue0(source, header);
      case 1:
         //System.out.println("residue type 1");
         return new Residue2(source, header);
      case 2:
         //System.out.println("residue type 2");
         return new Residue2(source, header);
      default:
         throw new VorbisFormatException("Residue type "+type+" is not supported.");
      }
   }

   protected abstract int getType();
   protected abstract void decodeResidue(VorbisStream vorbis, BitInputStream source, Mode mode, int ch, boolean[] doNotDecodeFlags, float[][] vectors) throws VorbisFormatException, IOException;
   //public abstract double[][] getDecodedVectors();

   protected int getBegin() {
      return begin;
   }

   protected int getEnd() {
      return end;
   }

   protected int getPartitionSize() {
      return partitionSize;
   }

   protected int getClassifications() {
      return classifications;
   }

   protected int getClassBook() {
      return classBook;
   }

   protected int[] getCascade() {
      return cascade;
   }

   protected int[][] getBooks() {
      return books;
   }

   protected final void fill(Residue clone) {
      clone.begin=begin;
      clone.books=books;
      clone.cascade=cascade;
      clone.classBook=classBook;
      clone.classifications=classifications;
      clone.end=end;
      clone.partitionSize=partitionSize;
   }

   protected Look getLook(VorbisStream source, Mode key) {
      //return new Look(source, key);
      Look look=(Look)looks.get(key);
      if(look==null) {
         look=new Look(source, key);
         looks.put(key, look);
      }
      return look;
   }


   class Look  {
      int map;
      int parts;
      int stages;
      CodeBook[] fullbooks;
      CodeBook phrasebook;
      int[][] partbooks;
      int partvals;
      int[][] decodemap;
      int postbits;
      int phrasebits;
      int frames;

      protected Look (VorbisStream source, Mode mode) {
         int dim=0, acc=0, maxstage=0;

         map=mode.getMapping();
         parts=Residue.this.getClassifications();
         fullbooks=source.getSetupHeader().getCodeBooks();
         phrasebook=fullbooks[Residue.this.getClassBook()];
         dim=phrasebook.getDimensions();

         partbooks=new int[parts][];

         for(int j=0;j<parts;j++) {
            int stages=Util.ilog(Residue.this.getCascade()[j]);
            if(stages!=0) {
               if(stages>maxstage) {
                  maxstage=stages;
               }
               partbooks[j]=new int[stages];
               for(int k=0; k<stages; k++){
                  if((Residue.this.getCascade()[j]&(1<<k))!=0){
                     partbooks[j][k]=Residue.this.getBooks()[j][k];
                  }
	            }
            }
         }

         partvals=(int)Math.rint(Math.pow(parts, dim));
         stages=maxstage;

         decodemap=new int[partvals][];

         for(int j=0;j<partvals;j++){
            int val=j;
            int mult=partvals/parts;
            decodemap[j]=new int[dim];

            for(int k=0;k<dim;k++){
               int deco=val/mult;
               val-=deco*mult;
               mult/=parts;
               decodemap[j][k]=deco;
            }
         }
      }

      protected int[][] getDecodeMap() {
         return decodemap;
      }

      protected int getFrames() {
         return frames;
      }

      protected int getMap() {
         return map;
      }

      protected int[][] getPartBooks() {
         return partbooks;
      }

      protected int getParts() {
         return parts;
      }

      protected int getPartVals() {
         return partvals;
      }

      protected int getPhraseBits() {
         return phrasebits;
      }

      protected CodeBook getPhraseBook() {
         return phrasebook;
      }

      protected int getPostBits() {
         return postbits;
      }

      protected int getStages() {
         return stages;
      }
   }

}