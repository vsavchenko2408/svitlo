cmake_minimum_required(VERSION 3.21)

project(Svitlo VERSION 0.1.0 LANGUAGES CXX)

# Svitlo — a free, open-source photo library and non-destructive photo editor.
# Target: macOS 12 Monterey first (Qt 6.5 LTS), Linux and Windows later.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SVITLO_WITH_LIBRAW "Enable RAW support via LibRaw" ON)
option(SVITLO_BUILD_TESTS "Build the core tests" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent)

# ---------------------------------------------------------------- LibRaw ----
set(SVITLO_HAVE_LIBRAW OFF)
if(SVITLO_WITH_LIBRAW)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        # libraw_r is the thread-safe flavour; we decode on worker threads.
        pkg_check_modules(LIBRAW IMPORTED_TARGET libraw_r)
        if(NOT LIBRAW_FOUND)
            pkg_check_modules(LIBRAW IMPORTED_TARGET libraw)
        endif()
    endif()
    if(LIBRAW_FOUND)
        set(SVITLO_HAVE_LIBRAW ON)
        message(STATUS "Svitlo: RAW support enabled (LibRaw ${LIBRAW_VERSION})")
    else()
        message(WARNING "Svitlo: LibRaw not found — building without RAW support "
                        "(JPEG/PNG/TIFF only). See README for how to install it.")
    endif()
endif()

# ----------------------------------------------------------------- core -----
# The core library has no widget dependencies: image loading, the processing
# pipeline, edit settings and export. It is shared by the app and the tests.
add_library(svitlo_core STATIC
    src/core/ColorMath.h
    src/core/ImageBuffer.h
    src/core/EditSettings.h      src/core/EditSettings.cpp
    src/core/ToneCurve.h         src/core/ToneCurve.cpp
    src/core/Geometry.h          src/core/Geometry.cpp
    src/core/ImageLoader.h       src/core/ImageLoader.cpp
    src/core/Pipeline.h          src/core/Pipeline.cpp
    src/core/Histogram.h         src/core/Histogram.cpp
    src/core/Exporter.h          src/core/Exporter.cpp
)
target_include_directories(svitlo_core PUBLIC src)
target_link_libraries(svitlo_core PUBLIC Qt6::Core Qt6::Gui Qt6::Concurrent)
if(SVITLO_HAVE_LIBRAW)
    target_compile_definitions(svitlo_core PUBLIC SVITLO_HAVE_LIBRAW=1)
    target_link_libraries(svitlo_core PRIVATE PkgConfig::LIBRAW)
endif()

# ------------------------------------------------------------------ app -----
set(APP_SOURCES
    src/main.cpp
    src/catalog/PhotoRecord.h
    src/catalog/Catalog.h            src/catalog/Catalog.cpp
    src/catalog/ThumbnailCache.h     src/catalog/ThumbnailCache.cpp
    src/ui/Theme.h                   src/ui/Theme.cpp
    src/ui/MainWindow.h              src/ui/MainWindow.cpp
    src/ui/PhotoModel.h              src/ui/PhotoModel.cpp
    src/ui/PhotoDelegate.h           src/ui/PhotoDelegate.cpp
    src/ui/LibraryView.h             src/ui/LibraryView.cpp
    src/ui/DevelopView.h             src/ui/DevelopView.cpp
    src/ui/ImageCanvas.h             src/ui/ImageCanvas.cpp
    src/ui/AdjustPanel.h             src/ui/AdjustPanel.cpp
    src/ui/SliderRow.h               src/ui/SliderRow.cpp
    src/ui/CollapsibleSection.h      src/ui/CollapsibleSection.cpp
    src/ui/ToneCurveWidget.h         src/ui/ToneCurveWidget.cpp
    src/ui/HistogramWidget.h         src/ui/HistogramWidget.cpp
    src/ui/ExportDialog.h            src/ui/ExportDialog.cpp
)

add_executable(Svitlo MACOSX_BUNDLE ${APP_SOURCES})
target_link_libraries(Svitlo PRIVATE svitlo_core Qt6::Widgets Qt6::Sql)
target_compile_definitions(Svitlo PRIVATE SVITLO_VERSION="${PROJECT_VERSION}")

set_target_properties(Svitlo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.svitlo"
    MACOSX_BUNDLE_BUNDLE_NAME "Svitlo"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# ---------------------------------------------------------------- tests -----
if(SVITLO_BUILD_TESTS)
    enable_testing()
    add_executable(svitlo_tests tests/test_core.cpp)
    target_link_libraries(svitlo_tests PRIVATE svitlo_core)
    add_test(NAME core COMMAND svitlo_tests)
    set_tests_properties(core PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
