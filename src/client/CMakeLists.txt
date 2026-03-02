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
    layout/PinEntryElement.cpp
    layout/KeypadButtonElement.cpp
    layout/ActionButtonElement.cpp
    layout/InfoLabelElement.cpp
    layout/PageWidget.cpp
    layout/LayoutEngine.cpp
    editor/EditorOverlay.cpp
    editor/PropertyDialog.cpp
    editor/LayoutSerializer.cpp
    editor/PageManagerDialog.cpp
    editor/PageTabBar.cpp
    printing/CupsPrinter.cpp
    displays/DisplayManager.cpp
)

# Find CUPS (required for printer discovery and test printing)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CUPS REQUIRED cups)

target_include_directories(vt_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CUPS_INCLUDE_DIRS}
)

target_link_libraries(vt_client PRIVATE
    vt_common
    Qt6::Widgets
    Qt6::Network
    ${CUPS_LIBRARIES}
)

install(TARGETS vt_client RUNTIME DESTINATION bin)
