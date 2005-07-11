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

import java.io.ByteArrayOutputStream;
import java.math.BigInteger;

import entagged.audioformats.asf.util.Utils;

/**
 * This class represents the data of a chunk which contains title, author,
 * copyright, description and the rating of the file. <br>
 * It is optional whithin asf files. But if exists only once.
 * 
 * @author Christian Laireiter
 */
public class ContentDescription extends Chunk {

    /**
     * File artist.
     */
    private String author = null;

    /**
     * File copyright.
     */
    private String copyRight = null;

    /**
     * File comment.
     */
    private String description = null;

    /**
     * File rating.
     */
    private String rating = null;

    /**
     * File title.
     */
    private String title = null;

    /**
     * Creates an instance. <br>
     */
    public ContentDescription() {
        this(0, BigInteger.valueOf(0));
    }

    /**
     * Creates an instance.
     * 
     * @param pos
     *                   Position of content description within file or stream
     * @param chunkLen
     *                   Length of content description.
     */
    public ContentDescription(long pos, BigInteger chunkLen) {
        super(GUID.GUID_CONTENTDESCRIPTION, pos, chunkLen);
    }

    /**
     * @return Returns the author.
     */
    public String getAuthor() {
        if (author == null)
            return "";
        return author;
    }

    /**
     * This method creates a byte array that could directly be written to an asf
     * file. <br>
     * 
     * @return The asf chunk representation of a content description with the
     *               values of the current object.
     */
    public byte[] getBytes() {
        ByteArrayOutputStream result = new ByteArrayOutputStream();
        try {
            ByteArrayOutputStream tags = new ByteArrayOutputStream();
            String[] toWrite = new String[] { getTitle(), getAuthor(),
                    getCopyRight(), getComment(), getRating() };
            byte[][] stringRepresentations = new byte[toWrite.length][];
            // Create byte[] of UTF-16LE encodings
            for (int i = 0; i < toWrite.length; i++) {
                stringRepresentations[i] = toWrite[i].getBytes("UTF-16LE");
            }
            // Write the amount of bytes needed to store the values.
            for (int i = 0; i < stringRepresentations.length; i++) {
                tags.write(Utils.getBytes(stringRepresentations[i].length + 2,
                        2));
            }
            // Write the values themselves.
            for (int i = 0; i < toWrite.length; i++) {
                tags.write(stringRepresentations[i]);
                // Zero term character.
                tags.write(Utils.getBytes(0, 2));
            }
            // Now tags has got the values. The result just needs
            // The GUID, length of the chunk and the tags.
            byte[] tagContent = tags.toByteArray();
            // The guid of the chunk
            result.write(GUID.GUID_CONTENTDESCRIPTION.getBytes());
            /*
             * The length of the chunk. 16 Bytes guid 8 Bytes the length
             * tagContent.length bytes.
             */
            result.write(Utils.getBytes(tagContent.length + 24, 8));
            // The tags.
            result.write(tagContent);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result.toByteArray();
    }

    /**
     * @return Returns the comment.
     */
    public String getComment() {
        if (description == null)
            return "";
        return description;
    }

    /**
     * @return Returns the copyRight.
     */
    public String getCopyRight() {
        if (copyRight == null)
            return "";
        return copyRight;
    }

    /**
     * @return returns the rating.
     */
    public String getRating() {
        if (rating == null)
            return "";
        return rating;
    }

    /**
     * @return Returns the title.
     */
    public String getTitle() {
        if (title == null)
            return "";
        return title;
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.asf.data.Chunk#prettyPrint()
     */
    public String prettyPrint() {
        StringBuffer result = new StringBuffer(super.prettyPrint());
        result.insert(0, Utils.LINE_SEPARATOR + "Content Description:"
                + Utils.LINE_SEPARATOR);
        result.append("   Title      : " + getTitle() + Utils.LINE_SEPARATOR);
        result.append("   Author     : " + getAuthor() + Utils.LINE_SEPARATOR);
        result.append("   Copyright  : " + getCopyRight()
                + Utils.LINE_SEPARATOR);
        result.append("   Description: " + getComment() + Utils.LINE_SEPARATOR);
        result.append("   Rating     :" + getRating() + Utils.LINE_SEPARATOR);
        return result.toString();
    }

    /**
     * @param fileAuthor
     *                   The author to set.
     * @throws IllegalArgumentException
     *                    If "UTF-16LE"-byte-representation would take more than 65535
     *                    bytes.
     */
    public void setAuthor(String fileAuthor) throws IllegalArgumentException {
        Utils.checkStringLengthNullSafe(fileAuthor);
        this.author = fileAuthor;
    }

    /**
     * @param tagComment
     *                   The comment to set.
     * @throws IllegalArgumentException
     *                    If "UTF-16LE"-byte-representation would take more than 65535
     *                    bytes.
     */
    public void setComment(String tagComment) throws IllegalArgumentException {
        Utils.checkStringLengthNullSafe(tagComment);
        this.description = tagComment;
    }

    /**
     * @param cpright
     *                   The copyRight to set.
     * @throws IllegalArgumentException
     *                    If "UTF-16LE"-byte-representation would take more than 65535
     *                    bytes.
     */
    public void setCopyRight(String cpright) throws IllegalArgumentException {
        Utils.checkStringLengthNullSafe(cpright);
        this.copyRight = cpright;
    }

    /**
     * @param ratingText
     *                   The rating to be set.
     * @throws IllegalArgumentException
     *                    If "UTF-16LE"-byte-representation would take more than 65535
     *                    bytes.
     */
    public void setRating(String ratingText) throws IllegalArgumentException {
        Utils.checkStringLengthNullSafe(ratingText);
        this.rating = ratingText;
    }

    /**
     * @param songTitle
     *                   The title to set.
     * @throws IllegalArgumentException
     *                    If "UTF-16LE"-byte-representation would take more than 65535
     *                    bytes.
     */
    public void setTitle(String songTitle) throws IllegalArgumentException {
        Utils.checkStringLengthNullSafe(songTitle);
        this.title = songTitle;
    }
}