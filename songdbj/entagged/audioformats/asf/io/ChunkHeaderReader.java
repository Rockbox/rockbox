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
package entagged.audioformats.asf.io;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.math.BigInteger;

import entagged.audioformats.asf.data.Chunk;
import entagged.audioformats.asf.data.GUID;
import entagged.audioformats.asf.util.Utils;

/**
 * Default reader, Reads GUID and size out of an inputsream and creates a
 * {@link entagged.audioformats.asf.data.Chunk}object.
 * 
 * @author Christian Laireiter
 */
class ChunkHeaderReader {

	/**
	 * Interprets current data as a header of a chunk.
	 * 
	 * @param input
	 *                  inputdata
	 * @return Chunk.
	 * @throws IOException
	 *                   Access errors.
	 */
	public static Chunk readChunckHeader(RandomAccessFile input)
			throws IOException {
		long pos = input.getFilePointer();
		GUID guid = Utils.readGUID(input);
		BigInteger chunkLength = Utils.readBig64(input);
		return new Chunk(guid, pos, chunkLength);
	}

}