# src/host/CMakeLists.txt
# Combined POS host: server + main client in a single process.

add_executable(vt_host
    main.cpp
)

# Re-use server and client sources via their object libraries.
# We link against the common protocol lib, Qt Widgets (for the GUI), and
# Qt Network (for the server).  We also need the actual source files from
# the server and client directories.
target_sources(vt_host PRIVATE
    # Server sources
    ${CMAKE_SOURCE_DIR}/src/server/PosServer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/ClientSession.cpp
    # Client sources
    ${CMAKE_SOURCE_DIR}/src/client/PosClient.cpp
    ${CMAKE_SOURCE_DIR}/src/client/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/client/layout/UiElement.cpp
    ${CMAKE_SOURCE_DIR}/src/client/layout/ButtonElement.cpp
    ${CMAKE_SOURCE_DIR}/src/client/layout/LabelElement.cpp
    ${CMAKE_SOURCE_DIR}/src/client/layout/PanelElement.cpp
    ${CMAKE_SOURCE_DIR}/src/client/layout/PageWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/client/layout/LayoutEngine.cpp
    # Editor sources
    ${CMAKE_SOURCE_DIR}/src/client/editor/EditorOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/client/editor/PropertyDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/client/editor/LayoutSerializer.cpp
    ${CMAKE_SOURCE_DIR}/src/client/editor/PageManagerDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/client/editor/PageTabBar.cpp
)

target_include_directories(vt_host PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/client
)

target_link_libraries(vt_host PRIVATE
    vt_common
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS vt_host RUNTIME DESTINATION bin)
