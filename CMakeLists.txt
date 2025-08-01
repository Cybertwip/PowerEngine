set(BUILD_SHARED_LIBS OFF)

cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0079 NEW)


set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum iOS deployment version" FORCE)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

if (NOT POWER_ENGINE_DISTRIBUTION)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

project(
  PowerEngine
  LANGUAGES CXX
  VERSION 0.1.0)

if (APPLE)
    enable_language(OBJCXX)
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_WARN_DEPRECATED FALSE)


# for ninja(unix)
if (UNIX AND NOT APPLE)
    set(CMAKE_MAKE_PROGRAM /usr/bin/ninja CACHE FILEPATH "")
endif()

#  for apple ccache
if(APPLE)
  get_property(RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
  if(RULE_LAUNCH_COMPILE AND CMAKE_GENERATOR STREQUAL "Xcode")
      # Set up wrapper scripts
      configure_file(launch-c.in launch-c)
      configure_file(launch-cxx.in launch-cxx)
      execute_process(COMMAND chmod a+rx
                               "${CMAKE_BINARY_DIR}/launch-c"
                               "${CMAKE_BINARY_DIR}/launch-cxx"
      )
      # Set Xcode project attributes to route compilation and linking
      # through our scripts
      set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-c")
      set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-cxx")
      set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-c")
      set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx")
  endif()
endif()


# for output directory
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(!APPLE)
  # file(COPY py_module DESTINATION ${CMAKE_BINARY_DIR}/bin)
  file(COPY LICENSE.md DESTINATION ${CMAKE_BINARY_DIR}/bin)
endif()
message("==========================================================")

# for external option
option(JSONCPP_WITH_TESTS OFF)
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES ON)

if(APPLE)
  set(NANOGUI_BACKEND "Metal" CACHE STRING "Metal Backend")
else()
  set(NANOGUI_BACKEND "OpenGL" CACHE STRING "OpenGL Backend")
endif()


set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Disable Flatbuffers Tests" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Disable Tests" FORCE)


# add external libraries
add_subdirectory(external/assimp/) 
add_subdirectory(external/cpp-peglib/)
add_subdirectory(external/entt/)
add_subdirectory(external/flatbuffers)
add_subdirectory(external/glad/)
add_library(STB_IMAGE external/stb/stb_image.cpp)
add_subdirectory(external/glfw/)
add_subdirectory(external/jsoncpp)
add_subdirectory(external/libriscv/)
add_subdirectory(external/glm/)
add_subdirectory(external/imgui-feature-layout/)
add_subdirectory(external/imgui-node-editor/)
add_subdirectory(external/cpp-httplib/)
add_subdirectory(external/zstr/)
add_subdirectory(external/zlib/)
add_subdirectory(external/nanogui/)
add_subdirectory(external/tinygizmo/) 
add_subdirectory(external/websocketpp/) 
add_subdirectory(external/refl-cpp/) 
add_subdirectory(external/reflecto/)


add_subdirectory(src/power)

target_link_libraries(nanogui PUBLIC glad glfw)

target_link_libraries(imgui-feature-layout glm json)


if(APPLE)
  target_compile_definitions(imgui-feature-layout PUBLIC -DDEFAULT_CWD="${CMAKE_CURRENT_LIST_DIR}/Resources")
endif()
