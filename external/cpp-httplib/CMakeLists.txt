cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

# Extract version from httplib.h as before
file(STRINGS httplib.h _raw_version_string REGEX "CPPHTTPLIB_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+)\"")
string(REGEX MATCH "([0-9]+\\.?)+"
       _httplib_version
       "${_raw_version_string}")

project(httplib
    VERSION ${_httplib_version}
    LANGUAGES CXX
    DESCRIPTION "A C++ header-only HTTP/HTTPS server and client library."
    HOMEPAGE_URL "https://github.com/yhirose/cpp-httplib"
)

# Set OpenSSL minimum version
set(_HTTPLIB_OPENSSL_MIN_VER "3.0.0")

# Options as before
option(HTTPLIB_NO_EXCEPTIONS "Disable the use of C++ exceptions" OFF)
option(HTTPLIB_REQUIRE_OPENSSL "Requires OpenSSL to be found & linked, or fails build." ON)
option(HTTPLIB_REQUIRE_ZLIB "Requires ZLIB to be found & linked, or fails build." OFF)
option(HTTPLIB_USE_OPENSSL_IF_AVAILABLE "Uses OpenSSL (if available) to enable HTTPS support." ON)
option(HTTPLIB_USE_ZLIB_IF_AVAILABLE "Uses ZLIB (if available) to enable Zlib compression support." ON)
option(HTTPLIB_COMPILE "If ON, uses a Python script to split the header into a compilable header & source file (requires Python v3)." OFF)
option(HTTPLIB_INSTALL "Enables the installation target" ON)
option(HTTPLIB_TEST "Enables testing and builds tests" OFF)
option(HTTPLIB_REQUIRE_BROTLI "Requires Brotli to be found & linked, or fails build." OFF)
option(HTTPLIB_USE_BROTLI_IF_AVAILABLE "Uses Brotli (if available) to enable Brotli decompression support." ON)
option(HTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN "Enable feature to load system certs from the Apple Keychain." ON)
option(BUILD_SHARED_LIBS "Build the library as a shared library instead of static. Has no effect if using header-only." OFF)

if (BUILD_SHARED_LIBS AND WIN32 AND HTTPLIB_COMPILE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Set compilation flags
set(HTTPLIB_IS_COMPILED ${HTTPLIB_COMPILE})
set(HTTPLIB_IS_USING_CERTS_FROM_MACOSX_KEYCHAIN ${HTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN})

# Threads
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Find OpenSSL 3.x
if(HTTPLIB_REQUIRE_OPENSSL)
    find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto SSL)
    if(OPENSSL_VERSION VERSION_LESS "3.0.0")
        message(FATAL_ERROR "Found OpenSSL version ${OpenSSL_VERSION}, but version >= 3.0.0 is required.")
    endif()
    set(HTTPLIB_IS_USING_OPENSSL TRUE)
elseif(HTTPLIB_USE_OPENSSL_IF_AVAILABLE)
    find_package(OpenSSL 3.0 COMPONENTS Crypto SSL QUIET)
    if(OpenSSL_FOUND AND OPENSSL_VERSION VERSION_GREATER_EQUAL "3.0.0")
        set(HTTPLIB_IS_USING_OPENSSL TRUE)
    else()
        set(HTTPLIB_IS_USING_OPENSSL FALSE)
    endif()
endif()

# Find ZLIB
if(HTTPLIB_REQUIRE_ZLIB)
    find_package(ZLIB REQUIRED)
    set(HTTPLIB_IS_USING_ZLIB TRUE)
elseif(HTTPLIB_USE_ZLIB_IF_AVAILABLE)
    find_package(ZLIB QUIET)
    if(TARGET ZLIB::ZLIB)
        set(HTTPLIB_IS_USING_ZLIB TRUE)
    endif()
endif()

# Find Brotli using custom FindBrotli.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(HTTPLIB_REQUIRE_BROTLI)
    find_package(Brotli COMPONENTS encoder decoder common REQUIRED)
    set(HTTPLIB_IS_USING_BROTLI TRUE)
elseif(HTTPLIB_USE_BROTLI_IF_AVAILABLE)
    find_package(Brotli COMPONENTS encoder decoder common QUIET)
    set(HTTPLIB_IS_USING_BROTLI ${Brotli_FOUND})
endif()

# Include directories
include(GNUInstallDirs)

if(HTTPLIB_COMPILE)
    configure_file(split.py "${CMAKE_CURRENT_BINARY_DIR}/split.py" COPYONLY)
    configure_file(httplib.h "${CMAKE_CURRENT_BINARY_DIR}/httplib.h" COPYONLY)

    set(_INTERFACE_OR_PUBLIC PUBLIC)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/split.py"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE split_result
        OUTPUT_VARIABLE split_output
        ERROR_VARIABLE split_error
    )
    if(NOT split_result EQUAL 0)
        message(FATAL_ERROR "Failed to split cpp-httplib with the Python script.\n${split_error}")
    endif()

    set(_httplib_build_includedir "${CMAKE_CURRENT_BINARY_DIR}/out")
    add_library(${PROJECT_NAME} "${_httplib_build_includedir}/httplib.cc")
    target_sources(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${_httplib_build_includedir}/httplib.h>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/httplib.h>
    )
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
            OUTPUT_NAME cpp-httplib
    )
else()
    set(_INTERFACE_OR_PUBLIC INTERFACE)
    add_library(${PROJECT_NAME} INTERFACE)
    set(_httplib_build_includedir "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} ${_INTERFACE_OR_PUBLIC} cxx_std_11)

target_include_directories(${PROJECT_NAME} SYSTEM ${_INTERFACE_OR_PUBLIC}
    $<BUILD_INTERFACE:${_httplib_build_includedir}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} ${_INTERFACE_OR_PUBLIC}
    Threads::Threads
    $<$<PLATFORM_ID:Windows>:ws2_32>
    $<$<PLATFORM_ID:Windows>:crypt32>
    "$<$<AND:$<PLATFORM_ID:Darwin>,$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>,$<BOOL:${HTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN}>>:-framework CoreFoundation -framework Security>"
    $<$<BOOL:${HTTPLIB_IS_USING_BROTLI}>:Brotli::common>
    $<$<BOOL:${HTTPLIB_IS_USING_BROTLI}>:Brotli::encoder>
    $<$<BOOL:${HTTPLIB_IS_USING_BROTLI}>:Brotli::decoder>
    $<$<BOOL:${HTTPLIB_IS_USING_ZLIB}>:ZLIB::ZLIB>
    $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:OpenSSL::SSL>
    $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:OpenSSL::Crypto>
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} ${_INTERFACE_OR_PUBLIC}
    $<$<BOOL:${HTTPLIB_NO_EXCEPTIONS}>:CPPHTTPLIB_NO_EXCEPTIONS>
    $<$<BOOL:${HTTPLIB_IS_USING_BROTLI}>:CPPHTTPLIB_BROTLI_SUPPORT>
    $<$<BOOL:${HTTPLIB_IS_USING_ZLIB}>:CPPHTTPLIB_ZLIB_SUPPORT>
    $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:CPPHTTPLIB_OPENSSL_SUPPORT>
    $<$<AND:$<PLATFORM_ID:Darwin>,$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>,$<BOOL:${HTTPLIB_IS_USING_CERTS_FROM_MACOSX_KEYCHAIN}>>:CPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN>
)

# CMake package configuration
set(_TARGET_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

include(CMakePackageConfigHelpers)

configure_package_config_file("cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${_TARGET_INSTALL_CMAKEDIR}"
    PATH_VARS CMAKE_INSTALL_FULL_INCLUDEDIR
)

if(HTTPLIB_COMPILE)
    write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
        COMPATIBILITY SameMinorVersion
    )
else()
    write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
        COMPATIBILITY SameMinorVersion
        ARCH_INDEPENDENT
    )
endif()

if(HTTPLIB_INSTALL)
    install(TARGETS ${PROJECT_NAME} EXPORT httplibTargets)

    install(FILES "${_httplib_build_includedir}/httplib.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindBrotli.cmake"
        DESTINATION "${_TARGET_INSTALL_CMAKEDIR}"
    )

    install(EXPORT httplibTargets
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${_TARGET_INSTALL_CMAKEDIR}"
    )

    install(FILES "README.md" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
    install(FILES "LICENSE" DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}")
endif()

if(HTTPLIB_TEST)
    include(CTest)
    add_subdirectory(test)
endif()
