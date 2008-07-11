PROJECT(pelib)

# source files for pelib
SET(pelib_srcs
  pelib/BoundImportDirectory.cpp
  pelib/ComHeaderDirectory.cpp
  pelib/DebugDirectory.cpp
  pelib/ExportDirectory.cpp
  pelib/IatDirectory.cpp
  pelib/MzHeader.cpp
  pelib/PeFile.cpp
  pelib/PeHeader.cpp
  pelib/PeLibAux.cpp
  pelib/RelocationsDirectory.cpp
  pelib/ResourceDirectory.cpp
  pelib/buffer/InputBuffer.cpp
  pelib/buffer/OutputBuffer.cpp
)

ADD_LIBRARY(pelib ${pelib_srcs})
