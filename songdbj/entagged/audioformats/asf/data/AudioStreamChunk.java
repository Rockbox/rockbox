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
 * This class represents the streamchunk describing an audio stream. <br>
 * 
 * @author Christian Laireiter
 */
public class AudioStreamChunk extends StreamChunk {

    /**
     * Stores the hex values of codec identifiers to their descriptions. <br>
     */
    public final static String[][] CODEC_DESCRIPTIONS = {
            { "161", " (Windows Media Audio (ver 7,8,9))" },
            { "162", " (Windows Media Audio 9 series (Professional))" },
            { "163", "(Windows Media Audio 9 series (Lossless))" },
            { "7A21", " (GSM-AMR (CBR))" }, { "7A22", " (GSM-AMR (VBR))" } };

    /**
     * Stores the average amount of bytes used by audio stream. <br>
     * This value is a field within type specific data of audio stream. Maybe it
     * could be used to calculate the kbps.
     */
    private long averageBytesPerSec;

    /**
     * Amount of bits used per sample. <br>
     */
    private int bitsPerSample;

    /**
     * The block alignment of the audio data.
     */
    private long blockAlignment;

    /**
     * Number of channels.
     */
    private long channelCount;

    /**
     * Some data which needs to be interpreted if the codec is handled.
     */
    private byte[] codecData;

    /**
     * The audio compression format code.
     */
    private long compressionFormat;

    /**
     * this field stores the error concealment type.
     */
    private GUID errorConcealment;

    /**
     * Sampling rate of audio stream.
     */
    private long samplingRate;

    /**
     * Creates an instance.
     * 
     * @param pos
     *                   Position of current chunk within asf file or stream.
     * @param chunkLen
     *                   Length of the entire chunk (including guid and size)
     */
    public AudioStreamChunk(long pos, BigInteger chunkLen) {
        super(pos, chunkLen);
    }

    /**
     * @return Returns the averageBytesPerSec.
     */
    public long getAverageBytesPerSec() {
        return averageBytesPerSec;
    }

    /**
     * @return Returns the bitsPerSample.
     */
    public int getBitsPerSample() {
        return bitsPerSample;
    }

    /**
     * @return Returns the blockAlignment.
     */
    public long getBlockAlignment() {
        return blockAlignment;
    }

    /**
     * @return Returns the channelCount.
     */
    public long getChannelCount() {
        return channelCount;
    }

    /**
     * @return Returns the codecData.
     */
    public byte[] getCodecData() {
        return codecData;
    }

    /**
     * This method will take a look at {@link #compressionFormat}and returns a
     * String with its hex value and if known a textual note on what coded it
     * represents. <br>
     * 
     * @return A description for the used codec.
     */
    public String getCodecDescription() {
        StringBuffer result = new StringBuffer(Long
                .toHexString(getCompressionFormat()));
        String furtherDesc = " (Unknown)";
        for (int i = 0; i < CODEC_DESCRIPTIONS.length; i++) {
            if (CODEC_DESCRIPTIONS[i][0].equalsIgnoreCase(result.toString())) {
                furtherDesc = CODEC_DESCRIPTIONS[i][1];
                break;
            }
        }
        if (result.length() % 2 != 0) {
            result.insert(0, "0x0");
        } else {
            result.insert(0, "0x");
        }
        result.append(furtherDesc);
        return result.toString();
    }

    /**
     * @return Returns the compressionFormat.
     */
    public long getCompressionFormat() {
        return compressionFormat;
    }

    /**
     * @return Returns the errorConcealment.
     */
    public GUID getErrorConcealment() {
        return errorConcealment;
    }

    /**
     * This method takes the value of {@link #getAverageBytesPerSec()}and
     * calculates the kbps out of it, by simply multiplying by 8 and dividing by
     * 1000. <br>
     * 
     * @return amount of bits per second in kilo bits.
     */
    public int getKbps() {
        return (int) getAverageBytesPerSec() * 8 / 1000;
    }

    /**
     * @return Returns the samplingRate.
     */
    public long getSamplingRate() {
        return samplingRate;
    }

    /**
     * This mehtod returns whether the audio stream data is error concealed.
     * <br>
     * For now only interleaved concealment is known. <br>
     * 
     * @return <code>true</code> if error concealment is used.
     */
    public boolean isErrorConcealed() {
        return getErrorConcealment().equals(
                GUID.GUID_AUDIO_ERROR_CONCEALEMENT_INTERLEAVED);
    }

    /**
     * (overridden)
     * 
     * @see entagged.audioformats.asf.data.StreamChunk#prettyPrint()
     */
    public String prettyPrint() {
        StringBuffer result = new StringBuffer(super.prettyPrint().replaceAll(
                Utils.LINE_SEPARATOR, Utils.LINE_SEPARATOR + "   "));
        result.insert(0, Utils.LINE_SEPARATOR + "AudioStream");
        result.append("Audio info:" + Utils.LINE_SEPARATOR);
        result.append("      Bitrate : " + getKbps() + Utils.LINE_SEPARATOR);
        result.append("      Channels : " + getChannelCount() + " at "
                + getSamplingRate() + " Hz" + Utils.LINE_SEPARATOR);
        result.append("      Bits per Sample: " + getBitsPerSample()
                + Utils.LINE_SEPARATOR);
        result.append("      Formatcode: " + getCodecDescription()
                + Utils.LINE_SEPARATOR);
        return result.toString();
    }

    /**
     * @param avgeBytesPerSec
     *                   The averageBytesPerSec to set.
     */
    public void setAverageBytesPerSec(long avgeBytesPerSec) {
        this.averageBytesPerSec = avgeBytesPerSec;
    }

    /**
     * Sets the bitsPerSample
     * 
     * @param bps
     */
    public void setBitsPerSample(int bps) {
        this.bitsPerSample = bps;
    }

    /**
     * Sets the blockAlignment.
     * 
     * @param align
     */
    public void setBlockAlignment(long align) {
        this.blockAlignment = align;
    }

    /**
     * @param channels
     *                   The channelCount to set.
     */
    public void setChannelCount(long channels) {
        this.channelCount = channels;
    }

    /**
     * Sets the codecData
     * 
     * @param codecSpecificData
     */
    public void setCodecData(byte[] codecSpecificData) {
        this.codecData = codecSpecificData;
    }

    /**
     * @param cFormatCode
     *                   The compressionFormat to set.
     */
    public void setCompressionFormat(long cFormatCode) {
        this.compressionFormat = cFormatCode;
    }

    /**
     * This method sets the error concealment type which is given by two GUIDs.
     * <br>
     * 
     * @param errConc
     *                   the type of error concealment the audio stream is stored as.
     */
    public void setErrorConcealment(GUID errConc) {
        this.errorConcealment = errConc;
    }

    /**
     * @param sampRate
     *                   The samplingRate to set.
     */
    public void setSamplingRate(long sampRate) {
        this.samplingRate = sampRate;
    }
}