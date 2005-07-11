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
 * Revision 1.1  2003/03/03 21:02:20  jarnbjo
 * no message
 *
 */

package de.jarnbjo.ogg;

import java.io.IOException;

/**
 * Exception thrown when trying to read a corrupted Ogg stream.
 */

public class OggFormatException extends IOException {
  private static final long serialVersionUID = 3544953238333175349L;

  public OggFormatException() {
      super();
   }

   public OggFormatException(String message) {
      super(message);
   }
}