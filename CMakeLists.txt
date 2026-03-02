cmake_minimum_required(VERSION 3.16)

project(ViewTouchQt
    VERSION 0.1.0
    DESCRIPTION "Modern C++ Restaurant POS System"
    LANGUAGES CXX
)

# ----- Global settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)          # Qt meta-object compiler
set(CMAKE_AUTORCC ON)          # Qt resource compiler

# ----- Find Qt 6 -------------------------------------------------------------
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)

# ----- Subdirectories ---------------------------------------------------------
add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)
add_subdirectory(src/host)

# ----- CPack (deb / rpm) -----------------------------------------------------
set(CPACK_PACKAGE_VENDOR "OrderPi LLC")
set(CPACK_PACKAGE_CONTACT "arielbrambila117@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ViewTouchQt Restaurant POS System")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Debian
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base (>= 6.5)")
set(CPACK_DEBIAN_PACKAGE_SECTION "misc")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.5")

include(CPack)
