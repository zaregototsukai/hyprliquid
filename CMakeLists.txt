cmake_minimum_required(VERSION 3.19)
project(hyprliquid)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(Stb_INCLUDE_DIR NAMES stb_image.h PATH_SUFFIXES stb)
if (NOT Stb_INCLUDE_DIR)
    find_package(Stb REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(HYPRLAND REQUIRED hyprland)
pkg_check_modules(SYSTEMD REQUIRED systemd)

find_program(HYPRWAYLAND_SCANNER hyprwayland-scanner)
if (HYPRWAYLAND_SCANNER)
    set(PROTOCOL_SRC_PATH ${CMAKE_BINARY_DIR}/Protocols)
    set(PROTOCOL_XML ${CMAKE_SOURCE_DIR}/src/Protocols/background-share-unstable-v1.xml)
    set(PROTOCOL_HPP ${PROTOCOL_SRC_PATH}/background-share-unstable-v1.hpp)
    set(PROTOCOL_CPP ${PROTOCOL_SRC_PATH}/background-share-unstable-v1.cpp)
    if (NOT EXISTS ${PROTOCOL_SRC_PATH})
        make_directory(${PROTOCOL_SRC_PATH})
    endif()
    add_custom_command(OUTPUT ${PROTOCOL_HPP} ${PROTOCOL_CPP}
        COMMAND hyprwayland-scanner "${PROTOCOL_XML}" ${PROTOCOL_SRC_PATH}
        DEPENDS ${PROTOCOL_XML}
        VERBATIM)
else()
    set(PROTOCOL_SRC_PATH ${CMAKE_SOURCE_DIR}/generated/Protocols)
    set(PROTOCOL_CPP ${PROTOCOL_SRC_PATH}/background-share-unstable-v1.cpp)
endif()

add_library(hyprliquid MODULE
    src/Shaders/Shaders.h src/Shaders/Shaders.cpp
    src/Hooks/Hooks.h src/Hooks/Hooks.cpp
    src/Protocols/BackgroundShareProtocol.h src/Protocols/BackgroundShareProtocol.cpp
    ${PROTOCOL_CPP}
    src/Render/Pass/BackgroundSharePassElement.cpp
    src/Render/Render.cpp
    src/Render/LiquidGlass.cpp
    src/Render/Acrylic.cpp
    src/Render/Mica.cpp
    src/Render/Aero.cpp
    src/Context/ClientContext.cpp
    src/Context/MonitorContext.cpp
    src/Config/RuleOrConfigValue.hpp
    src/Config/RuleOrDefaultValue.hpp
    src/Config/ConfigManager.cpp
    src/Utils/Utils.hpp src/Utils/Utils.cpp
    src/Utils/BackgroundManager.cpp
    src/Utils/ColorSchemeHelper.cpp
    src/main.cpp
)

target_include_directories(hyprliquid PRIVATE
    include
    src
    ${HYPRLAND_INCLUDE_DIRS}
    ${PROTOCOL_SRC_PATH}
    ${Stb_INCLUDE_DIR}
)

target_link_libraries(hyprliquid PRIVATE
    ${HYPRLAND_LIBRARIES}
    ${SYSTEMD_LIBRARIES}
)

install(TARGETS hyprliquid LIBRARY DESTINATION /usr/lib)
