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
package entagged.audioformats.generic;

import java.util.*;

import entagged.audioformats.Tag;

/**
 *	<p>This class stores all the meta-data (artist, album, ..) contained in an AudioFile.</p>
 *	<p>All the <code>set</code> operation are not "real-time", they only apply to this object. To commit the modification, either the <code>AudioFile</code>'s <code>commit()</code> or the <code>AudioFileIO.write(AudioFile)</code> have to be called</p>
 *	<p>There are two types of fields contained in a <code>Tag</code>. The most commonly used (the <em>common fields</em>) are accessible through get/set methods, like the artist, tha album, the track number, etc.</p>
 *	<p>Each tag format has then <em>specific fields</em> (eg. graphics, urls, etc) that do not exist in the other formats, or that cannot be mapped to <em>common fields</em></p>
 *	<p>To retreive the content of these <em>specific fields</em>, the <code>getSpecificields()</code> can be called giving the names of the <em>specific fields</em> stored in this tag</p>
 *	<p>This ensures that all the old meta-data contained in a tag read from <code>AudioFileIO</code> is kept and rewritten when the tag is updated</p>
 *
 *@author	Raphael Slinckx
 *@version	$Id$
 *@since	v0.01
 *@see	AudioFile
 */
public class OldTag {
	/**
	 * This final field contains all the tags that id3v1 supports. The list has
	 * the same order as the id3v1 genres. To be perfectly compatible (with id3v1)
	 * the genre field should match one of these genre (case ignored). You can also use
	 * this list to present a list of basic (modifiable) possible choices for the genre field.
	 */
	public static final String[] DEFAULT_GENRES = {"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz",
			"Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative",
			"Ska", "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion",
			"Trance", "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "AlternRock",
			"Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
			"Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
			"Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret", "New Wave",
			"Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro",
			"Musical", "Rock & Roll", "Hard Rock", "Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebob", "Latin", "Revival",
			"Celtic", "Bluegrass", "Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
			"Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music", "Sonata",
			"Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad",
			"Power Ballad", "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall"};


	//Contains all the fields found in a tag
	protected Hashtable fields;
	
	/**
	 *	<p>Creates a new empty tag, all the common fields are initialized to the empty string, and no specific fields exists.</p>
	 */
	public OldTag() {
		fields = new Hashtable();
		fields.put("TITLE"  ,"");
		fields.put("ALBUM"  ,"");
		fields.put("ARTIST" ,"");
		fields.put("GENRE"  ,"");
		fields.put("TRACK"  ,"");
		fields.put("YEAR"   ,"");
		fields.put("COMMENT","");
	}
	
	/**
	 *	<p>Returns the title of this song, eg: "Stairway to Heaven".</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the title of this song
	 */
	public String getTitle() {
		return (String) fields.get("TITLE");
	}
	
	/**
	 *	<p>Returns the album of this song, eg: "Led Zeppelin IV".</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the album of this song
	 */
	public String getAlbum() {
		return (String) fields.get("ALBUM");
	}
	
	/**
	 *	<p>Returns the artist of this song, eg: "Led Zeppelin".</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the artist of this song
	 */
	public String getArtist() {
		return (String) fields.get("ARTIST");
	}
	
	/**
	 *	<p>Returns the genre of this song, eg: "Classic Rock".</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the genre of this song
	 */
	public String getGenre() {
		return (String) fields.get("GENRE");
	}
	
	/**
	 *	<p>Returns the track number of this song.</p>
	 *	<p>The String can be anything, so don't expect a simple number, it could be "10" but also "10 / 15"</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the track number of this song
	 */
	public String getTrack() {
		return (String) fields.get("TRACK");
	}
	
	/**
	 *	<p>Returns the date of this song.</p>
	 *	<p>Most of the time the year of release of the album eg: "1970", but this can also be an arbitrary string like "19 december 1934"</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the date of this song
	 */
	public String getYear() {
		return (String) fields.get("YEAR");
	}
	
	/**
	 *	<p>Returns the comment associated with this song, eg: "Recorded live at the Royal Albert Hall".</p>
	 *	<p>Some tag format allow multi-line comments, line are separated with "\n" strings ("\\n" in java not "\n"). That means the String returned may have to be parsed to transform those strings in newline characters ("\n" in java)</p>
	 *	<p>If there wasn't such a field in the AudioFile, an empty String is returned</p>
	 *
	 *@return	Returns the comment associated with this song
	 *@todo Parse the returned string to replace \\n by \n for multiline comments
	 */
	public String getComment() {
		return (String) fields.get("COMMENT");
	}
	
	/**
	 *	<p>Sets the title for this song, replacing the previous one.</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new title of this song
	 */
	public void setTitle(String s) {
		if(s == null)
			fields.put("TITLE","");
		else
			fields.put("TITLE",s);
	}
	
	/**
	 *	<p>Sets the new album for this song, replacing the previous one.</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new album for this song
	 */
	public void setAlbum(String s) {
		if(s == null)
			fields.put("ALBUM","");
		else
			fields.put("ALBUM",s);
	}
	
	/**
	 *	<p>Sets the new artist for this song, replacing the previous one.</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new artist for this song
	 */
	public void setArtist(String s) {
		if(s == null)
			fields.put("ARTIST","");
		else
			fields.put("ARTIST",s);
	}
	
	/**
	 *	<p>Sets the new genre for this song, replacing the previous one.</p>
	 *	<p>Some formats does not support an arbitrary string as genre and use a predefined list of genres, with an index. If this is the case, the given genre will be matched against a predefined list, if a correspondance is found, that index is used, if not, a "no genre" or equivalent will be used instead</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new genre for this song
	 */
	public void setGenre(String s) {
		if(s == null)
			fields.put("GENRE","");
		else
			fields.put("GENRE",s);
	}
	
	/**
	 *	<p>Sets the new track number for this song, replacing the previous one.</p>
	 *	<p>Some formats use a single byte track encoding, allowing tracks to be only a number from 0 to 255. If this is the case, and the track number is not a simple number (like "19/30"), the track number is set to 0</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new track number for this song
	 */
	public void setTrack(String s) {
		if(s == null)
			fields.put("TRACK","");
		else
			fields.put("TRACK",s);
	}
	
	/**
	 *	<p>Sets the new date for this song, replacing the previous one.</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new date for this song
	 */
	public void setYear(String s) {
		if(s == null)
			fields.put("YEAR","");
		else
			fields.put("YEAR",s);
	}
	
	/**
	 *	<p>Sets the new comment of this song, replacing the previous one.</p>
	 *	<p>Some tag allow multi-line formats, using the "\n" string ("\\n" in java) but not the newline character ("\n" in java)</p>
	 *	<p><code>s</code> can be an arbitrary string, but keep in mind that some tag formats have limited space to store informations, if <code>s</code> is too long, it will be cut at the maximum length allowed by the tag format</p>
	 *	<p>If a null string is passed, it is converted to an empty String</p>
	 *
	 *@param s the new comment for this song
	 *@todo Parse the given string to replace \n by \\n for multiline comments
	 */
	public void setComment(String s) {
		if(s == null)
			fields.put("COMMENT","");
		else 
			fields.put("COMMENT",s);
	}
	
	
	/**
	 *	<p>This creates an iterator over the specific fields for this tag. Specific fields are tag-format specific fields (other than artist, album, etc).</p>
	 *	<p>If this tag was created by <code>new Tag()</code>, the iterator will be empty</p>
	 *	<p>To retreive the value associated to a specific field use the <code>getSpecificField(String)</code> method in this class</p>
	 *	<p>Example: an ogg file can contain an arbitrary number of arbitrary fields, so a field like MYCUSTOMFIELD cannot be mapped to artist, album, or other common field, it is instead stored as a specific field that can be retreived using this iterator</p>
	 *@return an iterator over the specific fields of this tag
	 */
	public Iterator getSpecificFields() {
		//Those fileds are created by subclasses of this class
		//And begin with a "-", the iterator contains the fields without the leading "-"
		List l = new LinkedList();
		Enumeration en = fields.keys();
		while(en.hasMoreElements()) {
          String key = (String) en.nextElement();
		  if( key.startsWith("-") )
			  l.add(key.substring(1));
		}
		
		return l.iterator();
	}
	
	/**
	 *	<p>Returns the specific field's content.</p>
	 *	<p>if <code>s</code> is not a valid specific field, the empty string is returned</p>
	 *	<p>Valid specific fields are returned by the iterator created by <code>getSpecificFields()</code> method</p>
	 *	<p>Note that some specific fields can have binary value (not a human readable string), the binary data can be obtained back using the <code>getBytes()</code> method on the String</p>
	 *
	 *@param s the specific field to retreive
	 *@return the value associated to this specific field, or the empty string if the field wasn't valid
	 *@todo binary strings ??
	 */
	public String getSpecificField(String s) {
		//Return the value associated with a specific field
		//User must ensure that the field actually exists, otherwise an empty string will be returned !
		//The given field must not contain the leading "-", it is appended here !
		String content = (String) fields.get("-"+s);
		if(content == null)
			return "";
		return content;
	}
	
	
	/**
	 *	<p>Checks wether all the common fields (artist, genre, ..) are empty, that is: contains the empty string or only spaces.</p>
	 *	<p>Specific fields can exist even if this returns true, use the <code>isSpecificEmpty()</code> to check if the specific fields are also empty</p>
	 *
	 *@return a boolean indicating if the common fields of this tag are empty or not
	 */
	public boolean isEmpty() {
		Enumeration en = fields.keys();
		while(en.hasMoreElements()) {
          String key = (String) en.nextElement();
		  String val = (String) fields.get(key);
		  if( !key.startsWith("-") && !val.trim().equals("") )
			  return false;
		}
		
		return true;
	}
	
	
	/**
	 *	<p>Checks wether all the specific fields (other than artist, genre, ..) are empty, that is: contains the empty string or only spaces.</p>
	 *	<p>Common fields can exist even if this returns true, use the <code>isEmpty()</code> to check if the common fields are also empty</p>
	 *
	 *@return a boolean indicating if the specific fields of this tag are empty or not
	 */
	public boolean isSpecificEmpty() {
		Enumeration en = fields.keys();
		while(en.hasMoreElements()) {
          String key = (String) en.nextElement();
		  String val = (String) fields.get(key);
		  if( key.startsWith("-") && !val.trim().equals("") )
			  return false;
		}
		
		return true;
	}
	
	/*
	 * Merge the content of the given tag with the content of this tag.
	 * It keeps this tag fields, but if one field is empty, it looks for a
	 * non-empty value in the given tag and place it's value in this tag.
	 * Note that it only looks in the "common" fields (artist, album).
	 * 
	 * @param tag The tag to use if one of this tag's field is empty.
	 */
	/*public void merge(Tag tag) {
		if( getTitle().trim().equals("") )
			setTitle(tag.getTitle());
		if( getArtist().trim().equals("") )
			setArtist(tag.getArtist());
		if( getAlbum().trim().equals("") )
			setAlbum(tag.getAlbum());
		if( getYear().trim().equals("") )
			setYear(tag.getYear());
		if( getComment().trim().equals("") )
			setComment(tag.getComment());
		if( getTrack().trim().equals("") )
			setTrack(tag.getTrack());
		if( getGenre().trim().equals("") )
			setGenre(tag.getGenre());
	}*/
	
	/**
	 *	<p>Returns a multi-line string showing all the meta-data of this tag (the common fields, and any specific field).</p>
	 *	<p>Specific fields are denoted with a "-" appended to the field name</p>
	 *
	 *@return the contents of this tag
	 */
	public String toString() {
		StringBuffer out = new StringBuffer(50);
		out.append("Tag content:\n");
		Enumeration en = fields.keys();
		while(en.hasMoreElements()) {
          Object field = en.nextElement();
		  Object content = fields.get(field);
		  out.append("\t");
		  out.append(field);
		  out.append(" : ");
		  out.append(content);
		  out.append("\n");
		}
		return out.toString().substring(0,out.length()-1);
	}
}
