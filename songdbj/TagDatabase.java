import java.util.*;
import java.io.*;
import java.lang.reflect.Array;

/*
	TreeSet for runtimedatabase with entry hash used in compareto
	fix commandline interface.
*/

public class TagDatabase {
	protected static TagDatabase instance=null;
	protected TreeMap songs;
	protected TreeMap files;
	protected TreeMap filehashes;
	protected TreeMap albums;
	protected TreeMap artists;
	protected int artiststart,albumstart,songstart,filestart;
	protected int artistcount,albumcount,songcount,filecount;
	public int artistlen,albumlen,songlen,genrelen,filelen,songarraylen,albumarraylen;
	public String strip,add;
	public boolean haveOldDatabase,dirisalbum,dirisalbumname,showduplicates;
	protected Vector sortedsongs,sortedfiles,sortedalbums,sortedartists;
	
	protected TagDatabase() {
		songs=new TreeMap();
		files=new TreeMap();
		filehashes=new TreeMap();
		albums=new TreeMap();
		artists=new TreeMap();
		strip=null;
		add=null;
		haveOldDatabase=false;
		dirisalbum=false;
		dirisalbumname=true;
		showduplicates=true;
	}
	
	public static TagDatabase getInstance() {
		if(instance==null)
			instance=new TagDatabase();
		return instance;
	}
	
	public void removeFileEntry(File file) {
		 String key = file.getAbsolutePath();
		 files.remove(key);
	}
	
	public FileEntry getFileEntry(File file) throws FileNotFoundException, IOException {
	  String key = file.getAbsolutePath();
		if(!files.containsKey(key)) {
			FileEntry f = new FileEntry(file);
		  files.put(key,f);
		  return f;
		}
		else
		  return (FileEntry)files.get(key);
	}
	
	public ArtistEntry getArtistEntry(String name) {
	  String key = name.toLowerCase();
		if(!artists.containsKey(key)) {
			ArtistEntry a = new ArtistEntry(name);
		  artists.put(key,a);
		  return a;
		}
		else
		  return (ArtistEntry)artists.get(key);
	}
	
	public String getAlbumKey(String name, String directory) {
		if(dirisalbum)
		  return directory;
		else
		  return name.toLowerCase()+"___"+directory;
	}
	
	public AlbumEntry getAlbumEntry(String name,String directory) {
	  String key = getAlbumKey(name,directory);
		if(!albums.containsKey(key)) {
			AlbumEntry a = new AlbumEntry(name);
		  albums.put(key,a);
		  return a;
		}
		else
		  return (AlbumEntry)albums.get(key);
	}

	public void removeSongEntry(FileEntry file) {
		 String key = file.getFilename();
		 songs.remove(key);
		 file.setSongEntry(null);
	}
	
	public SongEntry getSongEntry(FileEntry file) {
    String key = file.getFilename();
		if(!songs.containsKey(key)) {
			SongEntry s = new SongEntry(file);
		  songs.put(key,s);
		  return s;
		}
		else
		  return (SongEntry)songs.get(key);
	}
	
	private class SongFilter implements FileFilter {
		public boolean accept(File f) {
			if(f.isDirectory()) // always accept directories.
				return true;
			String name=f.getName();
			return name.endsWith(".mp3")||name.endsWith(".ogg");
		}
	}
	
	public void add(File f) {
	  if(!f.isDirectory()) {
	  	if(f.isFile()) {
	  		addSong(f);
	  	}
	  }
	  else {
	    File[] files = f.listFiles(new SongFilter());
		  int length=Array.getLength(files);
		  System.out.println(FileEntry.convertPath(f.getAbsolutePath()));
		  for(int i=0;i<length;i++) {
		  	add(files[i]);
		  }
		}
	}

	protected FileEntry addSong(File f) {
	  FileEntry file = null;
		try {
			file = getFileEntry(f);
		}
		catch(Exception e) {
			return null;
		}
		SongEntry song = getSongEntry(file);
		if(!song.gotTagInfo()) {
			removeSongEntry(file);
			return null;
		}
		ArtistEntry artist = getArtistEntry(song.getArtistTag());
		AlbumEntry album = getAlbumEntry(song.getAlbumTag(),f.getParent());
		album.setArtist(artist);
		album.addSong(song);
		return file;
	}
	
	protected int align(int len) {
		while((len&3)!=0) len++;
		return len;
	}
	
	protected void calcLimits() {
		ArtistEntry longartist=null,longalbumarray=null;
		AlbumEntry longalbum=null, longsongarray=null;
		SongEntry longsong=null,longgenre=null;
		FileEntry longfile=null;
		Iterator i;
		artistlen=0;
		albumarraylen=0;
		i=sortedartists.iterator();
		while(i.hasNext()) {
			ArtistEntry artist = (ArtistEntry) i.next();
			int length=artist.getName().length();
			int albumcount=artist.size();
			if(length > artistlen) {
				artistlen=align(length);
				longartist=artist;
			}
			if(albumcount> albumarraylen) {
			  albumarraylen=albumcount;
			  longalbumarray=artist;
			}
		}
		artistcount=sortedartists.size();
		if(longartist!=null)
			System.out.println("Artist with longest name ("+artistlen+") :"+longartist.getName());
		if(longalbumarray!=null)
			System.out.println("Artist with most albums ("+albumarraylen+") :"+longalbumarray.getName());
		albumlen=0;
		songarraylen=0;
		i=sortedalbums.iterator();
		while(i.hasNext()) {
			AlbumEntry album = (AlbumEntry) i.next();
			int length=album.getName().length();
			int songcount=album.size();
			if(length > albumlen) {
				albumlen=align(length);
				longalbum=album;
			}
			if(songcount> songarraylen) {
			  songarraylen=songcount;
			  longsongarray=album;
			}
		}
		albumcount=sortedalbums.size();
		if(longalbum!=null)
			System.out.println("Album with longest name ("+albumlen+") :"+longalbum.getName());
		if(longsongarray!=null)
			System.out.println("Album with most songs ("+songarraylen+") :"+longsongarray.getName());
		filelen=0;
		i=sortedfiles.iterator();
		while(i.hasNext()) {
			FileEntry file = (FileEntry) i.next();
			int length=file.getFilename().length();
			if(length> filelen) {
			  filelen=align(length);
			  longfile=file;
			}
		}
		filecount=sortedfiles.size();
		if(longfile!=null)
			System.out.println("File with longest filename ("+filelen+") :"+longfile.getFilename());
		songlen=0;
		genrelen=0;
		i=sortedsongs.iterator();
		while(i.hasNext()) {
			SongEntry song = (SongEntry) i.next();
			int tlength=song.getName().length();
			int glength=song.getGenreTag().length();
			if(tlength> songlen) {
			  songlen=align(tlength);
			  longsong=song;
			}
			if(glength> genrelen) {
			  genrelen=align(glength);
			  longgenre=song;
			}
		}
		songcount=sortedsongs.size();
		if(longsong!=null)
			System.out.println("Song with longest name ("+songlen+") :"+longsong.getName());
		if(longsong!=null)
			System.out.println("Song with longest genre ("+genrelen+") :"+longgenre.getGenreTag());
		System.out.println("Artistcount: "+artistcount);
		System.out.println("Albumcount : "+albumcount);
		System.out.println("Songcount  : "+songcount);
		System.out.println("Filecount  : "+filecount);
		artiststart=68;
		albumstart=artiststart+artistcount*ArtistEntry.entrySize();
		songstart=albumstart+albumcount*AlbumEntry.entrySize();
		filestart=songstart+songcount*SongEntry.entrySize();
	}
	
	protected void calcOffsets() {
		Iterator i;
		int offset=artiststart;
		i=sortedartists.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.setOffset(offset);
			offset+=ArtistEntry.entrySize();
		}
//		assert(offset==albumstart);
		i=sortedalbums.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.setOffset(offset);
			offset+=AlbumEntry.entrySize();
		}
//		assert(offset==songstart);
		i=sortedsongs.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.setOffset(offset);
			offset+=SongEntry.entrySize();
		}
//		assert(offset==filestart);
		i=sortedfiles.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.setOffset(offset);
			offset+=FileEntry.entrySize();
		}
	}
	
	protected void calcHashes() {
		Iterator i;
		i=sortedfiles.iterator();
		while(i.hasNext()) {
			FileEntry file = (FileEntry) i.next();
			Integer key = new Integer(file.getHash());
			if(!filehashes.containsKey(key)) 
		  	filehashes.put(key,file);
			else {
		  	System.out.println("Duplicate hash:");
		  	System.out.println(((FileEntry)filehashes.get(key)).getFilename());
		  	System.out.println(file.getFilename());
		  }
		}
	}
	
	protected void writeHeader(DataOutputStream w) throws IOException {
		w.write('R');
		w.write('D');
		w.write('B');
		w.write(0x3);
		w.writeInt(artiststart);
		w.writeInt(albumstart);
		w.writeInt(songstart);
		w.writeInt(filestart);
		w.writeInt(artistcount);
		w.writeInt(albumcount);
		w.writeInt(songcount);
		w.writeInt(filecount);
		w.writeInt(artistlen);
		w.writeInt(albumlen);
		w.writeInt(songlen);
		w.writeInt(genrelen);
		w.writeInt(filelen);
		w.writeInt(songarraylen);
		w.writeInt(albumarraylen);
		w.writeInt(RuntimeDatabase.getInstance().isDirty());
	}
	
	public void prepareWrite() {
		System.out.println("Sorting artists..");
		sortedartists=new Vector();
		sortedartists.addAll(artists.values());
		Collections.sort(sortedartists);
		System.out.println("Sorting albums..");
		sortedalbums=new Vector();
		sortedalbums.addAll(albums.values());
		Collections.sort(sortedalbums);
		System.out.println("Sorting songs..");
		sortedsongs=new Vector();
		sortedsongs.addAll(songs.values());
		Collections.sort(sortedsongs);
		System.out.println("Sorting files..");
		sortedfiles=new Vector();
		sortedfiles.addAll(files.values());
		Collections.sort(sortedfiles);
		System.out.println("Calculating tag database limits..");
		calcLimits();
		System.out.println("Calculating tag database offsets..");
		calcOffsets();
		if(showduplicates) {
			System.out.println("Comparing file hashes..");	
			calcHashes();
		}
	}
	
	public void writeDatabase(File f) throws IOException {
		int x;
		Iterator i;
		DataOutputStream w = new DataOutputStream(new FileOutputStream(f));
		System.out.println("Writing tag database..");
		writeHeader(w);

		i=sortedartists.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.write(w);
		}
		i=sortedalbums.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.write(w);
		}
		i=sortedsongs.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.write(w);
		}
		i=sortedfiles.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.write(w);
		}
		// done...
		w.flush();
		w.close();
	}
}