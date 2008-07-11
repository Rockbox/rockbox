PeLib - Version 0.09 (alpha release)
=========================================

Copyright 2004 by Sebastian Porst
WWW: http://www.pelib.com
E-Mail: webmaster@the-interweb.com

=========================================

1. What is PeLib?
2. Where can I find a documentation of PeLib DLL?
3. Which license is used for PeLib?
4. Which compilers are being supported?
5. How do I compile PeLib?

1. What is PeLib DLL?
   PeLib is an open-source C++ library to modify
   PE files. See http://www.pelib.com for more details.

2. Where can I find a documentation of PeLib DLL?
   http://www.pelib.com

3. All parts of PeLib are distributed under the zlib/libpng license.
   See license.htm for details.
   
4. The following compilers have been tested:
   MingW with g++ 3.2.3
   Visual C++ 7.1 / Compiler version 13.10.3052
   Borland C++ 5.6.4 (currently not supported)
   Digital Mars Compiler 8.38n (currently not supported)

5. Go into the PeLib/source directory and enter the following lines
   depending on which compiler you use.
   
   g++: make -f makefile.g++
   Borland C++: make -f makefile.bcc (currently not supported)
   Visual C++ 7.1: nmake makefile.vc7
   Digital Mars: make makefile.dmc (currently not supported)
   
   If the compilation is succesful there should be some *.o/*.obj files
   and (if you used g++) a PeLib.a file in the lib directory.
   Then go to the examples directory and pick one example (I
   suggest FileDump) and try to build it with the same make
   command as above.