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

class SetupHeader {

   private static final long HEADER = 0x736962726f76L; // 'vorbis'

   private CodeBook[] codeBooks;
   private Floor[] floors;
   private Residue[] residues;
   private Mapping[] mappings;
   private Mode[] modes;

   public SetupHeader(VorbisStream vorbis, BitInputStream source) throws VorbisFormatException, IOException {

      if(source.getLong(48)!=HEADER) {
         throw new VorbisFormatException("The setup header has an illegal leading.");
      }

      // read code books

      int codeBookCount=source.getInt(8)+1;
      codeBooks=new CodeBook[codeBookCount];

      for(int i=0; i<codeBooks.length; i++) {
         codeBooks[i]=new CodeBook(source);
      }

      // read the time domain transformations,
      // these should all be 0

      int timeCount=source.getInt(6)+1;
      for(int i=0; i<timeCount; i++) {
         if(source.getInt(16)!=0) {
            throw new VorbisFormatException("Time domain transformation != 0");
         }
      }

      // read floor entries

      int floorCount=source.getInt(6)+1;
      floors=new Floor[floorCount];

      for(int i=0; i<floorCount; i++) {
         floors[i]=Floor.createInstance(source, this);
      }

      // read residue entries

      int residueCount=source.getInt(6)+1;
      residues=new Residue[residueCount];

      for(int i=0; i<residueCount; i++) {
         residues[i]=Residue.createInstance(source, this);
      }

      // read mapping entries

      int mappingCount=source.getInt(6)+1;
      mappings=new Mapping[mappingCount];

      for(int i=0; i<mappingCount; i++) {
         mappings[i]=Mapping.createInstance(vorbis, source, this);
      }

      // read mode entries

      int modeCount=source.getInt(6)+1;
      modes=new Mode[modeCount];

      for(int i=0; i<modeCount; i++) {
         modes[i]=new Mode(source, this);
      }

      if(!source.getBit()) {
         throw new VorbisFormatException("The setup header framing bit is incorrect.");
      }
   }

   public CodeBook[] getCodeBooks() {
      return codeBooks;
   }

   public Floor[] getFloors() {
      return floors;
   }

   public Residue[] getResidues() {
      return residues;
   }

   public Mapping[] getMappings() {
      return mappings;
   }

   public Mode[] getModes() {
      return modes;
   }
}