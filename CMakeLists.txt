cmake_minimum_required(VERSION 3.5)

project(mandelbrot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Concurrent REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Concurrent REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        graphic_engine.cpp
        graphic_engine.h
        core.cpp
        core.h
        processing_area.cpp
        processing_area.h
        colors.h
        colors.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(mandelbrot
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

endif()

target_link_libraries(mandelbrot PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(mandelbrot PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(mandelbrot)
endif()
