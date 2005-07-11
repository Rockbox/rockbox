/*
 *	AudioFileTypes.java
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

import javax.sound.sampled.AudioFileFormat;
import org.tritonus.share.StringHashedSet;
import org.tritonus.share.TDebug;

/**
 * This class is a proposal for generic handling of audio file format types.
 * The main purpose is to provide a standardized way of
 * implementing audio file format types. Like this, file types
 * are only identified by their String name, and not, as currently,
 * by their object instance.
 * <p>
 * A standard registry of file type names will
 * be maintained by the Tritonus team.
 * <p>
 * In a specification request to JavaSoft, these static methods
 * could be integrated into<code>AudioFileFormat.Type</code>. The static
 * instances of AIFF, AIFC, AU, SND, and WAVE types in class
 * <code>AudioFileFormat.Type</code> should be retrieved
 * using this method, too (internally).<br>
 * At best, the protected constructor of that class
 * should also be replaced to be a private constructor.
 * Like this it will be prevented that developers create
 * instances of Type, which causes problems with the
 * equals method. In fact, the equals method should be redefined anyway
 * so that it compares the names and not the objects.
 * <p>
 * Also, the file name extension should be deprecated and moved
 * to <code>AudioFileFormat</code>. There are some file formats
 * which have varying extensions depending, e.g. on the encoding.
 * An example for this is MPEG: the special encoding Mpeg 1, layer 3
 * has the extension mp3, whereas other Mpeg files use mpeg or mpg.<br>
 * This could be implemented with 2 methods in <code>AudioFileFormat</code>:
 * <ol><li>String[] getFileExtensions(): returns all usable extensions
 *         for this file.
 *     <li>String getDefaultFileExtension(): returns the preferred extension.
 * </ol>
 *
 * @author Florian Bomers
 */
public class AudioFileTypes extends AudioFileFormat.Type {

	/** contains all known types */
	private static StringHashedSet types = new StringHashedSet();

	// initially add the standard types
	static {
		types.add(AudioFileFormat.Type.AIFF);
		types.add(AudioFileFormat.Type.AIFC);
		types.add(AudioFileFormat.Type.AU);
		types.add(AudioFileFormat.Type.SND);
		types.add(AudioFileFormat.Type.WAVE);
	}

	AudioFileTypes(String name, String ext) {
		super(name, ext);
	}

	/**
	 * Use this method to retrieve an instance of
	 * <code>AudioFileFormat.Type</code> of the specified
	 * name. If no type of this name is in the internally
	 * maintained list, <code>null</code> is returned.
	 * <p>
	 * This method is supposed to be used by user programs.
	 * <p>
	 * In order to assure a well-filled internal list,
	 * call <code>AudioSystem.getAudioFileTypes()</code>
	 * at initialization time.
	 *
	 * @see #getType(String, String)
	 */
	public static AudioFileFormat.Type getType(String name) {
		return getType(name, null);
	}

	/**
	 * Use this method to retrieve an instance of
	 * <code>AudioFileFormat.Type</code> of the specified
	 * name. If it does not exist in the internal list
	 * of types, a new type is created and returned.
	 * If it a type of that name already exists (regardless
	 * of extension), it is returned. In this case it can
	 * not be guaranteed that the extension is the same as
	 * passed as parameter.<br>
	 * If <code>extension</code> is <code>null</code>,
	 * this method returns <code>null</code> if the
	 * type of the specified name does not exist in the
	 * internal list.
	 * <p>
	 * This method is supposed to be used by file providers.
	 * Every file reader and file writer provider should
	 * exclusively use this method for retrieving instances
	 * of <code>AudioFileFormat.Type</code>.
	 */
	public static AudioFileFormat.Type getType(String name, String extension) {
		AudioFileFormat.Type res=(AudioFileFormat.Type) types.get(name);
		if (res==null) {
			// it is not already in the string set.
			if (extension==null) {
				return null;
			}
			// Create a new type instance.
			res=new AudioFileTypes(name, extension);
			// and save it for the future
			types.add(res);
		}
		return res;
	}

	/**
	 * Tests for equality of 2 file types. They are equal when their names match.
	 * <p>
	 * This function should be AudioFileFormat.Type.equals and must
	 * be considered as a temporary workaround until it flows into the
	 * JavaSound API.
	 */
	// IDEA: create a special "NOT_SPECIFIED" file type
	// and a AudioFileFormat.Type.matches method.
	public static boolean equals(AudioFileFormat.Type t1, AudioFileFormat.Type t2) {
		return t2.toString().equals(t1.toString());
	}

}

/*** AudioFileTypes.java ***/

