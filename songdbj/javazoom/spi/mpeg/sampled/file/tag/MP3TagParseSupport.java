/*
 * MP3TagParseSupport.
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

import java.util.ArrayList;
/**  
*/
public class MP3TagParseSupport extends Object {
	ArrayList tagParseListeners;
	/** trivial constructor, sets up listeners list.
	 */
	public MP3TagParseSupport() {
		super();
		tagParseListeners = new ArrayList();
	}
	/** Adds a TagParseListener to be notified when a stream
	    parses MP3Tags.
	 */
	public void addTagParseListener(TagParseListener tpl) {
		tagParseListeners.add(tpl);
	}
	/** Removes a TagParseListener, so it won't be notified when
	    a stream parses MP3Tags.
	 */
	public void removeTagParseListener(TagParseListener tpl) {
		tagParseListeners.add(tpl);
	}
	/** Fires the given event to all registered listeners
	 */
	public void fireTagParseEvent(TagParseEvent tpe) {
		for (int i = 0; i < tagParseListeners.size(); i++) {
			TagParseListener l = (TagParseListener) tagParseListeners.get(i);
			l.tagParsed(tpe);
		}
	}
	public void fireTagParsed(Object source, MP3Tag tag) {
		fireTagParseEvent(new TagParseEvent(source, tag));
	}
}
