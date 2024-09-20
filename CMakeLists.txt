set(BUILD_SHARED_LIBS OFF)

cmake_minimum_required(VERSION 3.21)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

if (NOT POWER_ENGINE_DISTRIBUTION)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

if(APPLE)
project(
  PowerEngine
  LANGUAGES CXX OBJC
  VERSION 0.1.0)
else()
project(
  PowerEngine
  LANGUAGES CXX
  VERSION 0.1.0)
endif()



set(CMAKE_CXX_STANDARD 20)

set(CMAKE_WARN_DEPRECATED FALSE)


set(gRPC_INSTALL OFF CACHE BOOL "" FORCE)
set(gRPC_ABSL_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_CARES_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_PROTOBUF_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_RE2_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_SSL_PROVIDER "module" CACHE STRING "" FORCE)
set(gRPC_ZLIB_PROVIDER "module" CACHE STRING "" FORCE)
set(protobuf_INSTALL OFF CACHE STRING "" FORCE)


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
  file(COPY Resources DESTINATION ${CMAKE_BINARY_DIR}/bin)
  # file(COPY py_module DESTINATION ${CMAKE_BINARY_DIR}/bin)
  file(COPY imgui.ini DESTINATION ${CMAKE_BINARY_DIR}/bin)
  file(COPY LICENSE DESTINATION ${CMAKE_BINARY_DIR}/bin)
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


# add external libraries
add_subdirectory(external/entt/)
add_subdirectory(external/glad/)
add_library(STB_IMAGE external/stb/stb_image.cpp)
add_subdirectory(external/glfw/)
add_subdirectory(external/jsoncpp)
add_subdirectory(external/glm/)
add_subdirectory(external/grpc/)
add_subdirectory(external/smallfbx/)
add_subdirectory(external/imgui-feature-layout/)
add_subdirectory(external/imgui-node-editor/)
add_subdirectory(external/cpp-httplib/)
add_subdirectory(external/zstr/)
add_subdirectory(external/nanogui/)
add_subdirectory(external/tinygizmo/) 

add_subdirectory(src/power)

target_link_libraries(nanogui PUBLIC glad glfw)

#target_link_libraries(application nanogui entt)

target_link_libraries(imgui-feature-layout glm json)
#target_link_libraries(test PUBLIC imgui-feature-layout imgui-node-editor httplib::httplib zstr::zstr)
#target_link_libraries(glcpp PUBLIC test)
#target_link_libraries(SmallFBX zlib)


if(APPLE)
target_compile_definitions(imgui-feature-layout PUBLIC -DDEFAULT_CWD="${CMAKE_CURRENT_LIST_DIR}/Resources")
#  target_compile_definitions(application PUBLIC -DDEFAULT_CWD="${CMAKE_CURRENT_LIST_DIR}/Resources")
endif()
