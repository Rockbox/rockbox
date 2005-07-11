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
import java.util.Date;

import entagged.audioformats.asf.util.Utils;

/**
 * This class stores the information about the file, which is contained within a
 * special chunk of asf files.<br>
 * 
 * @author Christian Laireiter
 */
public class FileHeader extends Chunk {

	/**
	 * Duration of the media content in 100ns steps.
	 */
	private final BigInteger duration;

	/**
	 * The time the file was created.
	 */
	private final Date fileCreationTime;

	/**
	 * Size of the file or stream.
	 */
	private BigInteger fileSize;

	/**
	 * Usually contains value of 2.
	 */
	private final long flags;

	/**
	 * Maximum size of stream packages. <br>
	 * <b>Warning: </b> must be same size as {@link #minPackageSize}. Its not
	 * known how to handle deviating values.
	 */
	private final long maxPackageSize;

	/**
	 * Minimun size of stream packages. <br>
	 * <b>Warning: </b> must be same size as {@link #maxPackageSize}. Its not
	 * known how to handle deviating values.
	 */
	private final long minPackageSize;

	/**
	 * Number of stream packages within the File.
	 */
	private final BigInteger packageCount;

	/**
	 * No Idea of the Meaning, but stored anyway. <br>
	 * Source documentation says it is: "Timestamp of end position"
	 */
	private final BigInteger timeEndPos;

	/**
	 * Like {@link #timeEndPos}no Idea.
	 */
	private final BigInteger timeStartPos;

	/**
	 * Size of an uncompressed video frame.
	 */
	private final long uncompressedFrameSize;

	/**
	 * Creates an instance.
	 * 
	 * @param fileHeaderStart
	 *                  Position in file or stream, where the file header starts.
	 * @param chunckLen
	 *                  Length of the file header (chunk)
	 * @param size
	 *                  Size of file or stream
	 * @param fileTime
	 *                  Time file or stream was created. Time is calculated since 1st
	 *                  january of 1601 in 100ns steps.
	 * @param pkgCount
	 *                  Number of stream packages.
	 * @param dur
	 *                  Duration of media clip in 100ns steps
	 * @param timestampStart
	 *                  Timestamp of start {@link #timeStartPos}
	 * @param timestampEnd
	 *                  Timestamp of end {@link #timeEndPos}
	 * @param headerFlags
	 *                  some stream related flags.
	 * @param minPkgSize
	 *                  minimun size of packages
	 * @param maxPkgSize
	 *                  maximum size of packages
	 * @param uncmpVideoFrameSize
	 *                  Size of an uncompressed Video Frame.
	 */
	public FileHeader(long fileHeaderStart, BigInteger chunckLen,
			BigInteger size, BigInteger fileTime, BigInteger pkgCount,
			BigInteger dur, BigInteger timestampStart, BigInteger timestampEnd,
			long headerFlags, long minPkgSize, long maxPkgSize,
			long uncmpVideoFrameSize) {
		super(GUID.GUID_FILE, fileHeaderStart, chunckLen);
		this.fileSize = size;
		this.packageCount = pkgCount;
		this.duration = dur;
		this.timeStartPos = timestampStart;
		this.timeEndPos = timestampEnd;
		this.flags = headerFlags;
		this.minPackageSize = minPkgSize;
		this.maxPackageSize = maxPkgSize;
		this.uncompressedFrameSize = uncmpVideoFrameSize;
		this.fileCreationTime = Utils.getDateOf(fileTime).getTime();
	}

	/**
	 * @return Returns the duration.
	 */
	public BigInteger getDuration() {
		return duration;
	}

	/**
	 * This method converts {@link #getDuration()}from 100ns steps to normal
	 * seconds.
	 * 
	 * @return Duration of the media in seconds.
	 */
	public int getDurationInSeconds() {
		return duration.divide(new BigInteger("10000000")).intValue();
	}

	/**
	 * @return Returns the fileCreationTime.
	 */
	public Date getFileCreationTime() {
		return fileCreationTime;
	}

	/**
	 * @return Returns the fileSize.
	 */
	public BigInteger getFileSize() {
		return fileSize;
	}

	/**
	 * @return Returns the flags.
	 */
	public long getFlags() {
		return flags;
	}

	/**
	 * @return Returns the maxPackageSize.
	 */
	public long getMaxPackageSize() {
		return maxPackageSize;
	}

	/**
	 * @return Returns the minPackageSize.
	 */
	public long getMinPackageSize() {
		return minPackageSize;
	}

	/**
	 * @return Returns the packageCount.
	 */
	public BigInteger getPackageCount() {
		return packageCount;
	}

	/**
	 * @return Returns the timeEndPos.
	 */
	public BigInteger getTimeEndPos() {
		return timeEndPos;
	}

	/**
	 * @return Returns the timeStartPos.
	 */
	public BigInteger getTimeStartPos() {
		return timeStartPos;
	}

	/**
	 * @return Returns the uncompressedFrameSize.
	 */
	public long getUncompressedFrameSize() {
		return uncompressedFrameSize;
	}

	/**
	 * (overridden)
	 * 
	 * @see entagged.audioformats.asf.data.Chunk#prettyPrint()
	 */
	public String prettyPrint() {
		StringBuffer result = new StringBuffer(super.prettyPrint());
		result.insert(0, "\nFileHeader\n");
		result.append("   Filesize      = " + getFileSize().toString()
				+ " Bytes \n");
		result.append("   Media duration= "
				+ getDuration().divide(new BigInteger("10000")).toString()
				+ " ms \n");
		result.append("   Created at    = " + getFileCreationTime() + "\n");
		return result.toString();
	}
}