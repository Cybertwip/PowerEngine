cmake_minimum_required(VERSION 3.10)

# Set the project name
project(tinygizmo)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_library(tinygizmo tiny-gizmo.cpp)

# Include directories
target_include_directories(tinygizmo PUBLIC ${CMAKE_CURRENT_LIST_DIR})