import java.util.*;
import java.io.*;
import javax.sound.sampled.UnsupportedAudioFileException;
import java.lang.NumberFormatException;
import net.shredzone.ifish.ltr.LTR;

public class SongEntry extends Entry implements Comparable {
	protected TagInfo info;
	protected LTR tag;
	protected ArtistEntry artist;
	protected AlbumEntry album;
	protected FileEntry file;
	
	public SongEntry(FileEntry f) {
		file=f;
		file.setSongEntry(this);
		readTagInfo();
	}
	
	public void setAlbum(AlbumEntry a) { album=a; }
	public void setArtist(ArtistEntry a) { artist=a; }
	public AlbumEntry getAlbum() { return album; }
	public ArtistEntry getArtist() { return artist; }
	public FileEntry getFile() { return file; }
	
  public int compareTo(Object o) {
	  return String.CASE_INSENSITIVE_ORDER.compare(getName(),((SongEntry)o).getName());
  }
  
  public String getName() {
  	String title=tag.getTitle();
	  if(title==null)
	  	title = stripExt(file.getFile().getName());
	  title=title.trim();
	  if(title.equals(""))
	  	title = stripExt(file.getFile().getName());
	 	return title;
  }
  
  public static String stripExt(String t) {
  	return t.substring(0,t.lastIndexOf('.'));
  }

  public String getAlbumTag() {
  	String album=tag.getAlbum(); 
	  if(album==null)
	  	album = "<no album tag>";
	  album=album.trim();
	  if(album.equals(""))
	  	album = "<no album tag>";
	  if(TagDatabase.getInstance().dirisalbumname&&album.equals("<no album tag>")) {
	  	album = file.getFile().getParentFile().getName();
	  }
	 	return album;
  }

  public String getArtistTag() {
  	String artist=tag.getArtist(); 
	  if(artist==null)
	  	artist = "<no artist tag>";
	  artist=artist.trim();
	  if(artist.equals(""))
	  	artist = "<no artist tag>";
	 	return artist;
  }

  public String getGenreTag() {
  	String genre=tag.getGenre(); 
	  if(genre==null)
	  	genre = "<no genre tag>";
	  genre=genre.trim();
	  if(genre.equals(""))
	  	genre = "<no genre tag>";
	 	return genre;
  }
	
	public int getYear() { 
	  try {
			return Integer.parseInt(tag.getYear()); 
		} catch(NumberFormatException e) {
			return 0;
		}
	}
	
	public int getTrack() { 	  
		try {
			return Integer.parseInt(tag.getTrack()); 
		} catch(NumberFormatException e) {
			return 0;
		}
  }
	
	public int getBitRate() { if(info==null) return -1; return info.getBitRate()/1000; }
	
	public int getPlayTime() { if(info==null) return -1; return (int)info.getPlayTime(); }
	
	public int getSamplingRate() { if(info==null) return -1; return info.getSamplingRate(); }
	
	public int getFirstFrameOffset() { if(info==null) return 0; return info.getFirstFrameOffset(); }

	public boolean gotTagInfo() { return tag!=null; }

  protected void readTagInfo() {
		// Check Mpeg format.
		try
		{
		  info = new MpegInfo();
		  info.load(file.getFile());
		}
/*		catch (IOException ex)
		{
			//ex.printStackTrace();
			System.out.println(ex);
		  info = null;
		}*/
		catch (Exception ex)
		{
		  // Error..
		  info = null;
		}

		if (info == null)
		{
		  // Check Ogg Vorbis format.
		  try
		  {
			info = new OggVorbisInfo();
			info.load(file.getFile());				
		  }
		  /*catch (IOException ex)
		  {
				//ex.printStackTrace();
				System.out.println(ex);
		    info = null;
		  }*/
		  catch (Exception ex)
		  {
			// Not Ogg Vorbis Format
		  //System.out.println("Failed reading tag for "+location.getAbsolutePath()+", tried mp3 and vorbis.");
			 info = null;
		  }
		}
		tag = LTR.create(file.getFile());
	}
	
  public void write(DataOutputStream w) throws IOException {
			String name=getName();
			w.writeBytes(name);
			for(int x=TagDatabase.getInstance().songlen-name.length();x>0;x--)
				w.write(0);
			w.writeInt(artist.getOffset());
			w.writeInt(album.getOffset());
			w.writeInt(file.getOffset());
			w.writeBytes(getGenreTag());
			for(int x=TagDatabase.getInstance().genrelen-getGenreTag().length();x>0;x--)
				w.write(0);
			w.writeShort(getBitRate());
			w.writeShort(getYear());
			w.writeInt(getPlayTime());
			w.writeShort(getTrack());
			w.writeShort(getSamplingRate());
	}
	public static int entrySize() {
		TagDatabase td=TagDatabase.getInstance();
		return td.songlen+12+td.genrelen+12;
	}
}