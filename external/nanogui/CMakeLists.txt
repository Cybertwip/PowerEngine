cmake_minimum_required(VERSION 3.5)

#============================================================================
# Project Definition
#============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Extract project version from the header to keep it in one place
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/nanogui/common.h"
     nanogui_version_defines REGEX "#define NANOGUI_VERSION_(MAJOR|MINOR|PATCH) ")

foreach(ver ${nanogui_version_defines})
    if(ver MATCHES [[#define NANOGUI_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$]])
        set(NANOGUI_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()

set(NANOGUI_VERSION "${NANOGUI_VERSION_MAJOR}.${NANOGUI_VERSION_MINOR}.${NANOGUI_VERSION_PATCH}")

project(nanogui LANGUAGES CXX C VERSION ${NANOGUI_VERSION})

message(STATUS "Configuring NanoGUI v${NANOGUI_VERSION}")

#============================================================================
# Backend Selection (OpenGL / Metal)
#============================================================================

# Auto-detect a default backend if not specified by the user
if(NOT NANOGUI_BACKEND)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(NANOGUI_BACKEND_DEFAULT "GLES 2")
    elseif(APPLE)
        set(NANOGUI_BACKEND_DEFAULT "Metal")
    else()
        set(NANOGUI_BACKEND_DEFAULT "OpenGL")
    endif()
    set(NANOGUI_BACKEND ${NANOGUI_BACKEND_DEFAULT} CACHE STRING
        "Rendering backend (OpenGL/GLES 2/GLES 3/Metal)" FORCE)
endif()
set_property(CACHE NANOGUI_BACKEND PROPERTY STRINGS "OpenGL" "GLES 2" "GLES 3" "Metal")

# Configure sources and definitions based on the selected backend
if(NANOGUI_BACKEND MATCHES "(OpenGL|GLES 2|GLES 3)")
    list(APPEND NANOGUI_EXTRA_SOURCES
        src/texture_gl.cpp src/shader_gl.cpp src/renderpass_gl.cpp src/opengl.cpp)
endif()

if(NANOGUI_BACKEND STREQUAL "OpenGL")
    set(NANOGUI_BACKEND_DEFS -DNANOGUI_USE_OPENGL)
    set(NANOGUI_RESOURCE_GLOB "resources/*.gl")
    message(STATUS "NanoGUI: Using OpenGL backend.")
elseif(NANOGUI_BACKEND STREQUAL "GLES 2")
    set(NANOGUI_BACKEND_DEFS -DNANOGUI_USE_GLES -DNANOGUI_GLES_VERSION=2)
    set(NANOGUI_RESOURCE_GLOB "resources/*.gles")
    message(STATUS "NanoGUI: Using GLES 2 backend.")
elseif(NANOGUI_BACKEND STREQUAL "GLES 3")
    set(NANOGUI_BACKEND_DEFS -DNANOGUI_USE_GLES -DNANOGUI_GLES_VERSION=3)
    set(NANOGUI_RESOURCE_GLOB "resources/*.gles")
    message(STATUS "NanoGUI: Using GLES 3 backend.")
elseif(NANOGUI_BACKEND STREQUAL "Metal")
    set(NANOGUI_BACKEND_DEFS -DNANOGUI_USE_METAL)
    list(APPEND NANOGUI_EXTRA_SOURCES
        ext/nanovg_metal/src/nanovg_mtl.m src/texture_metal.mm
        src/shader_metal.mm src/renderpass_metal.mm)
    set(NANOGUI_RESOURCE_GLOB "resources/*.metal")
    include_directories(ext/nanovg_metal/src)
    message(STATUS "NanoGUI: Using Metal backend.")
endif()

#============================================================================
# System-Specific Dependencies
#============================================================================

if(WIN32)
    list(APPEND NANOGUI_LIBS opengl32)
elseif(APPLE)
    enable_language(OBJCXX)
    add_compile_options(-fobjc-arc) # Use Automatic Reference Counting
    list(APPEND NANOGUI_EXTRA_SOURCES src/darwin.mm src/autorelease.mm)
    set_property(SOURCE src/autorelease.mm PROPERTY COMPILE_FLAGS -fno-objc-arc)

    find_library(COCOA_LIB Cocoa REQUIRED)
    find_library(IOKIT_LIB IOKit REQUIRED)
    list(APPEND NANOGUI_LIBS ${COCOA_LIB} ${IOKIT_LIB})

    if(NANOGUI_BACKEND STREQUAL "OpenGL")
        find_library(OPENGL_LIB OpenGL REQUIRED)
        add_definitions(-DGL_SILENCE_DEPRECATION)
        list(APPEND NANOGUI_LIBS ${OPENGL_LIB})
    else() # Metal
        find_library(METAL_LIB Metal REQUIRED)
        find_library(QUARTZCORE_LIB QuartzCore REQUIRED)
        list(APPEND NANOGUI_LIBS ${METAL_LIB} ${QUARTZCORE_LIB})
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
    list(APPEND NANOGUI_LIBS X11 pthread dl rt)
    if(NANOGUI_BACKEND STREQUAL "OpenGL")
        list(APPEND NANOGUI_LIBS GL)
    elseif(NANOGUI_BACKEND STREQUAL "GLES 2")
        list(APPEND NANOGUI_LIBS GLESv2)
    elseif(NANOGUI_BACKEND STREQUAL "GLES 3")
        list(APPEND NANOGUI_LIBS GLESv3)
    endif()
endif()

#============================================================================
# Resource Embedding
#============================================================================

file(GLOB resources
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ttf"
    "${NANOGUI_RESOURCE_GLOB}")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")

# Precompile Metal shaders to .metallib files if using the Metal backend
foreach(fname_in IN LISTS resources)
    if(NANOGUI_BACKEND STREQUAL "Metal" AND fname_in MATCHES "\\.metal$")
        get_filename_component(fname_out ${fname_in} NAME)
        set(fname_out "${CMAKE_CURRENT_BINARY_DIR}/resources/${fname_out}lib")
        add_custom_command(
            OUTPUT ${fname_out}
            DEPENDS ${fname_in}
            COMMAND xcrun -sdk macosx metal -std=osx-metal2.0 -O3 "${fname_in}" -o "${fname_out}"
            COMMENT "Compiling ${fname_in}" VERBATIM)
    else()
        set(fname_out "${fname_in}")
    endif()
    list(APPEND resources_processed ${fname_out})
endforeach()

# Run bin2c to embed all resource files into C++ source
string(REPLACE ";" "," resources_string "${resources_processed}")
add_custom_command(
    OUTPUT nanogui_resources.cpp nanogui_resources.h
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_C=nanogui_resources.cpp
        -DOUTPUT_H=nanogui_resources.h
        "-DINPUT_FILES=${resources_string}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/resources/bin2c.cmake"
    DEPENDS ${resources} ${resources_processed}
    COMMENT "Embedding resources into C++ source" VERBATIM)

#============================================================================
# Library Target
#============================================================================

# Respect the standard BUILD_SHARED_LIBS variable to build as static or shared
set(NANOGUI_LIBRARY_TYPE STATIC)

add_library(nanogui ${NANOGUI_LIBRARY_TYPE}
    # Sources from dependencies
    ext/nanovg/src/nanovg.c
    # Generated and backend-specific sources
    nanogui_resources.cpp
    ${NANOGUI_EXTRA_SOURCES}
    # Core NanoGUI sources
    src/common.cpp src/widget.cpp src/theme.cpp src/layout.cpp
    src/screen.cpp src/label.cpp src/window.cpp src/popup.cpp
    src/checkbox.cpp src/button.cpp src/popupbutton.cpp src/combobox.cpp
    src/progressbar.cpp src/slider.cpp src/messagedialog.cpp src/textbox.cpp
    src/textarea.cpp src/treeview.cpp src/treeviewitem.cpp src/imagepanel.cpp
    src/vscrollpanel.cpp src/colorwheel.cpp src/colorpicker.cpp src/graph.cpp
    src/tabwidget.cpp src/canvas.cpp src/texture.cpp src/shader.cpp
    src/imageview.cpp src/traits.cpp
)

target_compile_definitions(nanogui
    PUBLIC
        ${NANOGUI_BACKEND_DEFS}
    PRIVATE
        -DNANOGUI_BUILD
        -DNVG_STB_IMAGE_IMPLEMENTATION
)

target_include_directories(nanogui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nanovg>
    PRIVATE
        ext/nanovg/src
        ${CMAKE_CURRENT_BINARY_DIR} # For nanogui_resources.h
)

# Raspberry Pi GLES include path fix
if(EXISTS /opt/vc/include)
    target_include_directories(nanogui PUBLIC /opt/vc/include)
endif()

# Note: The 'json' target is an expected dependency that must be provided
# by the consuming project (e.g., via find_package or add_subdirectory).
target_link_libraries(nanogui PUBLIC ${NANOGUI_LIBS} json)

# Set RPATH for shared library builds on Unix-like systems for correct linking
if(BUILD_SHARED_LIBS)
    set_property(TARGET nanogui PROPERTY BUILD_WITH_INSTALL_RPATH TRUE)
    if(APPLE)
        set_property(TARGET nanogui PROPERTY INSTALL_RPATH "@loader_path")
    elseif(UNIX)
        set_property(TARGET nanogui PROPERTY INSTALL_RPATH "$ORIGIN")
    endif()
endif()