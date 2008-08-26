ADD_EXECUTABLE(zen_crypt main.cpp)

SET_SOURCE_FILES_PROPERTIES(main.cpp PROPERTIES COMPILE_FLAGS -Wno-multichar)

TARGET_LINK_LIBRARIES(zen_crypt shared)
TARGET_LINK_LIBRARIES(zen_crypt beecrypt)
