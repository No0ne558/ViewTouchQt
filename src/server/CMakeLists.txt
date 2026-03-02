# src/server/CMakeLists.txt
# Headless POS server.

add_executable(vt_server
    main.cpp
    PosServer.cpp
    ClientSession.cpp
)

target_link_libraries(vt_server PRIVATE
    vt_common
    Qt6::Network
)

install(TARGETS vt_server RUNTIME DESTINATION bin)
