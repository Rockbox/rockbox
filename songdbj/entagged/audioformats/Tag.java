package entagged.audioformats;

import java.util.Iterator;
import java.util.List;

import entagged.audioformats.generic.TagField;

public interface Tag {
    /**
     * This final field contains all the tags that id3v1 supports. The list has
     * the same order as the id3v1 genres. To be perfectly compatible (with
     * id3v1) the genre field should match one of these genre (case ignored).
     * You can also use this list to present a list of basic (modifiable)
     * possible choices for the genre field.
     */
    public static final String[] DEFAULT_GENRES = { "Blues", "Classic Rock",
            "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz",
            "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap",
            "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
            "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient",
            "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical",
            "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel",
            "Noise", "AlternRock", "Bass", "Soul", "Punk", "Space",
            "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic",
            "Gothic", "Darkwave", "Techno-Industrial", "Electronic",
            "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
            "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
            "Native American", "Cabaret", "New Wave", "Psychadelic", "Rave",
            "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk",
            "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll",
            "Hard Rock", "Folk", "Folk-Rock", "National Folk", "Swing",
            "Fast Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
            "Avantgarde", "Gothic Rock", "Progressive Rock",
            "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
            "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
            "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony",
            "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam",
            "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",
            "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo",
            "A capella", "Euro-House", "Dance Hall" };

    public void add(TagField field);

    public void addAlbum(String s);

    public void addArtist(String s);

    public void addComment(String s);

    public void addGenre(String s);

    public void addTitle(String s);

    public void addTrack(String s);

    public void addYear(String s);

    public List get(String id);

    public Iterator getFields();

    public List getGenre();

    public List getTitle();

    public List getTrack();

    public List getYear();
    public List getAlbum();

    public List getArtist();

    public List getComment();
    
    public String getFirstGenre();

    public String getFirstTitle();

    public String getFirstTrack();

    public String getFirstYear();
    public String getFirstAlbum();

    public String getFirstArtist();

    public String getFirstComment();
   
    public boolean hasCommonFields();

    public boolean hasField(String id);

    public boolean isEmpty();

    //public Iterator getCommonFields();
    //public Iterator getSpecificFields();

    public void merge(Tag tag);

    public void set(TagField field);

    public void setAlbum(String s);

    public void setArtist(String s);

    public void setComment(String s);

    public void setGenre(String s);

    public void setTitle(String s);

    public void setTrack(String s);

    public void setYear(String s);
    
    public String toString();
}