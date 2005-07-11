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
package entagged.audioformats.asf;

import java.io.IOException;
import java.io.RandomAccessFile;

import entagged.audioformats.EncodingInfo;
import entagged.audioformats.Tag;
import entagged.audioformats.asf.data.AsfHeader;
import entagged.audioformats.asf.io.AsfHeaderReader;
import entagged.audioformats.asf.util.TagConverter;
import entagged.audioformats.exceptions.CannotReadException;
import entagged.audioformats.generic.AudioFileReader;

/**
 * This reader can read asf files containing any content (stream type). <br>
 * 
 * @author Christian Laireiter
 */
public class AsfFileReader extends AudioFileReader {

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.AudioFileReader#getEncodingInfo(java.io.RandomAccessFile)
     */
    protected EncodingInfo getEncodingInfo(RandomAccessFile raf)
            throws CannotReadException, IOException {
        raf.seek(0);
        EncodingInfo info = new EncodingInfo();
        try {
            AsfHeader header = AsfHeaderReader.readHeader(raf);
            if (header == null) {
                throw new CannotReadException(
                        "Some values must have been "
                                + "incorrect for interpretation as asf with wma content.");
            }
            info.setBitrate(header.getAudioStreamChunk().getKbps());
            info.setChannelNumber((int) header.getAudioStreamChunk()
                    .getChannelCount());
            info.setEncodingType("ASF (audio): "+header.getAudioStreamChunk()
                    .getCodecDescription());
            info.setLength(header.getFileHeader().getDurationInSeconds());
            info.setSamplingRate((int) header.getAudioStreamChunk()
                    .getSamplingRate());

        } catch (Exception e) {
            if (e instanceof IOException)
                throw (IOException) e;
            else if (e instanceof CannotReadException)
                throw (CannotReadException) e;
            else {
                throw new CannotReadException("Failed to read. Cause: "
                        + e.getMessage());
            }
        }
        return info;
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.AudioFileReader#getTag(java.io.RandomAccessFile)
     */
    protected Tag getTag(RandomAccessFile raf) throws CannotReadException,
            IOException {
        raf.seek(0);
        Tag tag = null;
        try {
            AsfHeader header = AsfHeaderReader.readHeader(raf);
            if (header == null) {
                throw new CannotReadException(
                        "Some values must have been "
                                + "incorrect for interpretation as asf with wma content.");
            }

            tag = TagConverter.createTagOf(header);

        } catch (Exception e) {
            if (e instanceof IOException)
                throw (IOException) e;
            else if (e instanceof CannotReadException)
                throw (CannotReadException) e;
            else {
                throw new CannotReadException("Failed to read. Cause: "
                        + e.getMessage());
            }
        }
        return tag;
    }

}