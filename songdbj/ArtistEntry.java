import java.util.*;
import java.io.*;

public class ArtistEntry extends Entry implements Comparable {
	protected String name;
  protected Vector albums;
  protected int albumcount;

	public ArtistEntry(String n) {
  	name=n;
  	albums=new Vector();
  	albumcount=0;
  }

	public void addAlbum(AlbumEntry e) {
		if(!albums.contains(e)) {
  		albums.add(e);
  		e.setArtist(this);
  		albumcount++;
			Collections.sort(albums);
	  }
  }

	public void removeAlbum(AlbumEntry e) {
		albums.remove(e);
		albumcount--;
	}
  
  public int size() { return albumcount; }

  public int compareTo(Object o) {
  	return String.CASE_INSENSITIVE_ORDER.compare(name,((ArtistEntry)o).getName());
  }
  
  public String getName() { return name; }  
  public Collection getAlbums() { return albums; }
  public void write(DataOutputStream w) throws IOException {
  		int x;
			w.writeBytes(name);
			for(x=TagDatabase.getInstance().artistlen-name.length();x>0;x--)
				w.write(0);
			Iterator i2 = albums.iterator();
			x=0;
			while(i2.hasNext()) {
				Entry e = (Entry) i2.next();
			  w.writeInt(e.getOffset());
			  x++;
			}
			for(;x<TagDatabase.getInstance().albumarraylen;x++)
				w.writeInt(0);  	
  }
	public static int entrySize() {
		TagDatabase td=TagDatabase.getInstance();
		return td.artistlen+4*td.albumarraylen;
	}  
}