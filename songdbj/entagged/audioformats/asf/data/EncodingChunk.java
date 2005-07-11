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
package entagged.audioformats.asf.data;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import entagged.audioformats.asf.util.Utils;

/**
 * This class was intended to store the data of a chunk which contained the
 * encoding parameters in textual form. <br>
 * Since the needed parameters were found in other chunks the implementation of
 * this class was paused. <br>
 * TODO complete analysis. 
 * 
 * @author Christian Laireiter
 */
public class EncodingChunk extends Chunk {

	/**
	 * The read strings.
	 */
	private final ArrayList strings;

	/**
	 * Creates an instance.
	 * 
	 * @param pos
	 *                  Position of the chunk within file or stream
	 * @param chunkLen
	 *                  Length of current chunk.
	 */
	public EncodingChunk(long pos, BigInteger chunkLen) {
		super(GUID.GUID_ENCODING, pos, chunkLen);
		this.strings = new ArrayList();
	}

	/**
	 * This method appends a String.
	 * 
	 * @param toAdd
	 *                  String to add.
	 */
	public void addString(String toAdd) {
		strings.add(toAdd);
	}

	/**
	 * This method returns a collection of all {@link String}s which were addid
	 * due {@link #addString(String)}.
	 * 
	 * @return Inserted Strings.
	 */
	public Collection getStrings() {
		return new ArrayList(strings);
	}

	/**
	 * (overridden)
	 * 
	 * @see entagged.audioformats.asf.data.Chunk#prettyPrint()
	 */
	public String prettyPrint() {
		StringBuffer result = new StringBuffer(super.prettyPrint());
		result.insert(0, Utils.LINE_SEPARATOR + "Encoding:"
				+ Utils.LINE_SEPARATOR);
		Iterator iterator = this.strings.iterator();
		while (iterator.hasNext()) {
			result.append("   " + iterator.next() + Utils.LINE_SEPARATOR);
		}
		return result.toString();
	}
}