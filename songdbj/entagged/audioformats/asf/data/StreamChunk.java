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

import entagged.audioformats.asf.util.Utils;

/**
 * This class is the base for all handled stream contents. <br>
 * A Stream chunk delivers information about a audio or video stream. Because of
 * this the stream chunk identifies in one field what type of stream it is
 * describing and so other data is provided. However some information is common
 * to all stream chunks which are stored in this hierarchy of the class tree.
 * 
 * @author Christian Laireiter
 */
public class StreamChunk extends Chunk {

    /**
     * If <code>true</code>, the stream data is encrypted.
     */
    private boolean contentEncrypted;

    /**
     * This field stores the number of the current stream. <br>
     */
    private int streamNumber;

    /**
     * @see #typeSpecificDataSize
     */
    private long streamSpecificDataSize;

    /**
     * Something technical. <br>
     * Format time in 100-ns steps.
     */
    private long timeOffset;

    /**
     * Stores the size of type specific data structure within chunk.
     */
    private long typeSpecificDataSize;

    /**
     * Creates an instance
     * 
     * @param pos
     *                   Position of chunk within file or stream.
     * @param chunkLen
     *                   length of chunk
     */
    public StreamChunk(long pos, BigInteger chunkLen) {
        super(GUID.GUID_AUDIOSTREAM, pos, chunkLen);
    }

    /**
     * @return Returns the streamNumber.
     */
    public int getStreamNumber() {
        return streamNumber;
    }

    /**
     * @return Returns the streamSpecificDataSize.
     */
    public long getStreamSpecificDataSize() {
        return streamSpecificDataSize;
    }

    /**
     * @return Returns the timeOffset.
     */
    public long getTimeOffset() {
        return timeOffset;
    }

    /**
     * @return Returns the typeSpecificDataSize.
     */
    public long getTypeSpecificDataSize() {
        return typeSpecificDataSize;
    }

    /**
     * @return Returns the contentEncrypted.
     */
    public boolean isContentEncrypted() {
        return contentEncrypted;
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.asf.data.Chunk#prettyPrint()
     */
    public String prettyPrint() {
        StringBuffer result = new StringBuffer(super.prettyPrint());
        result.insert(0, Utils.LINE_SEPARATOR + "Stream Data:"
                + Utils.LINE_SEPARATOR);
        result.append("   Stream number: " + getStreamNumber()
                + Utils.LINE_SEPARATOR);
        result.append("   Type specific data size  : "
                + getTypeSpecificDataSize() + Utils.LINE_SEPARATOR);
        result.append("   Stream specific data size: "
                + getStreamSpecificDataSize() + Utils.LINE_SEPARATOR);
        result.append("   Time Offset              : " + getTimeOffset()
                + Utils.LINE_SEPARATOR);
        result.append("   Content Encryption       : " + isContentEncrypted()
                + Utils.LINE_SEPARATOR);
        return result.toString();
    }

    /**
     * @param cntEnc
     *                   The contentEncrypted to set.
     */
    public void setContentEncrypted(boolean cntEnc) {
        this.contentEncrypted = cntEnc;
    }

    /**
     * @param streamNum
     *                   The streamNumber to set.
     */
    public void setStreamNumber(int streamNum) {
        this.streamNumber = streamNum;
    }

    /**
     * @param strSpecDataSize
     *                   The streamSpecificDataSize to set.
     */
    public void setStreamSpecificDataSize(long strSpecDataSize) {
        this.streamSpecificDataSize = strSpecDataSize;
    }

    /**
     * @param timeOffs
     *                   sets the time offset
     */
    public void setTimeOffset(long timeOffs) {
        this.timeOffset = timeOffs;
    }

    /**
     * @param typeSpecDataSize
     *                   The typeSpecificDataSize to set.
     */
    public void setTypeSpecificDataSize(long typeSpecDataSize) {
        this.typeSpecificDataSize = typeSpecDataSize;
    }
}