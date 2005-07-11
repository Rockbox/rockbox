javac -d classes -cp . -source 1.5 -target 1.5 `find -name '*.java'`
jar cvfm SongDB.jar classes/META-INF/MANIFEST.MF -C classes/ .
