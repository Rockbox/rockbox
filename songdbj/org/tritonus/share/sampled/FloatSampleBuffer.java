/*
 * FloatSampleBuffer.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000,2004 by Florian Bomers <http://www.bomers.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as published
 *   by the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
|<---            this code is formatted to fit into 80 columns             --->|
*/

package org.tritonus.share.sampled;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Random;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.spi.AudioFileWriter;

import org.tritonus.share.TDebug;

/**
 * A class for small buffers of samples in linear, 32-bit
 * floating point format.
 * <p>
 * It is supposed to be a replacement of the byte[] stream
 * architecture of JavaSound, especially for chains of
 * AudioInputStreams. Ideally, all involved AudioInputStreams
 * handle reading into a FloatSampleBuffer.
 * <p>
 * Specifications:
 * <ol>
 * <li>Channels are separated, i.e. for stereo there are 2 float arrays
 *     with the samples for the left and right channel
 * <li>All data is handled in samples, where one sample means
 *     one float value in each channel
 * <li>All samples are normalized to the interval [-1.0...1.0]
 * </ol>
 * <p>
 * When a cascade of AudioInputStreams use FloatSampleBuffer for
 * processing, they may implement the interface FloatSampleInput.
 * This signals that this stream may provide float buffers
 * for reading. The data is <i>not</i> converted back to bytes,
 * but stays in a single buffer that is passed from stream to stream.
 * For that serves the read(FloatSampleBuffer) method, which is
 * then used as replacement for the byte-based read functions of
 * AudioInputStream.<br>
 * However, backwards compatibility must always be retained, so
 * even when an AudioInputStream implements FloatSampleInput,
 * it must work the same way when any of the byte-based read methods
 * is called.<br>
 * As an example, consider the following set-up:<br>
 * <ul>
 * <li>auAIS is an AudioInputStream (AIS) that reads from an AU file
 *     in 8bit pcm at 8000Hz. It does not implement FloatSampleInput.
 * <li>pcmAIS1 is an AIS that reads from auAIS and converts the data
 *     to PCM 16bit. This stream implements FloatSampleInput, i.e. it
 *     can generate float audio data from the ulaw samples.
 * <li>pcmAIS2 reads from pcmAIS1 and adds a reverb.
 *     It operates entirely on floating point samples.
 * <li>The method that reads from pcmAIS2 (i.e. AudioSystem.write) does
 *     not handle floating point samples.
 * </ul>
 * So, what happens when a block of samples is read from pcmAIS2 ?
 * <ol>
 * <li>the read(byte[]) method of pcmAIS2 is called
 * <li>pcmAIS2 always operates on floating point samples, so
 *     it uses an own instance of FloatSampleBuffer and initializes
 *     it with the number of samples requested in the read(byte[])
 *     method.
 * <li>It queries pcmAIS1 for the FloatSampleInput interface. As it
 *     implements it, pcmAIS2 calls the read(FloatSampleBuffer) method
 *     of pcmAIS1.
 * <li>pcmAIS1 notes that its underlying stream does not support floats,
 *     so it instantiates a byte buffer which can hold the number of
 *     samples of the FloatSampleBuffer passed to it. It calls the
 *     read(byte[]) method of auAIS.
 * <li>auAIS fills the buffer with the bytes.
 * <li>pcmAIS1 calls the <code>initFromByteArray</code> method of
 *     the float buffer to initialize it with the 8 bit data.
 * <li>Then pcmAIS1 processes the data: as the float buffer is
 *     normalized, it does nothing with the buffer - and returns
 *     control to pcmAIS2. The SampleSizeInBits field of the
 *     AudioFormat of pcmAIS1 defines that it should be 16 bits.
 * <li>pcmAIS2 receives the filled buffer from pcmAIS1 and does
 *     its processing on the buffer - it adds the reverb.
 * <li>As pcmAIS2's read(byte[]) method had been called, pcmAIS2
 *     calls the <code>convertToByteArray</code> method of
 *     the float buffer to fill the byte buffer with the
 *     resulting samples.
 * </ol>
 * <p>
 * To summarize, here are some advantages when using a FloatSampleBuffer
 * for streaming:
 * <ul>
 * <li>no conversions from/to bytes need to be done during processing
 * <li>the sample size in bits is irrelevant - normalized range
 * <li>higher quality for processing
 * <li>separated channels (easy process/remove/add channels)
 * <li>potentially less copying of audio data, as processing
 * the float samples is generally done in-place. The same
 * instance of a FloatSampleBuffer may be used from the original data source
 * to the final data sink.
 * </ul>
 * <p>
 * Simple benchmarks showed that the processing requirements
 * for the conversion to and from float is about the same as
 * when converting it to shorts or ints without dithering,
 * and significantly higher with dithering. An own implementation
 * of a random number generator may improve this.
 * <p>
 * &quot;Lazy&quot; deletion of samples and channels:<br>
 * <ul>
 * <li>When the sample count is reduced, the arrays are not resized, but
 * only the member variable <code>sampleCount</code> is reduced. A subsequent
 * increase of the sample count (which will occur frequently), will check
 * that and eventually reuse the existing array.
 * <li>When a channel is deleted, it is not removed from memory but only
 * hidden. Subsequent insertions of a channel will check whether a hidden channel
 * can be reused.
 * </ul>
 * The lazy mechanism can save many array instantiation (and copy-) operations
 * for the sake of performance. All relevant methods exist in a second
 * version which allows explicitely to disable lazy deletion.
 * <p>
 * Use the <code>reset</code> functions to clear the memory and remove
 * hidden samples and channels.
 * <p>
 * Note that the lazy mechanism implies that the arrays returned
 * from <code>getChannel(int)</code> may have a greater size
 * than getSampleCount(). Consequently, be sure to never rely on the
 * length field of the sample arrays.
 * <p>
 * As an example, consider a chain of converters that all act
 * on the same instance of FloatSampleBuffer. Some converters
 * may decrease the sample count (e.g. sample rate converter) and
 * delete channels (e.g. PCM2PCM converter). So, processing of one
 * block will decrease both. For the next block, all starts
 * from the beginning. With the lazy mechanism, all float arrays
 * are only created once for processing all blocks.<br>
 * Having lazy disabled would require for each chunk that is processed
 * <ol>
 * <li>new instantiation of all channel arrays
 * at the converter chain beginning as they have been
 * either deleted or decreased in size during processing of the
 * previous chunk, and
 * <li>re-instantiation of all channel arrays for
 * the reduction of the sample count.
 * </ol>
 * <p>
 * Dithering:<br>
 * By default, this class uses dithering for reduction
 * of sample width (e.g. original data was 16bit, target
 * data is 8bit). As dithering may be needed in other cases
 * (especially when the float samples are processed using DSP
 * algorithms), or it is preferred to switch it off,
 * dithering can be explicitely switched on or off with
 * the method setDitherMode(int).<br>
 * For a discussion about dithering, see
 * <a href="http://www.iqsoft.com/IQSMagazine/BobsSoapbox/Dithering.htm">
 * here</a> and
 * <a href="http://www.iqsoft.com/IQSMagazine/BobsSoapbox/Dithering2.htm">
 * here</a>.
 *
 * @author Florian Bomers
 */

public class FloatSampleBuffer {

	/** Whether the functions without lazy parameter are lazy or not. */
	private static final boolean LAZY_DEFAULT=true;

	private ArrayList<float[]> channels = new ArrayList<float[]>(); // contains for each channel a float array
	private int sampleCount=0;
	private int channelCount=0;
	private float sampleRate=0;
	private int originalFormatType=0;

	/** Constant for setDitherMode: dithering will be enabled if sample size is decreased */
	public static final int DITHER_MODE_AUTOMATIC=0;
	/** Constant for setDitherMode: dithering will be done */
	public static final int DITHER_MODE_ON=1;
	/** Constant for setDitherMode: dithering will not be done */
	public static final int DITHER_MODE_OFF=2;

	private float ditherBits = FloatSampleTools.DEFAULT_DITHER_BITS;

	// e.g. the sample rate converter may want to force dithering
	private int ditherMode = DITHER_MODE_AUTOMATIC;

	//////////////////////////////// initialization /////////////////////////////////

	/**
	 * Create an instance with initially no channels.
	 */
	public FloatSampleBuffer() {
		this(0,0,1);
	}

	/**
	 * Create an empty FloatSampleBuffer with the specified number of channels,
	 * samples, and the specified sample rate.
	 */
	public FloatSampleBuffer(int channelCount, int sampleCount, float sampleRate) {
		init(channelCount, sampleCount, sampleRate, LAZY_DEFAULT);
	}

	/**
	 * Creates a new instance of FloatSampleBuffer and initializes
	 * it with audio data given in the interleaved byte array <code>buffer</code>.
	 */
	public FloatSampleBuffer(byte[] buffer, int offset, int byteCount,
	                         AudioFormat format) {
		this(format.getChannels(),
		     byteCount/(format.getSampleSizeInBits()/8*format.getChannels()),
		     format.getSampleRate());
		initFromByteArray(buffer, offset, byteCount, format);
	}

	protected void init(int channelCount, int sampleCount, float sampleRate) {
		init(channelCount, sampleCount, sampleRate, LAZY_DEFAULT);
	}

	protected void init(int channelCount, int sampleCount, float sampleRate, boolean lazy) {
		if (channelCount<0 || sampleCount<0) {
			throw new IllegalArgumentException(
			    "invalid parameters in initialization of FloatSampleBuffer.");
		}
		setSampleRate(sampleRate);
		if (getSampleCount()!=sampleCount || getChannelCount()!=channelCount) {
			createChannels(channelCount, sampleCount, lazy);
		}
	}

	private void createChannels(int channelCount, int sampleCount, boolean lazy) {
		this.sampleCount=sampleCount;
		// lazy delete of all channels. Intentionally lazy !
		this.channelCount=0;
		for (int ch=0; ch<channelCount; ch++) {
			insertChannel(ch, false, lazy);
		}
		if (!lazy) {
			// remove hidden channels
			while (channels.size()>channelCount) {
				channels.remove(channels.size()-1);
			}
		}
	}


	/**
	 * Resets this buffer with the audio data specified
	 * in the arguments. This FloatSampleBuffer's sample count
	 * will be set to <code>byteCount / format.getFrameSize()</code>.
	 * If LAZY_DEFAULT is true, it will use lazy deletion.
	 *
	 * @throws IllegalArgumentException
	 */
	public void initFromByteArray(byte[] buffer, int offset, int byteCount,
	                              AudioFormat format) {
		initFromByteArray(buffer, offset, byteCount, format, LAZY_DEFAULT);
	}


	/**
	 * Resets this buffer with the audio data specified
	 * in the arguments. This FloatSampleBuffer's sample count
	 * will be set to <code>byteCount / format.getFrameSize()</code>.
	 *
	 * @param lazy if true, then existing channels will be tried to be re-used
	 *        to minimize garbage collection.
	 * @throws IllegalArgumentException
	 */
	public void initFromByteArray(byte[] buffer, int offset, int byteCount,
	                              AudioFormat format, boolean lazy) {
		if (offset+byteCount>buffer.length) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.initFromByteArray: buffer too small.");
		}

		int thisSampleCount = byteCount/format.getFrameSize();
		init(format.getChannels(), thisSampleCount, format.getSampleRate(), lazy);

		// save format for automatic dithering mode
		originalFormatType = FloatSampleTools.getFormatType(format);

		FloatSampleTools.byte2float(buffer, offset,
		                            channels, 0, sampleCount, format);
	}

	/**
	 * Resets this sample buffer with the data in <code>source</code>.
	 */
	public void initFromFloatSampleBuffer(FloatSampleBuffer source) {
		init(source.getChannelCount(), source.getSampleCount(), source.getSampleRate());
		for (int ch=0; ch<getChannelCount(); ch++) {
			System.arraycopy(source.getChannel(ch), 0, getChannel(ch), 0, sampleCount);
		}
	}

	/**
	 * Deletes all channels, frees memory...
	 * This also removes hidden channels by lazy remove.
	 */
	public void reset() {
		init(0,0,1, false);
	}

	/**
	 * Destroys any existing data and creates new channels.
	 * It also destroys lazy removed channels and samples.
	 */
	public void reset(int channels, int sampleCount, float sampleRate) {
		init(channels, sampleCount, sampleRate, false);
	}

	//////////////////////////////// conversion back to bytes /////////////////////////////////

	/**
	 * @return the required size of the buffer
	 *         for calling convertToByteArray(..) is called
	 */
	public int getByteArrayBufferSize(AudioFormat format) {
		// make sure this format is supported
		FloatSampleTools.getFormatType(format);
		return format.getFrameSize() * getSampleCount();
	}

	/**
	 * Writes this sample buffer's audio data to <code>buffer</code>
	 * as an interleaved byte array.
	 * <code>buffer</code> must be large enough to hold all data.
	 *
	 * @throws IllegalArgumentException when buffer is too small or <code>format</code> doesn't match
	 * @return number of bytes written to <code>buffer</code>
	 */
	public int convertToByteArray(byte[] buffer, int offset, AudioFormat format) {
		int byteCount = getByteArrayBufferSize(format);
		if (offset + byteCount > buffer.length) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.convertToByteArray: buffer too small.");
		}
		if (format.getSampleRate()!=getSampleRate()) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.convertToByteArray: different samplerates.");
		}
		if (format.getChannels()!=getChannelCount()) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.convertToByteArray: different channel count.");
		}
		FloatSampleTools.float2byte(channels, 0, buffer, offset, getSampleCount(),
		                            format, getConvertDitherBits(FloatSampleTools.getFormatType(format)));

		return byteCount;
	}


	/**
	 * Creates a new byte[] buffer, fills it with the audio data, and returns it.
	 * @throws IllegalArgumentException when sample rate or channels do not match
	 * @see #convertToByteArray(byte[], int, AudioFormat)
	 */
	public byte[] convertToByteArray(AudioFormat format) {
		// throws exception when sampleRate doesn't match
		// creates a new byte[] buffer and returns it
		byte[] res = new byte[getByteArrayBufferSize(format)];
		convertToByteArray(res, 0, format);
		return res;
	}

	//////////////////////////////// actions /////////////////////////////////

	/**
	 * Resizes this buffer.
	 * <p>If <code>keepOldSamples</code> is true, as much as possible samples are
	 * retained. If the buffer is enlarged, silence is added at the end.
	 * If <code>keepOldSamples</code> is false, existing samples are discarded
	 * and the buffer contains random samples.
	 */
	public void changeSampleCount(int newSampleCount, boolean keepOldSamples) {
		int oldSampleCount=getSampleCount();
		if (oldSampleCount==newSampleCount) {
			return;
		}
		Object[] oldChannels=null;
		if (keepOldSamples) {
			oldChannels=getAllChannels();
		}
		init(getChannelCount(), newSampleCount, getSampleRate());
		if (keepOldSamples) {
			// copy old channels and eventually silence out new samples
			int copyCount=newSampleCount<oldSampleCount?
			              newSampleCount:oldSampleCount;
			for (int ch=0; ch<getChannelCount(); ch++) {
				float[] oldSamples=(float[]) oldChannels[ch];
				float[] newSamples=(float[]) getChannel(ch);
				if (oldSamples!=newSamples) {
					// if this sample array was not object of lazy delete
					System.arraycopy(oldSamples, 0, newSamples, 0, copyCount);
				}
				if (oldSampleCount<newSampleCount) {
					// silence out new samples
					for (int i=oldSampleCount; i<newSampleCount; i++) {
						newSamples[i]=0.0f;
					}
				}
			}
		}
	}

	public void makeSilence() {
		// silence all channels
		if (getChannelCount()>0) {
			makeSilence(0);
			for (int ch=1; ch<getChannelCount(); ch++) {
				copyChannel(0, ch);
			}
		}
	}

	public void makeSilence(int channel) {
		float[] samples=getChannel(channel);
		for (int i=0; i<getSampleCount(); i++) {
			samples[i]=0.0f;
		}
	}

	public void addChannel(boolean silent) {
		// creates new, silent channel
		insertChannel(getChannelCount(), silent);
	}

	/**
	 * Insert a (silent) channel at position <code>index</code>.
	 * If LAZY_DEFAULT is true, this is done lazily.
	 */
	public void insertChannel(int index, boolean silent) {
		insertChannel(index, silent, LAZY_DEFAULT);
	}

	/**
	 * Inserts a channel at position <code>index</code>.
	 * <p>If <code>silent</code> is true, the new channel will be silent.
	 * Otherwise it will contain random data.
	 * <p>If <code>lazy</code> is true, hidden channels which have at least getSampleCount()
	 * elements will be examined for reusage as inserted channel.<br>
	 * If <code>lazy</code> is false, still hidden channels are reused,
	 * but it is assured that the inserted channel has exactly getSampleCount() elements,
	 * thus not wasting memory.
	 */
	public void insertChannel(int index, boolean silent, boolean lazy) {
		int physSize=channels.size();
		int virtSize=getChannelCount();
		float[] newChannel=null;
		if (physSize>virtSize) {
			// there are hidden channels. Try to use one.
			for (int ch=virtSize; ch<physSize; ch++) {
				float[] thisChannel=(float[]) channels.get(ch);
				if ((lazy && thisChannel.length>=getSampleCount())
				        || (!lazy && thisChannel.length==getSampleCount())) {
					// we found a matching channel. Use it !
					newChannel=thisChannel;
					channels.remove(ch);
					break;
				}
			}
		}
		if (newChannel==null) {
			newChannel=new float[getSampleCount()];
		}
		channels.add(index, newChannel);
		this.channelCount++;
		if (silent) {
			makeSilence(index);
		}
	}

	/** performs a lazy remove of the channel */
	public void removeChannel(int channel) {
		removeChannel(channel, LAZY_DEFAULT);
	}


	/**
	 * Removes a channel.
	 * If lazy is true, the channel is not physically removed, but only hidden.
	 * These hidden channels are reused by subsequent calls to addChannel
	 * or insertChannel.
	 */
	public void removeChannel(int channel, boolean lazy) {
		if (!lazy) {
			channels.remove(channel);
		} else if (channel<getChannelCount()-1) {
			// if not already, move this channel at the end
			channels.add(channels.remove(channel));
		}
		channelCount--;
	}

	/**
	 * both source and target channel have to exist. targetChannel
	 * will be overwritten
	 */
	public void copyChannel(int sourceChannel, int targetChannel) {
		float[] source=getChannel(sourceChannel);
		float[] target=getChannel(targetChannel);
		System.arraycopy(source, 0, target, 0, getSampleCount());
	}

	/**
	 * Copies data inside all channel. When the 2 regions
	 * overlap, the behavior is not specified.
	 */
	public void copy(int sourceIndex, int destIndex, int length) {
		for (int i=0; i<getChannelCount(); i++) {
			copy(i, sourceIndex, destIndex, length);
		}
	}

	/**
	 * Copies data inside a channel. When the 2 regions
	 * overlap, the behavior is not specified.
	 */
	public void copy(int channel, int sourceIndex, int destIndex, int length) {
		float[] data=getChannel(channel);
		int bufferCount=getSampleCount();
		if (sourceIndex+length>bufferCount || destIndex+length>bufferCount
			|| sourceIndex<0 || destIndex<0 || length<0) {
				throw new IndexOutOfBoundsException("parameters exceed buffer size");
		}
		System.arraycopy(data, sourceIndex, data, destIndex, length);
	}

	/**
	 * Mix up of 1 channel to n channels.<br>
	 * It copies the first channel to all newly created channels.
	 * @param targetChannelCount the number of channels that this sample buffer
	 *                        will have after expanding. NOT the number of
	 *                        channels to add !
	 * @exception IllegalArgumentException if this buffer does not have one
	 *            channel before calling this method.
	 */
	public void expandChannel(int targetChannelCount) {
		// even more sanity...
		if (getChannelCount()!=1) {
			throw new IllegalArgumentException(
			    "FloatSampleBuffer: can only expand channels for mono signals.");
		}
		for (int ch=1; ch<targetChannelCount; ch++) {
			addChannel(false);
			copyChannel(0, ch);
		}
	}

	/**
	 * Mix down of n channels to one channel.<br>
	 * It uses a simple mixdown: all other channels are added to first channel.<br>
	 * The volume is NOT lowered !
	 * Be aware, this might cause clipping when converting back
	 * to integer samples.
	 */
	public void mixDownChannels() {
		float[] firstChannel=getChannel(0);
		int sampleCount=getSampleCount();
		int channelCount=getChannelCount();
		for (int ch=channelCount-1; ch>0; ch--) {
			float[] thisChannel=getChannel(ch);
			for (int i=0; i<sampleCount; i++) {
				firstChannel[i]+=thisChannel[i];
			}
			removeChannel(ch);
		}
	}

	/**
	 * Initializes audio data from the provided byte array.
	 * The float samples are written at <code>destOffset</code>.
	 * This FloatSampleBuffer must be big enough to accomodate the samples.
	 * <p>
	 * <code>srcBuffer</code> is read from index <code>srcOffset</code>
	 * to <code>(srcOffset + (lengthInSamples * format.getFrameSize()))</code.
	 *
	 * @param input the input buffer in interleaved audio data
	 * @param inByteOffset the offset in <code>input</code>
	 * @param format input buffer's audio format
	 * @param floatOffset the offset where to write the float samples
	 * @param frameCount number of samples to write to this sample buffer
	 */
	public void setSamplesFromBytes(byte[] input, int inByteOffset, AudioFormat format,
	                                int floatOffset, int frameCount) {
		if (floatOffset < 0 || frameCount < 0 || inByteOffset < 0) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.setSamplesFromBytes: negative inByteOffset, floatOffset, or frameCount");
		}
		if (inByteOffset + (frameCount * format.getFrameSize()) > input.length) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.setSamplesFromBytes: input buffer too small.");
		}
		if (floatOffset + frameCount > getSampleCount()) {
			throw new IllegalArgumentException
			("FloatSampleBuffer.setSamplesFromBytes: frameCount too large");
		}

		FloatSampleTools.byte2float(input, inByteOffset, channels, floatOffset, frameCount, format);
	}

	//////////////////////////////// properties /////////////////////////////////

	public int getChannelCount() {
		return channelCount;
	}

	public int getSampleCount() {
		return sampleCount;
	}

	public float getSampleRate() {
		return sampleRate;
	}

	/**
	 * Sets the sample rate of this buffer.
	 * NOTE: no conversion is done. The samples are only re-interpreted.
	 */
	public void setSampleRate(float sampleRate) {
		if (sampleRate<=0) {
			throw new IllegalArgumentException
			("Invalid samplerate for FloatSampleBuffer.");
		}
		this.sampleRate=sampleRate;
	}

	/**
	 * NOTE: the returned array may be larger than sampleCount. So in any case,
	 * sampleCount is to be respected.
	 */
	public float[] getChannel(int channel) {
		if (channel<0 || channel>=getChannelCount()) {
			throw new IllegalArgumentException(
			    "FloatSampleBuffer: invalid channel number.");
		}
		return (float[]) channels.get(channel);
	}

	public Object[] getAllChannels() {
		Object[] res=new Object[getChannelCount()];
		for (int ch=0; ch<getChannelCount(); ch++) {
			res[ch]=getChannel(ch);
		}
		return res;
	}

	/**
	 * Set the number of bits for dithering.
	 * Typically, a value between 0.2 and 0.9 gives best results.
	 * <p>Note: this value is only used, when dithering is actually performed.
	 */
	public void setDitherBits(float ditherBits) {
		if (ditherBits<=0) {
			throw new IllegalArgumentException("DitherBits must be greater than 0");
		}
		this.ditherBits=ditherBits;
	}

	public float getDitherBits() {
		return ditherBits;
	}

	/**
	 * Sets the mode for dithering.
	 * This can be one of:
	 * <ul><li>DITHER_MODE_AUTOMATIC: it is decided automatically,
	 * whether dithering is necessary - in general when sample size is
	 * decreased.
	 * <li>DITHER_MODE_ON: dithering will be forced
	 * <li>DITHER_MODE_OFF: dithering will not be done.
	 * </ul>
	 */
	public void setDitherMode(int mode) {
		if (mode!=DITHER_MODE_AUTOMATIC
		        && mode!=DITHER_MODE_ON
		        && mode!=DITHER_MODE_OFF) {
			throw new IllegalArgumentException("Illegal DitherMode");
		}
		this.ditherMode=mode;
	}

	public int getDitherMode() {
		return ditherMode;
	}


	/**
	 * @return the ditherBits parameter for the float2byte functions
	 */
	protected float getConvertDitherBits(int newFormatType) {
		// let's see whether dithering is necessary
		boolean doDither = false;
		switch (ditherMode) {
		case DITHER_MODE_AUTOMATIC:
			doDither=(originalFormatType & FloatSampleTools.F_SAMPLE_WIDTH_MASK)>
			         (newFormatType & FloatSampleTools.F_SAMPLE_WIDTH_MASK);
			break;
		case DITHER_MODE_ON:
			doDither=true;
			break;
		case DITHER_MODE_OFF:
			doDither=false;
			break;
		}
		return doDither?ditherBits:0.0f;
	}
}
