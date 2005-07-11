/*
 * MP3MetadataParser.
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

/** An object that fires off TagParseEvents as they are parsed
    from a stream, ServerSocket, or other metadata source
  */
public interface MP3MetadataParser {
	/** Adds a TagParseListener to be notified when this object
	    parses MP3Tags.
	 */
	public void addTagParseListener(TagParseListener tpl);
	/** Removes a TagParseListener, so it won't be notified when
	    this object parses MP3Tags.
	 */
	public void removeTagParseListener(TagParseListener tpl);
	/** Get all tags (headers or in-stream) encountered thusfar.
	    This is included in this otherwise Listener-like scheme
	    because most standards are a mix of start-of-stream
	    metadata tags (like the http headers or the stuff at the
	    top of an ice stream) and inline data.  Implementations should
	    hang onto all tags they parse and provide them with this
	    call.  Callers should first use this call to get initial
	    tags, then subscribe for events as the stream continues.
	 */
	public MP3Tag[] getTags();
}
