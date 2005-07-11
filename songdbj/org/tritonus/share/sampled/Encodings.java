/*
 *	Encodings.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 2000 by Florian Bomers <http://www.bomers.de>
 *
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
 *
 */


package org.tritonus.share.sampled;

import java.util.Iterator;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.AudioFormat;
import org.tritonus.share.StringHashedSet;
import org.tritonus.share.TDebug;

/**
 * This class is a proposal for generic handling of encodings.
 * The main purpose is to provide a standardized way of
 * implementing encoding types. Like this, encodings
 * are only identified by their String name, and not, as currently,
 * by their object instance.
 * <p>
 * A registry of standard encoding names will
 * be maintained by the Tritonus team.
 * <p>
 * In a specification request to JavaSoft, the static method
 * <code>getEncoding</code> should be integrated into
 * <code>AudioFormat.Encoding(String name)</code> (possibly
 * renamed to <code>getInstance(String name)</code>.<br>
 * The static instances of ULAW, ALAW PCM_UNSIGNED and PCM_SIGNED
 * encodings in that class should be retrieved using that function,
 * too (internally).<br>
 * At best, the protected constructor of that class
 * should also be replaced to be a private constructor.
 * Like this it will be prevented that developers create their own
 * instances of Encoding, which causes problems with the
 * equals method. In fact, the equals method should be redefined anyway
 * so that it compares the names and not the objects.
 * <p>
 * Also, a specification request should be made to integrate
 * <code>getEncodings()</code> into AudioSystem (this is
 * especially annoying as the relevant methods already exist
 * in the provider interfaces of file readers, file writers and
 * converters).
 *
 * @author Florian Bomers
 */
public class Encodings extends AudioFormat.Encoding {

	/** contains all known encodings */
	private static StringHashedSet encodings = new StringHashedSet();

	// initially add the standard encodings
	static {
		encodings.add(AudioFormat.Encoding.PCM_SIGNED);
		encodings.add(AudioFormat.Encoding.PCM_UNSIGNED);
		encodings.add(AudioFormat.Encoding.ULAW);
		encodings.add(AudioFormat.Encoding.ALAW);
	}

	Encodings(String name) {
		super(name);
	}

	/**
	 * Use this method for retrieving an instance of
	 * <code>AudioFormat.Encoding</code> of the specified
	 * name. A standard registry of encoding names will
	 * be maintained by the Tritonus team.
	 * <p>
	 * Every file reader, file writer, and format converter
	 * provider should exclusively use this method for
	 * retrieving instances of <code>AudioFormat.Encoding</code>.
	 */
	/*
	  MP2000/09/11:
	  perhaps it is not a good idea to allow user programs the creation of new
	  encodings. The problem with it is that a plain typo will produce an encoding
	  object that is not supported. Instead, some indication of an error should be
	  signaled to the user program. And, there should be a second interface for
	  service providers allowing them to register encodings supported by themselves.

	  $$fb2000/09/26:
	  The problem is what you see as second issue: it can never be assured
	  that this class knows all available encodings. So at the moment, there
	  is no choice than to allow users to create any Encoding they wish.
	  The encodings database will simplify things.
	  A problem with an interface to retrieve supported encodings (or API
	  function in spi.FormatConversionProvider) is that this requires
	  loading of all providers very early. Hmmm, maybe this is necessary in any
	  case when the user issues something like AudioSystem.isConversionSupported.
	 */
	public static AudioFormat.Encoding getEncoding(String name) {
		AudioFormat.Encoding res=(AudioFormat.Encoding) encodings.get(name);
		if (res==null) {
			// it is not already in the string set. Create a new encoding instance.
			res=new Encodings(name);
			// and save it for the future
			encodings.add(res);
		}
		return res;
	}

	/**
	 * Tests for equality of 2 encodings. They are equal when their strings match.
	 * <p>
	 * This function should be AudioFormat.Encoding.equals and must
	 * be considered as a temporary work around until it flows into the
	 * JavaSound API.
	 */
	// IDEA: create a special "NOT_SPECIFIED" encoding
	// and a AudioFormat.Encoding.matches method.
	public static boolean equals(AudioFormat.Encoding e1, AudioFormat.Encoding e2) {
		return e2.toString().equals(e1.toString());
	}


	/**
	 * Returns all &quot;supported&quot; encodings.
	 * Supported means that it is possible to read or
	 * write files with this encoding, or that a converter
	 * accepts this encoding as source or target format.
	 * <p>
	 * Currently, this method returns a best guess and
	 * the search algorithm is far from complete: with standard
	 * methods of AudioSystem, only the target encodings
	 * of the converters can be retrieved - neither
	 * the source encodings of converters nor the encodings
	 * of file readers and file writers cannot be retrieved.
	 */
	public static AudioFormat.Encoding[] getEncodings() {
		StringHashedSet iteratedSources=new StringHashedSet();
		StringHashedSet retrievedTargets=new StringHashedSet();
		Iterator sourceFormats=encodings.iterator();
		while (sourceFormats.hasNext()) {
			AudioFormat.Encoding source=(AudioFormat.Encoding) sourceFormats.next();
			iterateEncodings(source, iteratedSources, retrievedTargets);
		}
		return (AudioFormat.Encoding[]) retrievedTargets.toArray(
		           new AudioFormat.Encoding[retrievedTargets.size()]);
	}


	private static void iterateEncodings(AudioFormat.Encoding source,
	                                     StringHashedSet iteratedSources,
	                                     StringHashedSet retrievedTargets) {
		if (!iteratedSources.contains(source)) {
			iteratedSources.add(source);
			AudioFormat.Encoding[] targets=AudioSystem.getTargetEncodings(source);
			for (int i=0; i<targets.length; i++) {
				AudioFormat.Encoding target=targets[i];
				if (retrievedTargets.add(target.toString())) {
					iterateEncodings(target, iteratedSources,retrievedTargets);
				}
			}
		}
	}
}



/*** Encodings.java ***/

