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
 * Revision 1.2  2003/03/16 01:11:12  jarnbjo
 * no message
 *
 *
 */
 
package de.jarnbjo.vorbis;

import java.io.IOException;

import de.jarnbjo.util.io.BitInputStream;

class Floor0 extends Floor {

   private int order, rate, barkMapSize, amplitudeBits, amplitudeOffset;
   private int bookList[];

   protected Floor0(BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {

      order=source.getInt(8);
      rate=source.getInt(16);
      barkMapSize=source.getInt(16);
      amplitudeBits=source.getInt(6);
      amplitudeOffset=source.getInt(8);

      int bookCount=source.getInt(4)+1;
      bookList=new int[bookCount];

      for(int i=0; i<bookList.length; i++) {
         bookList[i]=source.getInt(8);
         if(bookList[i]>header.getCodeBooks().length) {
            throw new VorbisFormatException("A floor0_book_list entry is higher than the code book count.");
         }
      }
   }

   protected int getType() {
      return 0;
   }

   protected Floor decodeFloor(VorbisStream vorbis, BitInputStream source) throws VorbisFormatException, IOException {
      /** @todo implement */
      throw new UnsupportedOperationException();
   }

   protected void computeFloor(float[] vector) {
      /** @todo implement */
      throw new UnsupportedOperationException();
   }

}