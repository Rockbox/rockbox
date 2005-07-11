import java.io.*;

public abstract class Entry {
	protected int offset;
	
	public Entry() {
		offset=-1;
	}
	
	public void setOffset(int pos) { offset=pos; }
	public int getOffset() { return offset; }
	
	public abstract void write(DataOutputStream w) throws IOException;
}