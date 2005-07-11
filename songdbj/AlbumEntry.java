import java.util.*;
import java.io.*;

public class AlbumEntry extends Entry implements Comparable {
	protected String name;
  protected ArtistEntry artist;
  protected Vector songs;
  protected int songcount;
  
  public AlbumEntry(String n) {
  	name=n;
  	songs=new Vector();
  	artist=null;
    songcount=0;
  }
  
  protected class SongSorter implements Comparator {
  	public int compare(Object o1, Object o2) {
  		SongEntry s1=(SongEntry)o1;
  		SongEntry s2=(SongEntry)o2;
  		int track1=s1.getTrack(),track2=s2.getTrack();
  		if(track1>track2)
  			return 1;
  	  else if(track1<track2)
  	  	return -1;
  	  return s1.getFile().getFile().getName().compareTo(s2.getFile().getFile().getName());
  	}
  }
  
  public void addSong(SongEntry e) {
  	songs.add(e);
  	e.setAlbum(this);
  	e.setArtist(artist);
  	songcount++;
  	Collections.sort(songs,new SongSorter());
  }

	public int size() { return songcount; }
	public void setArtist(ArtistEntry a) { 
 		a.addAlbum(this);
	 	if(artist!=null&&artist!=a&&!artist.getName().equals("<various artists>")) {
			artist.removeAlbum(this);
	 		artist=TagDatabase.getInstance().getArtistEntry("<various artists>");
	 	}
	 	else 
	 	  artist=a; 
	}
	public ArtistEntry getArtist() { return artist; }
	  
  public int compareTo(Object o) {
  	return String.CASE_INSENSITIVE_ORDER.compare(name,((AlbumEntry)o).getName());
  }
  
  public String getName() { return name; }
  public Collection getSongs() { return songs; }
  public void write(DataOutputStream w) throws IOException {
  		int x;
			w.writeBytes(name);
			for(x=TagDatabase.getInstance().albumlen-name.length();x>0;x--)
				w.write(0);
			w.writeInt(artist.getOffset());
			Iterator i2 = songs.iterator();
			x=0;
			while(i2.hasNext()) {
				Entry e = (Entry) i2.next();
			  w.writeInt(e.getOffset());
			  x++;
			}
			for(;x<TagDatabase.getInstance().songarraylen;x++)
				w.writeInt(0);
  }
	public static int entrySize() {
		TagDatabase td=TagDatabase.getInstance();
		return td.albumlen+4+td.songarraylen*4;
	}
}