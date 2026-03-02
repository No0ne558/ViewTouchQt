# src/client/CMakeLists.txt
# Fullscreen POS terminal client.

add_executable(vt_client
    main.cpp
    PosClient.cpp
    MainWindow.cpp
    ButtonItem.cpp
)

target_link_libraries(vt_client PRIVATE
    vt_common
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS vt_client RUNTIME DESTINATION bin)
