PROJECT(shared)

# source files for shared
SET(shared_srcs
  cenc.cpp
  crypt.cpp
  file.cpp
  firmware.cpp
  pe.cpp
  updater.cpp
  utils.cpp
)

ADD_LIBRARY(shared ${shared_srcs})
TARGET_LINK_LIBRARIES(shared pelib)
TARGET_LINK_LIBRARIES(shared zlib)
