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

import java.io.*;

import de.jarnbjo.util.io.*;

class Mode {

   private boolean blockFlag;
   private int windowType, transformType, mapping;

   protected Mode(BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {
      blockFlag=source.getBit();
      windowType=source.getInt(16);
      transformType=source.getInt(16);
      mapping=source.getInt(8);

      if(windowType!=0) {
         throw new VorbisFormatException("Window type = "+windowType+", != 0");
      }

      if(transformType!=0) {
         throw new VorbisFormatException("Transform type = "+transformType+", != 0");
      }

      if(mapping>header.getMappings().length) {
         throw new VorbisFormatException("Mode mapping number is higher than total number of mappings.");
      }
   }

   protected boolean getBlockFlag() {
      return blockFlag;
   }

   protected int getWindowType() {
      return windowType;
   }

   protected int getTransformType() {
      return transformType;
   }

   protected int getMapping() {
      return mapping;
   }
}