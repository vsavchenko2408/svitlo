# Replacement for Qt's FindWrapOpenGL module, used on macOS only.
#
# Older Qt releases (e.g. 6.5.x, 6.8.3, 6.9.1) link the legacy "AGL"
# framework, which recent Xcode SDKs no longer ship. Linking then fails with
#     ld: framework 'AGL' not found
# This module provides the same WrapOpenGL::WrapOpenGL target but links only
# OpenGL.framework. Newer Qt versions (6.8.4+, 6.9.2+) fixed this upstream
# (QTBUG-137687).

if(TARGET WrapOpenGL::WrapOpenGL)
    set(WrapOpenGL_FOUND ON)
    return()
endif()

set(WrapOpenGL_FOUND OFF)
find_package(OpenGL ${WrapOpenGL_FIND_VERSION})

if(OpenGL_FOUND)
    set(WrapOpenGL_FOUND ON)
    add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
    if(APPLE)
        find_library(SVITLO_OPENGL_FRAMEWORK NAMES OpenGL)
        if(SVITLO_OPENGL_FRAMEWORK)
            target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE "${SVITLO_OPENGL_FRAMEWORK}")
        else()
            target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE "-framework OpenGL")
        endif()
    else()
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE OpenGL::GL)
    endif()
    message(STATUS "Svitlo: using OpenGL module without AGL")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenGL DEFAULT_MSG WrapOpenGL_FOUND)
