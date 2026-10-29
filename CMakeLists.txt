cmake_minimum_required(VERSION 3.21)

# Oldest macOS the app should run on. Must be set before project().
# Without it clang targets the macOS version of the build machine, so an app
# built on Sequoia would refuse to start on Sonoma or Monterey.
# Note: Qt itself also has a minimum (Qt 6.5 → macOS 11, Qt 6.12 → macOS 14.4);
# the app runs only where both are satisfied.
if(CMAKE_HOST_APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()

project(Svitlo VERSION 0.1.0 LANGUAGES CXX)

# Svitlo — a free, open-source photo library and non-destructive photo editor.
# Target: macOS 12 Monterey first (Qt 6.5 LTS), Linux and Windows later.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SVITLO_WITH_LIBRAW "Enable RAW support via LibRaw" ON)
option(SVITLO_BUILD_TESTS "Build the core tests" ON)

# On macOS LibRaw is downloaded and built together with the app by default,
# so no Homebrew is needed and the app runs on older macOS versions too.
# On Linux the system package (libraw-dev) is used by default.
if(APPLE)
    set(_bundled_default ON)
else()
    set(_bundled_default OFF)
endif()
option(SVITLO_BUNDLED_LIBRAW "Download and build LibRaw instead of using an installed one"
       ${_bundled_default})

# Avoid "framework 'AGL' not found" with older Qt and new Xcode SDKs.
if(APPLE)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos-agl-fix")
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent)
# Optional: needed to compile the interface translations (.ts → .qm).
find_package(Qt6 6.4 QUIET COMPONENTS LinguistTools)

# ---------------------------------------------------------------- LibRaw ----
set(SVITLO_HAVE_LIBRAW OFF)
if(SVITLO_WITH_LIBRAW AND SVITLO_BUNDLED_LIBRAW)
    include(cmake/BundledLibRaw.cmake)
    set(SVITLO_HAVE_LIBRAW ON)
    set(SVITLO_LIBRAW_TARGET svitlo_libraw)
elseif(SVITLO_WITH_LIBRAW)
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
        set(SVITLO_LIBRAW_TARGET PkgConfig::LIBRAW)
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
    target_link_libraries(svitlo_core PRIVATE ${SVITLO_LIBRAW_TARGET})
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
    src/ui/Language.h                src/ui/Language.cpp
    src/ui/SupportDialog.h           src/ui/SupportDialog.cpp
    src/ui/LicensesDialog.h          src/ui/LicensesDialog.cpp
)

# App icon: Svitlo.icns goes into the bundle's Resources folder (Finder/Dock),
# svitlo.png is compiled in via Qt resources (window icon on Linux/Windows).
# Regenerate both with: python3 packaging/icon/make_icon.py
set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/Svitlo.icns")
set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

# Licence texts of the bundled LibRaw go into Svitlo.app/Contents/Resources/licenses.
set(APP_LICENSE_FILES "")
if(TARGET svitlo_libraw)
    foreach(_lic LICENSE.CDDL LICENSE.LGPL COPYRIGHT)
        set(_src "${libraw_src_SOURCE_DIR}/${_lic}")
        if(EXISTS "${_src}")
            set(_dst "${CMAKE_CURRENT_BINARY_DIR}/licenses/LibRaw-${_lic}.txt")
            configure_file("${_src}" "${_dst}" COPYONLY)
            list(APPEND APP_LICENSE_FILES "${_dst}")
        endif()
    endforeach()
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/licenses/Svitlo-LICENSE.txt" COPYONLY)
list(APPEND APP_LICENSE_FILES "${CMAKE_CURRENT_BINARY_DIR}/licenses/Svitlo-LICENSE.txt")
set_source_files_properties(${APP_LICENSE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/licenses")

add_executable(Svitlo MACOSX_BUNDLE ${APP_SOURCES} resources/resources.qrc ${APP_ICON_MACOS}
               ${APP_LICENSE_FILES})
target_link_libraries(Svitlo PRIVATE svitlo_core Qt6::Widgets Qt6::Sql)
target_compile_definitions(Svitlo PRIVATE SVITLO_VERSION="${PROJECT_VERSION}")

# ---------------------------------------------------------- translations ----
# Source language is English. Russian is intentionally not provided.
set(SVITLO_LANGUAGES uk da de pl)
set(SVITLO_TS_FILES "")
foreach(_lang ${SVITLO_LANGUAGES})
    list(APPEND SVITLO_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations/svitlo_${_lang}.ts")
endforeach()

if(TARGET Qt6::lrelease)
    # Compiles the .ts files and embeds them under :/i18n.
    # `cmake --build build --target update_translations` re-scans the code.
    qt_add_translations(Svitlo TS_FILES ${SVITLO_TS_FILES} RESOURCE_PREFIX "/i18n")

    # Qt's own strings (standard buttons, dialogs) for the same languages,
    # embedded under :/qt_i18n so the .app needs nothing extra.
    set(_qt_tr_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")
    set(_qt_qrc "${CMAKE_CURRENT_BINARY_DIR}/qt_translations.qrc")
    set(_qrc_body "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n<qresource prefix=\"/qt_i18n\">\n")
    foreach(_lang ${SVITLO_LANGUAGES})
        if(EXISTS "${_qt_tr_dir}/qtbase_${_lang}.qm")
            string(APPEND _qrc_body "<file alias=\"qtbase_${_lang}.qm\">${_qt_tr_dir}/qtbase_${_lang}.qm</file>\n")
        endif()
    endforeach()
    string(APPEND _qrc_body "</qresource>\n</RCC>\n")
    file(WRITE "${_qt_qrc}" "${_qrc_body}")
    target_sources(Svitlo PRIVATE "${_qt_qrc}")
else()
    message(WARNING "Svitlo: Qt LinguistTools not found — the app will be English only.")
endif()

set_target_properties(Svitlo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.svitlo"
    MACOSX_BUNDLE_BUNDLE_NAME "Svitlo"
    MACOSX_BUNDLE_ICON_FILE "Svitlo.icns"
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
