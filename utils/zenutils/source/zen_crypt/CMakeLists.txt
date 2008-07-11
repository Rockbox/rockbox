ADD_EXECUTABLE(zen_crypt main.cpp)

TARGET_LINK_LIBRARIES(zen_crypt shared)
TARGET_LINK_LIBRARIES(zen_crypt beecrypt)
