/*
 * MP3Tag.
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

/** An individual piece of mp3 metadata, as a name/value pair.
    Abstract just so that subclasses will indicate their
    tagging scheme (Icy, ID3, etc.).
 */
public abstract class MP3Tag extends Object {
	protected String name;
	protected Object value;
	public MP3Tag(String name, Object value) {
		this.name = name;
		this.value = value;
	}
	public String getName() {
		return name;
	}
	public Object getValue() {
		return value;
	}
	public String toString() {
		return getClass().getName()
			+ " -- "
			+ getName()
			+ ":"
			+ getValue().toString();
	}
}
