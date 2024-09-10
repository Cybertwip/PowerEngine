set(BUILD_SHARED_LIBS OFF)

cmake_minimum_required(VERSION 3.21)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)



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

set(CMAKE_CXX_STANDARD 17)

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

set(NANOGUI_BACKEND "OpenGL" CACHE STRING "OpenGL Backend")



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
add_subdirectory(external/ozz/)
add_subdirectory(external/tinygizmo/) 

add_subdirectory(src/power)

target_link_libraries(nanogui PUBLIC glad glfw)
#target_link_libraries(ozz_animation PUBLIC glad glfw json)
#target_link_libraries(ozz_animation_tools json)

#target_link_libraries(application nanogui ozz_animation entt)

target_link_libraries(imgui-feature-layout glm json)
#target_link_libraries(test PUBLIC imgui-feature-layout imgui-node-editor httplib::httplib zstr::zstr)
#target_link_libraries(glcpp PUBLIC test)
#target_link_libraries(SmallFBX zlib)

if(APPLE)
target_compile_definitions(imgui-feature-layout PUBLIC -DDEFAULT_CWD="${CMAKE_CURRENT_LIST_DIR}/Resources")
#  target_compile_definitions(application PUBLIC -DDEFAULT_CWD="${CMAKE_CURRENT_LIST_DIR}/Resources")
endif()
