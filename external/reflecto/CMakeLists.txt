# CMakeLists.txt for the 'reflecto' code generation tool

cmake_minimum_required(VERSION 3.16)
project(Reflecto)

# --- 1. Build the Generator Tool (once) ---
# This creates the 'generator' executable from your C++ source file.
# It links against cpp-peglib, which it needs for parsing.
add_executable(generator generator.cpp)
target_link_libraries(generator PRIVATE peglib)


# --- 2. Define the Code Generation Function ---
# This function can be called multiple times for different source directories.
#
# Parameters:
#     TARGET_NAME: A unique name for the generation target (e.g., "execution_reflection").
#     SOURCE_PATH: The absolute path to the directory to scan for headers.
#     OUTPUT_VAR:  The variable name that will hold the list of generated source files.
#
function(generate_reflection_sources TARGET_NAME SOURCE_PATH OUTPUT_VAR)
    # Define the output directory for this specific generation task.
    set(REFLECTION_OUTPUT_DIR ${CMAKE_BINARY_DIR}/reflected/${TARGET_NAME})

    # Find all .h and .hpp files recursively within the source directory.
    file(GLOB_RECURSE HEADERS_TO_REFLECT
        "${SOURCE_PATH}/*.h"
        "${SOURCE_PATH}/*.hpp"
    )

    set(CURRENTLY_GENERATED_FILES "")

    # Loop through each header file found.
    foreach(HEADER_FILE ${HEADERS_TO_REFLECT})
        # Get the path of the header relative to the source directory to preserve structure.
        file(RELATIVE_PATH RELATIVE_HEADER_PATH ${SOURCE_PATH} ${HEADER_FILE})

        # --- CORRECTED FILENAME LOGIC (CMake < 3.20 compatible) ---
        # Get the directory part of the relative path (e.g., "execution/nodes")
        get_filename_component(RELATIVE_DIR ${RELATIVE_HEADER_PATH} DIRECTORY)
        
        # Get just the filename with its extension (e.g., "BlueprintCanvas.hpp")
        get_filename_component(HEADER_NAME ${RELATIVE_HEADER_PATH} NAME)
        # Use a regex to strip the extension, creating the "stem" (e.g., "BlueprintCanvas")
        string(REGEX REPLACE "\\.[^.]*$" "" HEADER_STEM ${HEADER_NAME})

        # Construct the full path for the output file with the new naming scheme.
        set(GENERATED_FILE "${REFLECTION_OUTPUT_DIR}/${RELATIVE_DIR}/${HEADER_STEM}.generated.cpp")
        list(APPEND CURRENTLY_GENERATED_FILES ${GENERATED_FILE})

        # Define the rule to generate one C++ file from one header.
        add_custom_command(
            OUTPUT  ${GENERATED_FILE}
            COMMAND
                $<TARGET_FILE:generator> ${HEADER_FILE} ${GENERATED_FILE} ${RELATIVE_HEADER_PATH}
            DEPENDS
                generator ${HEADER_FILE}
            COMMENT "Reflecting: ${RELATIVE_HEADER_PATH}"
        )
    endforeach()

    # Create a custom target that groups all generated files for this specific path.
    add_custom_target(
        ${TARGET_NAME}
        DEPENDS ${CURRENTLY_GENERATED_FILES}
    )

    # Make the list of generated files available to the parent scope.
    set(${OUTPUT_VAR} ${CURRENTLY_GENERATED_FILES} PARENT_SCOPE)

    # Add the dependency to the main 'generate_reflection' target.
    add_dependencies(generate_reflection ${TARGET_NAME})

    # Add the specific output directory to the include path of the main 'reflecto' library.
    target_include_directories(reflecto INTERFACE ${REFLECTION_OUTPUT_DIR})

endfunction()

# --- 3. Create a Central Target and Interface Library ---
# This is a global target that all individual generation targets will depend on.
add_custom_target(generate_reflection)

# Create the main INTERFACE library.
# Other targets will link against this to get include directories and dependencies.
add_library(reflecto INTERFACE)
add_dependencies(reflecto generate_reflection)