cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recursively glob source files from src directory
file(GLOB AGNOSGRANNY_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/ansi/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/ansi/*.c"

)

# Add source files to the library
add_library(agnosgranny ${AGNOSGRANNY_SOURCES})


if(APPLE OR UNIX)
    target_compile_options(agnosgranny PRIVATE
        -fno-strict-aliasing
    )
endif()


# Specify include directories for granny2
target_include_directories(agnosgranny PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(agnosgranny PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

# Add subdirectories
add_subdirectory(gstate)
