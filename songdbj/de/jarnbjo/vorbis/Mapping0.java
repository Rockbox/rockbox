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

class Mapping0 extends Mapping {

   private int[] magnitudes, angles, mux, submapFloors, submapResidues;

   protected Mapping0(VorbisStream vorbis, BitInputStream source, SetupHeader header) throws VorbisFormatException, IOException {

      int submaps=1;

      if(source.getBit()) {
         submaps=source.getInt(4)+1;
      }

      //System.out.println("submaps: "+submaps);

      int channels=vorbis.getIdentificationHeader().getChannels();
      int ilogChannels=Util.ilog(channels-1);

      //System.out.println("ilogChannels: "+ilogChannels);

      if(source.getBit()) {
         int couplingSteps=source.getInt(8)+1;
         magnitudes=new int[couplingSteps];
         angles=new int[couplingSteps];

         for(int i=0; i<couplingSteps; i++) {
            magnitudes[i]=source.getInt(ilogChannels);
            angles[i]=source.getInt(ilogChannels);
            if(magnitudes[i]==angles[i] || magnitudes[i]>=channels || angles[i]>=channels) {
               System.err.println(magnitudes[i]);
               System.err.println(angles[i]);
               throw new VorbisFormatException("The channel magnitude and/or angle mismatch.");
            }
         }
      }
      else {
         magnitudes=new int[0];
         angles=new int[0];
      }

      if(source.getInt(2)!=0) {
         throw new VorbisFormatException("A reserved mapping field has an invalid value.");
      }

      mux=new int[channels];
      if(submaps>1) {
         for(int i=0; i<channels; i++) {
            mux[i]=source.getInt(4);
            if(mux[i]>submaps) {
               throw new VorbisFormatException("A mapping mux value is higher than the number of submaps");
            }
         }
      }
      else {
         for(int i=0; i<channels; i++) {
            mux[i]=0;
         }
      }

      submapFloors=new int[submaps];
      submapResidues=new int[submaps];

      int floorCount=header.getFloors().length;
      int residueCount=header.getResidues().length;

      for(int i=0; i<submaps; i++) {
         source.getInt(8); // discard time placeholder
         submapFloors[i]=source.getInt(8);
         submapResidues[i]=source.getInt(8);

         if(submapFloors[i]>floorCount) {
            throw new VorbisFormatException("A mapping floor value is higher than the number of floors.");
         }

         if(submapResidues[i]>residueCount) {
            throw new VorbisFormatException("A mapping residue value is higher than the number of residues.");
         }
      }
   }

   protected int getType() {
      return 0;
   }

   protected int[] getAngles() {
      return angles;
   }

   protected int[] getMagnitudes() {
      return magnitudes;
   }

   protected int[] getMux() {
      return mux;
   }

   protected int[] getSubmapFloors() {
      return submapFloors;
   }

   protected int[] getSubmapResidues() {
      return submapResidues;
   }

   protected int getCouplingSteps() {
      return angles.length;
   }

   protected int getSubmaps() {
      return submapFloors.length;
   }
}