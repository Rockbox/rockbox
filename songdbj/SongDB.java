import java.io.*;
import java.lang.reflect.Array;

public class SongDB { 
	
	public static final void main(String[] args) {		
		TagDatabase td = TagDatabase.getInstance();
		File tdfile = new File("rockbox.tagdb");
	//	RuntimeDatabase rd = RuntimeDatabase.getInstance();
    int i = 0, j;
    String arg,path = null;
		
    while (i < args.length) {
      arg = args[i++];
      if (arg.equals("--dirisnotalbumname")) {
       	 td.dirisalbumname=false;
      }
      else if(arg.equals("--dirisalbum")) {
      	td.dirisalbum=true;
      }
      else if(arg.equals("--dontshowduplicates")) {
      	td.showduplicates=false;
      }
      else if(arg.equals("--strip")) {
        if (i < args.length)
          td.strip = args[i++];
        else {
          System.err.println("--strip requires a path");
          System.exit(0);
        }
      }
      else if(arg.equals("--add")) {
        if (i < args.length)
          td.add = args[i++];
        else {
          System.err.println("--add requires a path");
          System.exit(0);
        }
      }
      else {
      	if(path!=null) {
          System.err.println("you can't specify more than one path!");
          System.exit(0);
        }
      	path = arg;
      }
    }
    if (i != args.length||path==null) {
			System.out.println("Usage: SongDB [--showduplicates] [--strip <directory>] [--add <directory>] [--dirisnotalbumname] [--dirisalbum] <directory>");
			return;
		}
		if(tdfile.exists()&&!tdfile.canWrite()) {
			System.out.println("rockbox.tagdb is not writable.");
			return;
		}
		try {
			tdfile.createNewFile();
		}
		catch(Exception e) {
			System.out.println("Error while trying to create rockbox.tagdb: "+e.getMessage());
			return;
		}
		td.add(new File(path));
		try {
			td.prepareWrite();
		//	rd.prepareWrite();
			td.writeDatabase(new File("rockbox.tagdb"));
		//	rd.writeDatabase(new File("rockbox.rundb"));
		}
		catch(IOException e) {
			System.out.println(e);
		}
	}
}