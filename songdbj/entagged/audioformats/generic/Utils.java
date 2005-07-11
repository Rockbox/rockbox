/*
 *  ********************************************************************   **
 *  Copyright notice                                                       **
 *  **																	   **
 *  (c) 2003 Entagged Developpement Team				                   **
 *  http://www.sourceforge.net/projects/entagged                           **
 *  **																	   **
 *  All rights reserved                                                    **
 *  **																	   **
 *  This script is part of the Entagged project. The Entagged 			   **
 *  project is free software; you can redistribute it and/or modify        **
 *  it under the terms of the GNU General Public License as published by   **
 *  the Free Software Foundation; either version 2 of the License, or      **
 *  (at your option) any later version.                                    **
 *  **																	   **
 *  The GNU General Public License can be found at                         **
 *  http://www.gnu.org/copyleft/gpl.html.                                  **
 *  **																	   **
 *  This copyright notice MUST APPEAR in all copies of the file!           **
 *  ********************************************************************
 */
package entagged.audioformats.generic;

import java.io.*;

/*
 * Contains various frequently used static functions in the different tag formats
 * 
 *@author	Raphael Slinckx
 *@version	$Id$
 *@since	v0.02
 */
public class Utils {
	/*
	 * Returns the extension of the given file.
	 * The extension is empty if there is no extension
	 * The extension is the string after the last "."
	 * 
	 * @param f The file whose extension is requested
	 * @return The extension of the given file
	 */
	public static String getExtension(File f) {
		String name = f.getName().toLowerCase();
		int i = name.lastIndexOf( "." );
		if(i == -1)
			return "";
		
		return name.substring( i + 1 );
	}
	
	/*
	 * Tries to convert a string into an UTF8 array of bytes
	 * If the conversion fails, return the string converted with the default
	 * encoding.
	 * 
	 * @param s The string to convert
	 * @return The byte array representation of this string in UTF8 encoding
	 */
	public static byte[] getUTF8Bytes(String s) throws UnsupportedEncodingException {
		return s.getBytes("UTF-8");
	}
	
	/*
	 * Computes a number composed of (end-start) bytes in the b array.
	 * 
	 * @param b The byte array
	 * @param start The starting offset in b (b[offset]). The less significant byte
	 * @param end The end index (included) in b (b[end]). The most significant byte
	 * @return a long number represented by the byte sequence.
	 */
	public static long getLongNumber(byte[] b, int start, int end) {
		long number = 0;
		for(int i = 0; i<(end-start+1); i++) {
			number += ((b[start+i]&0xFF) << i*8);
		}
		
		return number;
	}
	
	/*
	 * same as above, but returns an int instead of a long
	 * @param b The byte array
	 * @param start The starting offset in b (b[offset]). The less significant byte
	 * @param end The end index (included) in b (b[end]). The most significant byte
	 * @return a int number represented by the byte sequence.
	 */
	public static int getNumber( byte[] b, int start, int end) {
		int number = 0;
		for(int i = 0; i<(end-start+1); i++) {
			number += ((b[start+i]&0xFF) << i*8);
		}
		
		return number;
	}
}
