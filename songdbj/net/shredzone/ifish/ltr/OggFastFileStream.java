/*
 * iFish -- An iRiver iHP jukebox database creation tool
 *
 * Copyright (c) 2004 Richard "Shred" Körber
 *   http://www.shredzone.net/go/ifish
 *
 *-----------------------------------------------------------------------
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is IFISH.
 *
 * The Initial Developer of the Original Code is
 * Richard "Shred" Körber.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK *****
 */
 
package net.shredzone.ifish.ltr;

import java.io.*;
import java.util.*;

import de.jarnbjo.ogg.*;

/**
 * Replacement file reader class. The original J-Ogg FileStream has the
 * major disadvantage that it reads the entire file, even though we just
 * need a few byte from it. This FastFileStream will only read as little
 * information as possible.
 *
 * @author    Richard Körber &lt;dev@shredzone.de&gt;
 * @version   $Id$
 */
public class OggFastFileStream implements PhysicalOggStream {
  private InputStream sourceStream;
  private boolean     closed          = false;
  private int         contentLength   = 0;
  private int         position        = 0;
  private HashMap     logicalStreams  = new HashMap();
  private OggPage     firstPage;

  /**
   * Constructor for the OggFastFileStream object
   *
   * @param   in                  RandomAccessFile to be read
   * @throws  OggFormatException  Bad format
   * @throws  IOException         IO error
   */
  public OggFastFileStream( RandomAccessFile in )
  throws OggFormatException, IOException {
    this.sourceStream = new RandomAdapterInputStream( in );    
    contentLength = (int) in.length();
    firstPage = OggPage.create( sourceStream );
    position += firstPage.getTotalLength();
    LogicalOggStreamImpl los  = new LogicalOggStreamImpl( this, firstPage.getStreamSerialNumber() );
    logicalStreams.put( new Integer( firstPage.getStreamSerialNumber() ), los );
    los.checkFormat( firstPage );
  }

  /**
   * Get a collection of the logical streams.
   *
   * @return   Collection
   */
  public Collection getLogicalStreams() {
    return logicalStreams.values();
  }

  /**
   * Checks if the file is open.
   *
   * @return   true: open, false: closed
   */
  public boolean isOpen() {
    return !closed;
  }

  /**
   * Closes the stream
   *
   * @throws  IOException   IO error
   */
  public void close()
  throws IOException {
    closed = true;
    sourceStream.close();
  }

  /**
   * Get the content length
   *
   * @return  The content length
   */
  public int getContentLength() {
    return contentLength;
  }

  /**
   * Get the current position
   *
   * @return   Position
   */
  public int getPosition() {
    return position;
  }

  /**
   * Get an OggPage.
   *
   * @param   index         Index to be fetched
   * @return  The oggPage value
   * @throws  IOException   IO Error
   */
  public OggPage getOggPage( int index )
  throws IOException {
    if( firstPage != null ) {
      OggPage tmp  = firstPage;
      firstPage = null;
      return tmp;
    } else {
      OggPage page  = OggPage.create( sourceStream );
      position += page.getTotalLength();
      return page;
    }
  }

  /**
   * Move the stream to a certain time position.
   *
   * @param   granulePosition  The new position
   * @throws  IOException
   */
  public void setTime( long granulePosition )
  throws IOException {
    throw new UnsupportedOperationException( "not supported" );
  }

  /**
   * Is this FileStream seekable? We pretend we are not, so J-Ogg
   * will not get some stupid thoughts... ;)
   *
   * @return  false
   */
  public boolean isSeekable() {
    return false;
  }
  
/*--------------------------------------------------------------------*/  
  
  /**
   * This class repairs a design flaw in JDK1.0. A RandomAccessFile
   * is not derived from InputStream, though it provides the same API.
   * This Adapter gives an InputStream view of a Random Access File.
   * <p>
   * For a detailed method description, see InputStream.
   */
  private static class RandomAdapterInputStream extends InputStream {
    private RandomAccessFile rf;
    
    /**
     * Create a new Adapter.
     *
     * @param   rf        RandomAccessFile to be used
     */
    public RandomAdapterInputStream( RandomAccessFile rf ) {
      this.rf = rf;
    }
    
    /**
     * Read a byte.
     *
     * @return  Read byte or -1.
     */
    public int read() throws IOException {
      return rf.read();
    }
    
    /**
     * Read a byte array.
     *
     * @param   b       Byte array to be read
     * @return  Number of bytes read or -1
     */
    public int read( byte[] b) throws IOException {
      return rf.read(b);
    }
    
    /**
     * Read into a byte array.
     *
     * @param   b       Byte array to be read
     * @param   off     Starting offset
     * @param   len     Length
     * @return  Number of bytes read or -1
     */
    public int read( byte[] b, int off, int len) throws IOException {
      return rf.read( b, off, len );
    }
                  
    /**
     * Skip a number of bytes in forward direction.
     *
     * @param   n       Number of bytes to skip
     * @return  Number of bytes skipped, or -1
     */
    public long skip( long n ) throws IOException {
      return rf.skipBytes( (int) n );
    }
    
    /**
     * Return the number of available bytes. Here it is the number of
     * bytes remaining until EOF.
     *
     * @return  Number of bytes available.
     */
    public int available() throws IOException {
      return (int) (rf.length() - rf.getFilePointer());
    }
              
  }

}
