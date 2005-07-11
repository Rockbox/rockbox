/*
 * IcyInputStream.
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

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.HashMap;
import java.util.StringTokenizer;

/** An BufferedInputStream that parses Shoutcast's "icy" metadata
    from the stream.  Gets headers at the beginning and if the
    "icy-metaint" tag is found, it parses and strips in-stream
    metadata.
    <p>
    <b>The deal with metaint</b>: Icy streams don't try to put
    tags between MP3 frames the way that ID3 does.  Instead, it
    requires the client to strip metadata from the stream before
    it hits the decoder.  You get an
    <code>icy-metaint</code> name/val in the beginning of the
    stream iff you sent "Icy-Metadata" with value "1" in the
    request headers (SimpleMP3DataSource does this if the
    "parseStreamMetadata" boolean is true).  If this is the case
    then the value of icy-metaint is the amount of real data
    between metadata blocks.  Each block begins with an int 
    indicating how much metadata there is -- the block is this
    value times 16 (it can be, and often is, 0).
    <p>
    Originally thought that "icy" implied Icecast, but this is
    completely wrong -- real Icecast servers, found through
    www.icecast.net and typified by URLs with a trailing directory 
    (like CalArts School of Music - http://65.165.174.100:8000/som)
    do not have the "ICY 200 OK" magic string or any of the 
    CRLF-separated headers.  Apparently, "icy" means "Shoutcast".
    Yep, that's weird.
    @author Chris Adamson, invalidname@mac.com
 */
public class IcyInputStream
	extends BufferedInputStream
	implements MP3MetadataParser {
	
	public static boolean DEBUG = false;
	
	MP3TagParseSupport tagParseSupport;
	/** inline tags are delimited by ';', also filter out
	    null bytes
	 */
	protected static final String INLINE_TAG_SEPARATORS = ";\u0000";
	/* looks like icy streams start start with
	   ICY 200 OK\r\n
	   then the tags are like
	   icy-notice1:<BR>This stream requires <a href="http://www.winamp.com/">Winamp</a><BR>\r\n
	   icy-notice2:SHOUTcast Distributed Network Audio Server/win32 v1.8.2<BR>\r\n
	   icy-name:Core-upt Radio\r\n
	   icy-genre:Punk Ska Emo\r\n
	   icy-url:http://www.core-uptrecords.com\r\n
	   icy-pub:1\r\n
	   icy-metaint:8192\r\n
	   icy-br:56\r\n
	   \r\n (signifies end of headers)
	   we only get icy-metaint if the http request that created
	   this stream sent the header "icy-metadata:1"
	   //
	   in in-line metadata, we read a byte that tells us how
	   many 16-byte blocks there are (presumably, we still use
	   \r\n for the separator... the block is padded out with
	   0x00's that we can ignore)
	
	   // when server is full/down/etc, we get the following for
	   // one of the notice lines:
	   icy-notice2:This server has reached its user limit<BR>
	       or
	   icy-notice2:The resource requested is currently unavailable<BR>
	 */
	/** Tags that have been discovered in the stream.
	 */
	HashMap tags;
	/** Buffer for readCRLF line... note this limits lines to
	    1024 chars (I've read that WinAmp barfs at 128, so
	    this is generous)
	 */
	protected byte[] crlfBuffer = new byte[1024];
	/** value of the "metaint" tag, which tells us how many bytes
	    of real data are between the metadata tags.  if -1, this stream
	    does not have metadata after the header.
	 */
	protected int metaint = -1;
	/** how many bytes of real data remain before the next
	    block of metadata.  Only meaningful if metaint != -1.
	 */
	protected int bytesUntilNextMetadata = -1;
	// TODO: comment for constructor
	/** Reads the initial headers of the stream and adds
	    tags appropriatly.  Gets set up to find, read, 
	    and strip blocks of in-line metadata if the
	    <code>icy-metaint</code> header is found.
	 */
	public IcyInputStream(InputStream in) throws IOException {
		super(in);
		tags = new HashMap();
		tagParseSupport = new MP3TagParseSupport();
		// read the initial tags here, including the metaint
		// and set the counter for how far we read until
		// the next metadata block (if any).
		readInitialHeaders();
		IcyTag metaIntTag = (IcyTag) getTag("icy-metaint");
		if (DEBUG) System.out.println("METATAG:"+metaIntTag);
		if (metaIntTag != null) {
			String metaIntString = metaIntTag.getValueAsString();			
			try {
				metaint = Integer.parseInt(metaIntString.trim());
				if (DEBUG) System.out.println("METAINT:"+metaint);
				bytesUntilNextMetadata = metaint;
			}
			catch (NumberFormatException nfe) {
			}
		}
	}

	/**
	 * IcyInputStream constructor for know meta-interval (Icecast 2)
	 * @param in
	 * @param metaint
	 * @throws IOException
	 */
	public IcyInputStream(InputStream in, String metaIntString) throws IOException {
		super(in);
		tags = new HashMap();
		tagParseSupport = new MP3TagParseSupport();
		try 
		{
			metaint = Integer.parseInt(metaIntString.trim());
			if (DEBUG) System.out.println("METAINT:"+metaint);
			bytesUntilNextMetadata = metaint;
		}
		catch (NumberFormatException nfe) {
		}
	}

	/** Assuming we're at the top of the stream, read lines one
	    by one until we hit a completely blank \r\n.  Parse the 
	    data as IcyTags.
	 */
	protected void readInitialHeaders() throws IOException {
		String line = null;
		while (!((line = readCRLFLine()).equals(""))) {
			int colonIndex = line.indexOf(':');
			// does it have a ':' separator
			if (colonIndex == -1)
				continue;
			IcyTag tag =
				new IcyTag(
					line.substring(0, colonIndex),
					line.substring(colonIndex + 1));
			//System.out.println(tag);
			addTag(tag);
		}
	}
	/** Read everything up to the next CRLF, return it as 
	    a String.
	 */
	protected String readCRLFLine() throws IOException {
		int i = 0;
		for (; i < crlfBuffer.length; i++) {
			byte aByte = (byte) read();
			if (aByte == '\r') {
				// possible end of line
				byte anotherByte = (byte) read();
				i++; // since we read again
				if (anotherByte == '\n') {
					break; // break out of while
				}
				else {
					// oops, not end of line - put these in array
					crlfBuffer[i - 1] = aByte;
					crlfBuffer[i] = anotherByte;
				}
			}
			else {
				// if not \r
				crlfBuffer[i] = aByte;
			}
		} // for
		// get the string from the byte[].  i is 1 too high because of
		// read-ahead in crlf block
		return new String(crlfBuffer, 0, i - 1);
	}
	/** Reads and returns a single byte.
	    If the next byte is a metadata block, then that
	    block is read, stripped, and parsed before reading
	    and returning the first byte after the metadata block.
	 */
	public int read() throws IOException {
		if (bytesUntilNextMetadata > 0) {
			bytesUntilNextMetadata--;
			return super.read();
		}
		else if (bytesUntilNextMetadata == 0) {
			// we need to read next metadata block
			readMetadata();
			bytesUntilNextMetadata = metaint - 1;
			// -1 because we read byte on next line
			return super.read();
		}
		else {
			// no metadata in this stream
			return super.read();
		}
	}
	/** Reads a block of bytes.  If the next byte is known
	    to be a block of metadata, then that is read, parsed,
	    and stripped, and then a block of bytes is read and
	    returned.
	    Otherwise, it may read up to but
	    not into the next metadata block if
	    <code>bytesUntilNextMetadata &lt; length</code>
	 */
	public int read(byte[] buf, int offset, int length) throws IOException {
		// if not on metadata, do the usual read so long as we
		// don't read past metadata
		if (bytesUntilNextMetadata > 0) {
			int adjLength = Math.min(length, bytesUntilNextMetadata);
			int got = super.read(buf, offset, adjLength);
			bytesUntilNextMetadata -= got;
			return got;
		}
		else if (bytesUntilNextMetadata == 0) {
			// read/parse the metadata
			readMetadata();
			// now as above, except that we reset
			// bytesUntilNextMetadata differently
			
			//int adjLength = Math.min(length, bytesUntilNextMetadata);
			//int got = super.read(buf, offset, adjLength);
			//bytesUntilNextMetadata = metaint - got;

			// Chop Fix - JavaZOOM (3 lines above seem buggy)
			bytesUntilNextMetadata = metaint;
			int adjLength = Math.min(length, bytesUntilNextMetadata);
			int got = super.read(buf, offset, adjLength);
			bytesUntilNextMetadata -= got;
			// End fix - JavaZOOM
			
			
			return got;
		}
		else {
			// not even reading metadata
			return super.read(buf, offset, length);
		}
	}
	/** trivial <code>return read (buf, 0, buf.length)</code>
	 */
	public int read(byte[] buf) throws IOException {
		return read(buf, 0, buf.length);
	}
	/** Read the next segment of metadata.  The stream <b>must</b>
	    be right on the segment, ie, the next byte to read is
	    the metadata block count.  The metadata is parsed and
	    new tags are added with addTag(), which fires events
	 */
	protected void readMetadata() throws IOException {
		int blockCount = super.read();
		if (DEBUG) System.out.println("BLOCKCOUNT:"+blockCount);
		// System.out.println ("blocks to read: " + blockCount);
		int byteCount = (blockCount * 16); // 16 bytes per block
		if (byteCount < 0)
			return; // WTF?!
		byte[] metadataBlock = new byte[byteCount];
		int index = 0;
		// build an array of this metadata
		while (byteCount > 0) {
			int bytesRead = super.read(metadataBlock, index, byteCount);
			index += bytesRead;
			byteCount -= bytesRead;
		}
		// now parse it
		if (blockCount > 0)
			parseInlineIcyTags(metadataBlock);
	} // readMetadata
	/** Parse metadata from an in-stream "block" of bytes, add
	    a tag for each one.
	    <p>
	    Hilariously, the inline data format is totally different
	    than the top-of-stream header.  For example, here's a
	    block I saw on "Final Fantasy Radio":
	<pre>
	StreamTitle='Final Fantasy 8 - Nobuo Uematsu - Blue Fields';StreamUrl='';
	</pre>
	    In other words:
	    <ol>
	    <li>Tags are delimited by semicolons
	    <li>Keys/values are delimited by equals-signs
	    <li>Values are wrapped in single-quotes
	    <li>Key names are in SentenceCase, not lowercase-dashed
	    </ol>
	 */
	protected void parseInlineIcyTags(byte[] tagBlock) {
		String blockString = new String(tagBlock);
		if (DEBUG) System.out.println("BLOCKSTR:"+blockString);
		StringTokenizer izer =
			new StringTokenizer(blockString, INLINE_TAG_SEPARATORS);
		int i = 0;
		while (izer.hasMoreTokens()) {
			String tagString = izer.nextToken();
			int separatorIdx = tagString.indexOf('=');
			if (separatorIdx == -1)
				continue; // bogus tagString if no '='
			// try to strip single-quotes around value, if present
			int valueStartIdx =
				(tagString.charAt(separatorIdx + 1) == '\'')
					? separatorIdx + 2
					: separatorIdx + 1;
			int valueEndIdx =
				(tagString.charAt(tagString.length() - 1)) == '\''
					? tagString.length() - 1
					: tagString.length();
			String name = tagString.substring(0, separatorIdx);
			String value = tagString.substring(valueStartIdx, valueEndIdx);
			// System.out.println (i++ + " " +  name + ":" + value);
			IcyTag tag = new IcyTag(name, value);
			addTag(tag);
		}
	}
	/** adds the tag to the HashMap of tags we have encountered
	    either in-stream or as headers, replacing any previous
	    tag with this name.
	 */
	protected void addTag(IcyTag tag) {
		tags.put(tag.getName(), tag);
		// fire this as an event too
		tagParseSupport.fireTagParsed(this, tag);
	}
	/** Get the named tag from the HashMap of headers and
	    in-line tags.  Null if no such tag has been encountered.
	 */
	public MP3Tag getTag(String tagName) {
		return (MP3Tag) tags.get(tagName);
	}
	/** Get all tags (headers or in-stream) encountered thus far.
	 */
	public MP3Tag[] getTags() {
		return (MP3Tag[]) tags.values().toArray(new MP3Tag[0]);
	}
	/** Returns a HashMap of all headers and in-stream tags
	    parsed so far.
	 */
	public HashMap getTagHash() {
		return tags;
	}
	/** Adds a TagParseListener to be notified when this stream
	    parses MP3Tags.
	 */
	public void addTagParseListener(TagParseListener tpl) {
		tagParseSupport.addTagParseListener(tpl);
	}
	/** Removes a TagParseListener, so it won't be notified when
	    this stream parses MP3Tags.
	 */
	public void removeTagParseListener(TagParseListener tpl) {
		tagParseSupport.removeTagParseListener(tpl);
	}
	/** Quickie unit-test.
	 */
	public static void main(String args[]) {
		byte[] chow = new byte[200];
		if (args.length != 1) {
			//System.out.println("Usage: IcyInputStream <url>");
			return;
		}
		try {
			URL url = new URL(args[0]);
			URLConnection conn = url.openConnection();
			conn.setRequestProperty("Icy-Metadata", "1");
			IcyInputStream icy =
				new IcyInputStream(
					new BufferedInputStream(conn.getInputStream()));
			while (icy.available() > -1) {
				// icy.read();
				icy.read(chow, 0, chow.length);
			}
		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
}
