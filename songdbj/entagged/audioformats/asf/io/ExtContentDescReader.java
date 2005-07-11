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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.math.BigInteger;

import entagged.audioformats.asf.data.Chunk;
import entagged.audioformats.asf.data.ContentDescriptor;
import entagged.audioformats.asf.data.ExtendedContentDescription;
import entagged.audioformats.asf.data.GUID;
import entagged.audioformats.asf.util.Utils;

/**
 * Class for reading Tag information out of the extended content description of
 * an asf file. <br>
 * 
 * @see entagged.audioformats.asf.data.ExtendedContentDescription
 * 
 * @author Christian Laireiter
 */
public class ExtContentDescReader {

    /**
     * Reads the current chunk if it is a matching one.
     * 
     * @param raf
     *                  Input source
     * @param candidate
     *                  Chunk which possibly contains additional tags
     * @return Wrapper for the extended content description
     * @throws IOException
     *                   Read errors
     */
    public static ExtendedContentDescription read(RandomAccessFile raf,
            Chunk candidate) throws IOException {
        if (raf == null || candidate == null) {
            throw new IllegalArgumentException("Arguments must not be null.");
        }
        if (GUID.GUID_EXTENDED_CONTENT_DESCRIPTION.equals(candidate.getGuid())) {
            raf.seek(candidate.getPosition());
            return new ExtContentDescReader().parseData(raf);
        }
        return null;
    }

    /**
     * Should not be used for now.
     *  
     */
    protected ExtContentDescReader() {
        // NOTHING toDo
    }

    /**
     * Does the job of {@link #read(RandomAccessFile, Chunk)}
     * 
     * @param raf
     *                  Input source
     * @return Wrapper for properties
     * @throws IOException
     *                   read errors.
     */
    private ExtendedContentDescription parseData(RandomAccessFile raf)
            throws IOException {
        ExtendedContentDescription result = null;
        long chunkStart = raf.getFilePointer();
        GUID guid = Utils.readGUID(raf);

        if (GUID.GUID_EXTENDED_CONTENT_DESCRIPTION.equals(guid)) {
            BigInteger chunkLen = Utils.readBig64(raf);

            // Reading Number of Tags.
            long descriptorCount = Utils.readUINT16(raf);

            // Create Result object
            result = new ExtendedContentDescription(chunkStart, chunkLen);

            for (long i = 0; i < descriptorCount; i++) {
                String tagElement = Utils.readUTF16LEStr(raf);
                int type = Utils.readUINT16(raf);
                ContentDescriptor prop = new ContentDescriptor(tagElement, type);
                switch (type) {
                case ContentDescriptor.TYPE_STRING:
                    prop.setStringValue(Utils.readUTF16LEStr(raf));
                    break;
                case ContentDescriptor.TYPE_BINARY:
                    prop.setBinaryValue(readBinaryData(raf));
                    break;
                case ContentDescriptor.TYPE_BOOLEAN:
                    prop.setBooleanValue(readBoolean(raf));
                    break;
                case ContentDescriptor.TYPE_DWORD:
                    raf.skipBytes(2);
                    prop.setDWordValue(Utils.readUINT32(raf));
                    break;
                case ContentDescriptor.TYPE_WORD:
                    raf.skipBytes(2);
                    prop.setWordValue(Utils.readUINT16(raf));
                    break;
                case ContentDescriptor.TYPE_QWORD:
                    raf.skipBytes(2);
                    prop.setQWordValue(Utils.readUINT64(raf));
                    break;
                default:
                    // Unknown, hopefully the convention for the size of the
                    // value
                    // is given, so we could read it binary
                    prop.setStringValue("Invalid datatype: "
                            + new String(readBinaryData(raf)));
                }
                result.addDescriptor(prop);
            }
        }
        return result;
    }

    /**
     * This method read binary Data. <br>
     * 
     * @param raf
     *                  input source.
     * @return the binary data
     * @throws IOException
     *                   read errors.
     */
    private byte[] readBinaryData(RandomAccessFile raf) throws IOException {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        int size = Utils.readUINT16(raf);
        for (int i = 0; i < size; i++) {
            bos.write(raf.read());
        }
        return bos.toByteArray();
    }

    /**
     * This Method reads a boolean value out of the tag chunk. <br>
     * A boolean requires 6 bytes. This means we've got 3 16-Bit unsigned
     * numbers. The first number should always be 4 because the other 2 numbers
     * needs them. The second number seems to take the values 0 (for
     * <code>false</code>) and 1 (for <code>true</code>). The third one is
     * zero, maybe indication the end of the value. <br>
     * 
     * @param raf
     *                  input source
     * @return boolean representation.
     * @throws IOException
     *                   read errors.
     */
    private boolean readBoolean(RandomAccessFile raf) throws IOException {
        int size = Utils.readUINT16(raf);
        if (size != 4)
            throw new IllegalStateException(
                    "Boolean value do require 4 Bytes. (Size value is: " + size
                            + ")");
        long value = Utils.readUINT32(raf);
        boolean result = value == 1;
        return result;
    }

}