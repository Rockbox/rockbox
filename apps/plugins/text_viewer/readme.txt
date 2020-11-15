About the text viewer plugin.

Limitation
    for targets where PLUGIN_BUFFER_SIZE < 0x13000,
    Only 999 pages can be read.
    


Difference between viewer.rock
    [settings file]
       - the global setting, 'tv_global.dat' is stored.
       - Settings and bookmarks for each file, 'tv_file.dat' is stored.

      Note: when viewer.dat(viewer_file.dat) exists, tv_global.dat(tv_file.dat) is created by
            using viewer.dat(viewer_file.dat).

    [word wrap]
          - add the following characters which can be split the line.
                '!', ',', '.', ':', ';', '?', 
                U+00b7, U+2010, U+3000, U+3001, U+3002, U+30fb, U+30fc, 
                U+ff01, U+ff0c, U+ff0d, U+ff0e, U+ff1a, U+ff1b, U+ff1f.

          - when the line split, if the line length is short ( < 0.75 * display width), 
            split the line in display width. (thus, maybe split a word)

    [line mode]
       [join]
           - break line condition has changed.
               - If the next line is a blank line or spaces only line, this line breaks.

       [reflow]
           - indent changes is two spaces (changable in the settings).

    [bookmark]
           - increased the number of bookmarks that can be registered to 16.


TODO list
  - for the target which PLUGIN_BUFFER_SIZE < 0x13000,   
    support more than 999 pages of text.

  - add History feature.

  - draw images that are linked to the text. (<img src="...">)

  - play audios that are linked to the text. (<audio src="...">)

  - more treatments of line breaking, word wrappings.
    (for example, period does not appear the top of line.)
