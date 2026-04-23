cmake_minimum_required(VERSION 3.24)
project(parse VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Network)

set(SOURCES
    src/main.cpp
    src/SingleInstance.cpp
    src/HotkeyManager.cpp
    src/RegionSelector.cpp
    src/Capturer.cpp
    src/OpenAIClient.cpp
    src/ResultPopup.cpp
    src/AppController.cpp
)

set(HEADERS
    src/SingleInstance.h
    src/HotkeyManager.h
    src/RegionSelector.h
    src/Capturer.h
    src/OpenAIClient.h
    src/ResultPopup.h
    src/AppController.h
)

qt_add_executable(parse WIN32
    ${SOURCES}
    ${HEADERS}
    resources/parse.qrc
    app.manifest
)

target_link_libraries(parse PRIVATE
    Qt6::Widgets
    Qt6::Network
    Gdi32
    User32
)

set_target_properties(parse PROPERTIES
    WIN32_EXECUTABLE ON
    OUTPUT_NAME "parse"
)

if(MSVC)
    target_compile_options(parse PRIVATE /W4 /permissive- /EHsc)
    target_compile_definitions(parse PRIVATE
        UNICODE _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()
