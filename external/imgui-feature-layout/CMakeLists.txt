cmake_minimum_required(VERSION 3.12)

# See https://cmake.org/cmake/help/latest/policy/CMP0054.html
cmake_policy(SET CMP0054 NEW)

# Makes a hidden visibility preset on a static lib respected
# This is used to hide glfw's symbols from the library exports when building an so/dylib
# See https://cmake.org/cmake/help/latest/policy/CMP0063.html
cmake_policy(SET CMP0063 NEW)
cmake_policy(SET CMP0079 NEW)


if(APPLE)
    project(imgui-feature-layout LANGUAGES C CXX OBJC)
else()
    project(imgui-feature-layout LANGUAGES C CXX)
endif()

# Define IMGUI_NODE_EDITOR_ROOT_DIR pointing to project root directory
set(IMGUI_FEATURE_LAYOUT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/ CACHE STRINg "${CMAKE_CURRENT_LISTS_DIR}")

# Enable solution folders in Visual Studio and Folders in Xcode
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Point CMake where to look for module files.
list(APPEND CMAKE_MODULE_PATH ${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/misc/cmake-modules)

# Node editor use C++14
set(CMAKE_CXX_STANDARD            20)
set(CMAKE_CXX_STANDARD_REQUIRED   YES)

set(IMGUI_ICONS
        icons/icons.cpp
        icons/folder.cpp
        icons/fontaudio.cpp
        icons/fa_brands.cpp
        icons/fa_regular.cpp
        icons/fa_solid.cpp
        icons/webfont.cpp
        icons/fork_webfont.cpp
        icons/material_icons.cpp
        icons/kenney.cpp
        icons/StepMath.cpp
    )

add_library(imgui-feature-layout STATIC
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/imgui.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/imgui_draw.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/imgui_tables.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/imgui_widgets.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/TextEditor.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/ImSequencer.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/ImGuiFileDialog/ImGuiFileDialog.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/imgui_neo_sequencer.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/imgui_neo_internal.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/ImCurveEdit.cpp
${IMGUI_FEATURE_LAYOUT_ROOT_DIR}/ImGuizmo.cpp

${IMGUI_ICONS}
)

target_include_directories(imgui-feature-layout PUBLIC ${IMGUI_FEATURE_LAYOUT_ROOT_DIR})
