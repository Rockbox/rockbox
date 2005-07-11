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

import entagged.audioformats.asf.data.ContentDescription;
import entagged.audioformats.asf.data.Chunk;
import entagged.audioformats.asf.data.GUID;
import entagged.audioformats.asf.util.Utils;

/**
 * Reads and interprets the data of a asf chunk containing title, author... <br>
 * 
 * @see entagged.audioformats.asf.data.ContentDescription
 * 
 * @author Christian Laireiter
 */
public class ContentDescriptionReader {

    /**
     * Creates and fills a
     * {@link entagged.audioformats.asf.data.ContentDescription}from given
     * file. <br>
     * 
     * @param raf
     *                  Input
     * @param candidate
     *                  Chunk which possibly is a file header.
     * @return FileHeader if filepointer of <code>raf</code> is at valid
     *              fileheader.
     * @throws IOException
     *                   Read errors.
     */
    public static ContentDescription read(RandomAccessFile raf, Chunk candidate)
            throws IOException {
        if (raf == null || candidate == null) {
            throw new IllegalArgumentException("Arguments must not be null.");
        }
        if (GUID.GUID_CONTENTDESCRIPTION.equals(candidate.getGuid())) {
            raf.seek(candidate.getPosition());
            return new ContentDescriptionReader().parseData(raf);
        }
        return null;
    }

    /**
     * This method reads a UTF-16 encoded String. <br>
     * For the use this method the number of bytes used by current string must
     * be known. <br>
     * The ASF spec recommends that those strings end with a terminating zero.
     * However it also says that it is not always the case.
     * 
     * @param raf
     *                  Input source
     * @param strLen
     *                  Number of bytes the String may take.
     * @return read String.
     * @throws IOException
     *                   read errors.
     */
    public static String readFixedSizeUTF16Str(RandomAccessFile raf, int strLen)
            throws IOException {
        byte[] strBytes = new byte[strLen];
        int read = raf.read(strBytes);
        if (read == strBytes.length) {
            if (strBytes.length >= 2) {
                /*
                 * Zero termination is recommended but optional.
                 * So check and if, remove.
                 */
                if (strBytes[strBytes.length-1] == 0 && strBytes[strBytes.length-2] == 0) {
                    byte[] copy = new byte[strBytes.length-2];
                    System.arraycopy(strBytes, 0, copy, 0, strBytes.length-2);
                    strBytes = copy;
                }
            }
            return new String(strBytes, "UTF-16LE");
        }
        throw new IllegalStateException(
                "Couldn't read the necessary amount of bytes.");
    }

    /**
     * Should not be used for now.
     *  
     */
    protected ContentDescriptionReader() {
        // NOTHING toDo
    }

    /**
     * Directly behind the GUID and chunkSize of the current chunck comes 5
     * sizes (16-bit) of string lengths. <br>
     * 
     * @param raf
     *                  input source
     * @return Number and length of Strings, which are directly behind
     *              filepointer if method exits.
     * @throws IOException
     *                   read errors.
     */
    private int[] getStringSizes(RandomAccessFile raf) throws IOException {
        int[] result = new int[5];
        for (int i = 0; i < result.length; i++) {
            result[i] = Utils.readUINT16(raf);
        }
        return result;
    }

    /**
     * Does the job of {@link #read(RandomAccessFile, Chunk)}
     * 
     * @param raf
     *                  input source
     * @return Contentdescription
     * @throws IOException
     *                   read errors.
     */
    private ContentDescription parseData(RandomAccessFile raf)
            throws IOException {
        ContentDescription result = null;
        long chunkStart = raf.getFilePointer();
        GUID guid = Utils.readGUID(raf);
        if (GUID.GUID_CONTENTDESCRIPTION.equals(guid)) {
            BigInteger chunkLen = Utils.readBig64(raf);
            result = new ContentDescription(chunkStart, chunkLen);
            /*
             * Now comes 16-Bit values representing the length of the Strings
             * which follows.
             */
            int[] stringSizes = getStringSizes(raf);
            /*
             * Now we know the String length of each occuring String.
             */
            String[] strings = new String[stringSizes.length];
            for (int i = 0; i < strings.length; i++) {
                if (stringSizes[i] > 0)
                    strings[i] = readFixedSizeUTF16Str(raf, stringSizes[i]);
            }
            if (stringSizes[0] > 0)
                result.setTitle(strings[0]);
            if (stringSizes[1] > 0)
                result.setAuthor(strings[1]);
            if (stringSizes[2] > 0)
                result.setCopyRight(strings[2]);
            if (stringSizes[3] > 0)
                result.setComment(strings[3]);
            if (stringSizes[4] > 0)
                result.setRating(strings[4]);
        }
        return result;
    }
}