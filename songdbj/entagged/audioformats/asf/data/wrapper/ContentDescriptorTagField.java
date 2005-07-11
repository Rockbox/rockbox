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
package entagged.audioformats.asf.data.wrapper;

import java.io.UnsupportedEncodingException;

import entagged.audioformats.asf.data.ContentDescriptor;
import entagged.audioformats.generic.TagField;

/**
 * This class encapsulates a
 * {@link entagged.audioformats.asf.data.ContentDescriptor}and provides access
 * to it. <br>
 * The content descriptor used for construction is copied.
 * 
 * @author Christian Laireiter (liree)
 */
public class ContentDescriptorTagField implements TagField {

    /**
     * This descriptor is wrapped.
     */
    private ContentDescriptor toWrap;

    /**
     * Creates an instance.
     * 
     * @param source
     *                   The descriptor which should be represented as a
     *                   {@link TagField}.
     */
    public ContentDescriptorTagField(ContentDescriptor source) {
        this.toWrap = source.createCopy();
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#copyContent(entagged.audioformats.generic.TagField)
     */
    public void copyContent(TagField field) {
        throw new UnsupportedOperationException("Not implemented yet.");
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#getId()
     */
    public String getId() {
        return toWrap.getName();
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#getRawContent()
     */
    public byte[] getRawContent() throws UnsupportedEncodingException {
        return toWrap.getRawData();
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#isBinary()
     */
    public boolean isBinary() {
        return toWrap.getType() == ContentDescriptor.TYPE_BINARY;
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#isBinary(boolean)
     */
    public void isBinary(boolean b) {
        if (!b && isBinary()) {
            throw new UnsupportedOperationException("No conversion supported.");
        }
        toWrap.setBinaryValue(toWrap.getRawData());
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#isCommon()
     */
    public boolean isCommon() {
        return toWrap.isCommon();
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#isEmpty()
     */
    public boolean isEmpty() {
        return toWrap.isEmpty();
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.generic.TagField#toString()
     */
    public String toString() {
        return toWrap.getString();
    }

}