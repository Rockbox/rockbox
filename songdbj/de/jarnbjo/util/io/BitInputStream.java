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
 * Revision 1.5  2003/04/10 19:48:31  jarnbjo
 * no message
 *
 * Revision 1.4  2003/03/16 20:57:06  jarnbjo
 * no message
 *
 * Revision 1.3  2003/03/16 20:56:56  jarnbjo
 * no message
 *
 * Revision 1.2  2003/03/16 01:11:39  jarnbjo
 * no message
 *
 * Revision 1.1  2003/03/03 21:02:20  jarnbjo
 * no message
 *
 */

package de.jarnbjo.util.io;

import java.io.IOException;

/**
 *	An interface with methods allowing bit-wise reading from
 * an input stream. All methods in this interface are optional
 * and an implementation not support a method or a specific state
 * (e.g. endian) will throw an UnspportedOperationException if
 * such a method is being called. This should be speicified in
 * the implementation documentation.
 */

public interface BitInputStream {
	
	/**
	 *  constant for setting this stream's mode to little endian
	 *
	 *  @see #setEndian(int)
	 */
	 
   public static final int LITTLE_ENDIAN = 0;

	/**
	 *  constant for setting this stream's mode to big endian
	 *
	 *  @see #setEndian(int)
	 */

   public static final int BIG_ENDIAN = 1;

	/**
	 *  reads one bit (as a boolean) from the input stream
	 *
	 *  @return <code>true</code> if the next bit is 1, 
	 *          <code>false</code> otherwise
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */
	 
   public boolean getBit() throws IOException;

	/**
	 *  reads <code>bits</code> number of bits from the input
	 *  stream
	 *
	 *  @return the unsigned integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */
	 
   public int getInt(int bits) throws IOException;

	/**
	 *  reads <code>bits</code> number of bits from the input
	 *  stream
	 *
	 *  @return the signed integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public int getSignedInt(int bits) throws IOException;

	/**
	 *  reads a huffman codeword based on the <code>root</code> 
	 *  parameter and returns the decoded value
	 *
	 *  @param root the root of the Huffman tree used to decode the codeword
	 *  @return the decoded unsigned integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public int getInt(HuffmanNode root) throws IOException;

	/**
	 *  reads an integer encoded as "signed rice" as described in
	 *  the FLAC audio format specification
	 *
	 *  @param order 
	 *  @return the decoded integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public int readSignedRice(int order) throws IOException;

	/**
	 *  fills the array from <code>offset</code> with <code>len</code> 
	 *  integers encoded as "signed rice" as described in
	 *  the FLAC audio format specification
	 *
	 *  @param order 
	 *  @param buffer
	 *  @param offset
	 *  @param len 
	 *  @return the decoded integer value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public void readSignedRice(int order, int[] buffer, int offset, int len) throws IOException;

	/**
	 *  reads <code>bits</code> number of bits from the input
	 *  stream
	 *
	 *  @return the unsigned long value read from the stream
	 *
	 *  @throws IOException if an I/O error occurs
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public long getLong(int bits) throws IOException;

	/**
	 *  causes the read pointer to be moved to the beginning
	 *  of the next byte, remaining bits in the current byte
	 *  are discarded
	 *
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public void align();

	/**
	 *  changes the endian mode used when reading bit-wise from
	 *  the stream, changing the mode mid-stream will cause the
	 *  read cursor to move to the beginning of the next byte
	 *  (as if calling the <code>allign</code> method
	 *
	 *  @see #align()
	 *
	 *  @throws UnsupportedOperationException if the method is not supported by the implementation
	 */

   public void setEndian(int endian);
}