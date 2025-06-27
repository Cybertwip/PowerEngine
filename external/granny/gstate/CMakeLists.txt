cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recursively glob source files from src directory
file(GLOB AGNOSGRANNY_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/gstate_character_info.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gstate_base.cpp"
)

# Add source files to the library
add_library(gstate ${AGNOSGRANNY_SOURCES})


if(APPLE OR UNIX)
    target_compile_options(gstate PRIVATE
        -fno-strict-aliasing
    )
endif()


# Specify include directories for granny2
target_include_directories(agnosgranny PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(agnosgranny PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../includ)
target_link_libraries(gstate agnosgranny)

