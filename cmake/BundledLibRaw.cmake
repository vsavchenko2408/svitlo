# Downloads LibRaw from GitHub and builds it as a static library together with
# Svitlo. Because it is compiled by this project, it uses the same compiler
# settings — in particular CMAKE_OSX_DEPLOYMENT_TARGET — so the finished app
# runs on the same macOS versions as Qt itself (e.g. macOS 12 with Qt 6.5),
# and the .app has no dependency on Homebrew.
#
# Built without LittleCMS and libjpeg (not needed for ordinary RAW files);
# zlib comes with macOS and enables floating-point DNG support.
#
# Defines the target `svitlo_libraw`.

include(FetchContent)

set(SVITLO_LIBRAW_TAG "0.22.3" CACHE STRING "LibRaw git tag to build")

FetchContent_Declare(libraw_src
    GIT_REPOSITORY https://github.com/LibRaw/LibRaw.git
    GIT_TAG        ${SVITLO_LIBRAW_TAG}
    GIT_SHALLOW    TRUE
)
# LibRaw has no CMakeLists.txt, so this only downloads the sources.
FetchContent_MakeAvailable(libraw_src)

file(GLOB_RECURSE _libraw_sources CONFIGURE_DEPENDS "${libraw_src_SOURCE_DIR}/src/*.cpp")
# *_ph.cpp are placeholder units that LibRaw's own build files do not compile.
list(FILTER _libraw_sources EXCLUDE REGEX "_ph\\.cpp$")

add_library(svitlo_libraw STATIC ${_libraw_sources})
target_include_directories(svitlo_libraw SYSTEM PUBLIC "${libraw_src_SOURCE_DIR}")
# LIBRAW_NODLL: static linking (only matters on Windows).
target_compile_definitions(svitlo_libraw PUBLIC LIBRAW_NODLL PRIVATE USE_ZLIB)
set_target_properties(svitlo_libraw PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LibRaw is third-party code; don't drown the build log in its warnings.
if(MSVC)
    target_compile_options(svitlo_libraw PRIVATE /w)
else()
    target_compile_options(svitlo_libraw PRIVATE -w)
endif()

find_package(ZLIB REQUIRED)
target_link_libraries(svitlo_libraw PRIVATE ZLIB::ZLIB)

message(STATUS "Svitlo: building bundled LibRaw ${SVITLO_LIBRAW_TAG} (static)")
