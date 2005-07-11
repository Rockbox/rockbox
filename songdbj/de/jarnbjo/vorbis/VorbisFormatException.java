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
 * Revision 1.2  2005/02/09 23:10:47  shred
 * Serial UID für jarnbjo
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

/**
 * Exception thrown when trying to read a corrupted Vorbis stream.
 */

public class VorbisFormatException extends IOException {
  private static final long serialVersionUID = 3616453405694834743L;

  public VorbisFormatException() {
      super();
   }

   public VorbisFormatException(String message) {
      super(message);
   }
}