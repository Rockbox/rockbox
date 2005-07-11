import java.io.*;

public class RundbEntry extends Entry  {
	protected FileEntry file;
	protected short rating, voladj;
	protected int playcount,lastplayed;

	public RundbEntry(FileEntry f) {
		file=f;
		rating=0;
		voladj=0;
		playcount=0;
		lastplayed=0;
	}
	
	public void write(DataOutputStream w) throws IOException {
			w.writeInt(file.getOffset());
			w.writeInt(file.getHash());
			w.writeShort(rating);
			w.writeShort(voladj);
			w.writeInt(playcount);
			w.writeInt(lastplayed);
	}

	public static int entrySize() {
		return 20;
	}
}