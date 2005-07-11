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

import java.util.Arrays;

import entagged.audioformats.asf.util.Utils;

/**
 * This class is used for representation of GUIDs and as a reference list of all
 * Known GUIDs. <br>
 * 
 * @author Christian Laireiter
 */
public class GUID {

    /**
     * This constant defines the GUID for stream chunks describing audio
     * streams, indicating the the audio stream has no error concealment. <br>
     */
    public final static GUID GUID_AUDIO_ERROR_CONCEALEMENT_ABSENT = new GUID(
            new int[] { 0x40, 0xA4, 0xF1, 0x49, 0xCE, 0x4E, 0xD0, 0x11, 0xA3,
                    0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6 },
            "Audio error concealment absent.");

    /**
     * This constant defines the GUID for stream chunks describing audio
     * streams, indicating the the audio stream has interleaved error
     * concealment. <br>
     */
    public final static GUID GUID_AUDIO_ERROR_CONCEALEMENT_INTERLEAVED = new GUID(
            new int[] { 0x40, 0xA4, 0xF1, 0x49, 0xCE, 0x4E, 0xD0, 0x11, 0xA3,
                    0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6 },
            "Interleaved audio error concealment.");

    /**
     * This constant stores the GUID indicating that stream type is audio.
     */
    public final static GUID GUID_AUDIOSTREAM = new GUID(new int[] { 0x40,
            0x9E, 0x69, 0xF8, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80,
            0x5F, 0x5C, 0x44, 0x2B }, " Audio stream");

    /**
     * This constant represents the guid for a chunk which contains Title,
     * author, copyright, description and rating.
     */
    public final static GUID GUID_CONTENTDESCRIPTION = new GUID(new int[] {
            0x33, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00,
            0xAA, 0x00, 0x62, 0xCE, 0x6C }, "Content Description");

    /**
     * This constant stores the GUID for Encoding-Info chunks.
     */
    public final static GUID GUID_ENCODING = new GUID(new int[] { 0x40, 0x52,
            0xD1, 0x86, 0x1D, 0x31, 0xD0, 0x11, 0xA3, 0xA4, 0x00, 0xA0, 0xC9,
            0x03, 0x48, 0xF6 }, "Encoding description");

    /**
     * This constant defines the GUID for a WMA "Extended Content Description"
     * chunk. <br>
     */
    public final static GUID GUID_EXTENDED_CONTENT_DESCRIPTION = new GUID(
            new int[] { 0x40, 0xA4, 0xD0, 0xD2, 0x07, 0xE3, 0xD2, 0x11, 0x97,
                    0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 },
            "Extended Content Description");

    /**
     * GUID of ASF file header.
     */
    public final static GUID GUID_FILE = new GUID(new int[] { 0xA1, 0xDC, 0xAB,
            0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20,
            0x53, 0x65 }, "File header");

    /**
     * This constant defines the GUID of a asf header chunk.
     */
    public final static GUID GUID_HEADER = new GUID(new int[] { 0x30, 0x26,
            0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00,
            0x62, 0xce, 0x6c }, "Asf header");

    /**
     * This constant stores the length of GUIDs used with ASF streams. <br>
     */
    public final static int GUID_LENGTH = 16;

    /**
     * This constant stores the GUID indicating a stream object.
     */
    public final static GUID GUID_STREAM = new GUID(new int[] { 0x91, 0x07,
            0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C,
            0x20, 0x53, 0x65 }, "Stream");

    /**
     * This constant stores a GUID whose functionality is unknown.
     */
    public final static GUID GUID_UNKNOWN_1 = new GUID(new int[] { 0xB5, 0x03,
            0xBF, 0x5F, 0x2E, 0xA9, 0xCF, 0x11, 0x8E, 0xE3, 0x00, 0xC0, 0x0C,
            0x20, 0x53, 0x65 }, "Unknown 1");

    /**
     * This constant stores a GUID whose functionality is unknown.
     */
    public final static GUID GUID_UNKNOWN_2 = new GUID(new int[] { 0xCE, 0x75,
            0xF8, 0x7B, 0x8D, 0x46, 0xD1, 0x11, 0x8D, 0x82, 0x00, 0x60, 0x97,
            0xC9, 0xA2, 0xB2 }, "Unknown 2");

    /**
     * This constant stores the GUID indicating that stream type is video.
     */
    public final static GUID GUID_VIDEOSTREAM = new GUID(new int[] { 0xC0,
            0xEF, 0x19, 0xBC, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80,
            0x5F, 0x5C, 0x44, 0x2B }, "Video stream");

    /**
     * This field stores all knwon GUIDs.
     */
    public final static GUID[] KNOWN_GUIDS = new GUID[] {
            GUID_AUDIO_ERROR_CONCEALEMENT_ABSENT,
            GUID_AUDIO_ERROR_CONCEALEMENT_INTERLEAVED, GUID_CONTENTDESCRIPTION,
            GUID_AUDIOSTREAM, GUID_ENCODING, GUID_FILE, GUID_HEADER,
            GUID_STREAM, GUID_EXTENDED_CONTENT_DESCRIPTION, GUID_VIDEOSTREAM,
            GUID_UNKNOWN_1, GUID_UNKNOWN_2 };

    /**
     * This method checks if the given <code>value</code> is matching the GUID
     * specification of ASF streams. <br>
     * 
     * @param value
     *                   possible GUID.
     * @return <code>true</code> if <code>value</code> matches the
     *               specification of a GUID.
     */
    public static boolean assertGUID(int[] value) {
        boolean result = false;
        if (value != null) {
            if (value.length == GUID.GUID_LENGTH) {
                result = true;
            }
        }
        return result;
    }

    /**
     * This method searches a GUID in {@link #KNOWN_GUIDS}which is equal to the
     * given <code>guid</code> and returns its description. <br>
     * This method is useful if a guid was read out of a file and no
     * identification has been done yet.
     * 
     * @param guid
     *                   guid, which description is needed.
     * @return description of the guid if found. Else <code>null</code>
     */
    public static String getGuidDescription(GUID guid) {
        String result = null;
        if (guid == null) {
            throw new IllegalArgumentException("Argument must not be null.");
        }
        for (int i = 0; i < KNOWN_GUIDS.length; i++) {
            if (KNOWN_GUIDS[i].equals(guid)) {
                result = KNOWN_GUIDS[i].getDescription();
            }
        }
        return result;
    }

    /**
     * Stores an optionally description of the GUID.
     */
    private String description = "";

    /**
     * An isntance of this class stores the value of the wrapped GUID in this
     * field. <br>
     */
    private int[] guid = null;

    /**
     * Creates an empty instance.
     *  
     */
    public GUID() {
        // Nothing to do
    }

    /**
     * Creates an instance and assigns given <code>guid</code>.<br>
     * 
     * @param value
     *                   Guid, which should be assigned.
     */
    public GUID(int[] value) {
        setGUID(value);
    }

    /**
     * Creates an instance like {@link #GUID(int[])}and sets the optional
     * description. <br>
     * 
     * @param value
     *                   Guid, which should be assigned.
     * @param desc
     *                   Description for the guid.
     */
    public GUID(int[] value, String desc) {
        this(value);
        if (desc == null) {
            throw new IllegalArgumentException("Argument must not be null.");
        }
        this.description = desc;
    }

    /**
     * This method compares two objects. If the given Object is a {@link  GUID},
     * the stored GUID values are compared. <br>
     * 
     * @see java.lang.Object#equals(java.lang.Object)
     */
    public boolean equals(Object obj) {
        boolean result = false;
        if (obj instanceof GUID) {
            GUID other = (GUID) obj;
            result = Arrays.equals(this.getGUID(), other.getGUID());
        } else {
            result = super.equals(obj);
        }
        return result;
    }

    /**
     * This method returns the guid as an array of bytes. <br>
     * 
     * @see #getGUID()
     * @return The guid as a byte array.
     */
    public byte[] getBytes() {
        byte[] result = new byte[this.guid.length];
        for (int i = 0; i < result.length; i++) {
            result[i] = (byte) (this.guid[i] & 0xFF);
        }
        return result;
    }

    /**
     * @return Returns the description.
     */
    public String getDescription() {
        return description;
    }

    /**
     * This method returns the GUID of this object. <br>
     * 
     * @return stored GUID.
     */
    public int[] getGUID() {
        int[] copy = new int[this.guid.length];
        System.arraycopy(this.guid, 0, copy, 0, this.guid.length);
        return copy;
    }

    /**
     * This method checks if the currently stored GUID ({@link #guid}) is
     * correctly filled. <br>
     * 
     * @return <code>true</code> if it is.
     */
    public boolean isValid() {
        return assertGUID(getGUID());
    }

    /**
     * This method saves a copy of the given <code>value</code> as the
     * represented value of this object. <br>
     * The given value is checked with {@link #assertGUID(int[])}.<br>
     * 
     * @param value
     *                   GUID to assign.
     */
    private void setGUID(int[] value) {
        if (assertGUID(value)) {
            this.guid = new int[GUID_LENGTH];
            System.arraycopy(value, 0, this.guid, 0, GUID_LENGTH);
        } else {
            throw new IllegalArgumentException(
                    "The given guid doesn't match the GUID specification.");
        }
    }

    /**
     * This method gives a hex formatted representation of {@link #getGUID()}
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuffer result = new StringBuffer();
        if (getDescription().trim().length() > 0) {
            result.append("Description: " + getDescription()
                    + Utils.LINE_SEPARATOR + "   ");
        }
        for (int i = 0; i < guid.length; i++) {
            String tmp = Integer.toHexString(guid[i]);
            if (tmp.length() < 2)
                tmp = "0" + tmp;
            if (i > 0)
                result.append(", ");
            result.append("0x" + tmp);
        }
        return result.toString();
    }
}