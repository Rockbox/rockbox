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
 * 
 * 
 * @author Christian Laireiter
 */
public class VideoStreamChunk extends StreamChunk {

	private long pictureHeight;

	/**
	 * This field stores the width of the video stream.
	 */
	private long pictureWidth;

	/**
	 * Creates an instance.
	 * 
	 * @param pos
	 *                  Position of the current chunk in the asf file or stream.
	 * @param chunkLen
	 *                  Length of the entire chunk (including guid and size)
	 */
	public VideoStreamChunk(long pos, BigInteger chunkLen) {
		super(pos, chunkLen);
	}

	/**
	 * @return Returns the pictureHeight.
	 */
	public long getPictureHeight() {
		return pictureHeight;
	}

	/**
	 * @return Returns the pictureWidth.
	 */
	public long getPictureWidth() {
		return pictureWidth;
	}

	/**
	 * (overridden)
	 * 
	 * @see entagged.audioformats.asf.data.StreamChunk#prettyPrint()
	 */
	public String prettyPrint() {
		StringBuffer result = new StringBuffer(super.prettyPrint().replaceAll(
				Utils.LINE_SEPARATOR, Utils.LINE_SEPARATOR + "   "));
		result.insert(0, Utils.LINE_SEPARATOR + "VideoStream");
		result.append("Video info:" + Utils.LINE_SEPARATOR);
		result.append("      Width  : " + getPictureWidth()
				+ Utils.LINE_SEPARATOR);
		result.append("      Heigth : " + getPictureHeight()
				+ Utils.LINE_SEPARATOR);
		return result.toString();
	}

	/**
	 * @param picHeight
	 */
	public void setPictureHeight(long picHeight) {
		this.pictureHeight = picHeight;
	}

	/**
	 * @param picWidth
	 */
	public void setPictureWidth(long picWidth) {
		this.pictureWidth = picWidth;
	}
}