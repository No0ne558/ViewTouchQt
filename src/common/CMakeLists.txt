# src/common/CMakeLists.txt
# Shared protocol library used by both server and client.

add_library(vt_common STATIC
    Protocol.cpp
)

target_include_directories(vt_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vt_common PUBLIC
    Qt6::Network
)
