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
        get_filename_component(RELATIVE_DIR ${RELATIVE_HEADER_PATH} DIRECTORY)
        get_filename_component(HEADER_NAME ${RELATIVE_HEADER_PATH} NAME)
        string(REGEX REPLACE "\\.[^.]*$" "" HEADER_STEM ${HEADER_NAME})

        # Construct the full path for the output file with the new naming scheme.
        set(GENERATED_FILE "${REFLECTION_OUTPUT_DIR}/${RELATIVE_DIR}/${HEADER_STEM}.generated.cpp")
        list(APPEND CURRENTLY_GENERATED_FILES ${GENERATED_FILE})
        
        # Get the directory for the generated file.
        get_filename_component(GENERATED_FILE_DIR ${GENERATED_FILE} DIRECTORY)

        # Define the rule to generate one C++ file from one header.
        add_custom_command(
            OUTPUT  ${GENERATED_FILE}
            
            # This is a sequence of commands executed in order.
            COMMAND
                # 1. Ensure the output directory exists before trying to write to it.
                ${CMAKE_COMMAND} -E make_directory ${GENERATED_FILE_DIR}
            COMMAND
                # 2. CRITICAL FIX: Create an empty placeholder file first. This command
                #    is available in older CMake versions.
                ${CMAKE_COMMAND} -E touch ${GENERATED_FILE}
            COMMAND
                # 3. Run the generator. It will overwrite the empty placeholder if it
                #    finds any reflectable structs.
                $<TARGET_FILE:generator> ${HEADER_FILE} ${GENERATED_FILE} ${RELATIVE_HEADER_PATH}

            DEPENDS
                generator ${HEADER_FILE}
            COMMENT "Reflecting: ${RELATIVE_HEADER_PATH}"
            VERBATIM # Recommended for complex commands
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

endfunction()

# --- 3. Create a Central Target and Interface Library ---
# This is a global target that all individual generation targets will depend on.
add_custom_target(generate_reflection)

# Create the main INTERFACE library.
# Other targets will link against this to get include directories and dependencies.
add_library(reflecto INTERFACE)
add_dependencies(reflecto generate_reflection)