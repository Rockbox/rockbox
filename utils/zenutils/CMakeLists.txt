PROJECT(zenutils)

SET(EXECUTABLE_OUTPUT_PATH ${zenutils_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME})

SUBDIRS(source libraries)

INCLUDE_DIRECTORIES(
  ${zenutils_SOURCE_DIR}/libraries/beecrypt-4.1.2
  ${zenutils_SOURCE_DIR}/libraries/getpot-c++-1.1.17
  ${zenutils_SOURCE_DIR}/libraries/pelib-0.9
  ${zenutils_SOURCE_DIR}/libraries/zlib123
  ${zenutils_SOURCE_DIR}/source/shared
)
