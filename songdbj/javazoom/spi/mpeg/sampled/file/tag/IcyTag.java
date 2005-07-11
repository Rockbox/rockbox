/*
 * IcyTag.
 * 
 * jicyshout : http://sourceforge.net/projects/jicyshout/
 *  
 * JavaZOOM : mp3spi@javazoom.net
 * 			  http://www.javazoom.net
 * 
 *-----------------------------------------------------------------------
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
 *----------------------------------------------------------------------
 */

package javazoom.spi.mpeg.sampled.file.tag;
/** 
 * A tag parsed from an icecast tag. 
 */
public class IcyTag extends MP3Tag implements StringableTag {
	/** Create a new tag, from the parsed name and (String) value.
	    It looks like all Icecast tags are Strings (safe to assume
	    this going forward?)
	 */
	public IcyTag(String name, String stringValue) {
		super(name, stringValue);
	}
	// so far as I know, all Icecast tags are strings
	public String getValueAsString() {
		return (String) getValue();
	}
}
