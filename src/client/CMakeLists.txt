# src/client/CMakeLists.txt
# Fullscreen POS terminal client.

add_executable(vt_client
    main.cpp
    PosClient.cpp
    MainWindow.cpp
    layout/UiElement.cpp
    layout/ButtonElement.cpp
    layout/LabelElement.cpp
    layout/PanelElement.cpp
    layout/PageWidget.cpp
    layout/LayoutEngine.cpp
)

target_include_directories(vt_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vt_client PRIVATE
    vt_common
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS vt_client RUNTIME DESTINATION bin)
