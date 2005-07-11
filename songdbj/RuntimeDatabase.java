import java.util.*;
import java.io.*;
import java.lang.reflect.Array;

/*
	TreeSet for runtimedatabase with entry hash used in compareto
	fix commandline interface.
*/

public class RuntimeDatabase {
	protected static RuntimeDatabase instance=null;
	protected TreeMap entries;
	protected int entrycount;
	public static final int headersize = 8;
	
	protected RuntimeDatabase() {
		entries=new TreeMap();
	}
	
	public static RuntimeDatabase getInstance() {
		if(instance==null)
			instance=new RuntimeDatabase();
		return instance;
	}
	
  public RundbEntry getEntry(FileEntry file) {
  	Integer key = new Integer(file.getHash());
		if(!entries.containsKey(key)) {
			RundbEntry e = new RundbEntry(file);
		  entries.put(key,e);
		  return e;
		}
		else
		  return (RundbEntry)entries.get(key);
	}

	protected void calcOffsets() {
	  Collection values = entries.values();
		Iterator i;
		int offset=headersize;
		i=values.iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.setOffset(offset);
			offset+=RundbEntry.entrySize();
		}
		entrycount=values.size();
	}

	public int isDirty() {
		return 0;
	}

	protected void writeHeader(DataOutputStream w) throws IOException {
		w.write('R');
		w.write('R');
		w.write('D');
		w.write(0x1);
		w.writeInt(entrycount);
	}

	public void prepareWrite() {
		System.out.println("Calculating Runtime Database Offsets..");
		calcOffsets();
	}

	public void writeDatabase(File f) throws IOException {
		int x;
		Iterator i;
		DataOutputStream w = new DataOutputStream(new FileOutputStream(f));
		System.out.println("Writing runtime database..");
		writeHeader(w);
		i=entries.values().iterator();
		while(i.hasNext()) {
			Entry e = (Entry) i.next();
			e.write(w);
		}
		w.flush();
		w.close();
	}
}