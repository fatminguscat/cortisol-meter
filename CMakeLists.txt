cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(CortisolMeter VERSION 1.0.0)

# Keep the builder's home folder (and so their name) out of the shipped DLL: source paths baked in
# by __FILE__ - ours and the Geode SDK's dependencies' - and the PDB path in the debug directory.
# (CI machines have no personal paths, so this is only for local builds.)
if (DEFINED ENV{USERPROFILE} AND NOT DEFINED ENV{CI})
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" BUILDER_HOME)
    add_compile_options("-ffile-prefix-map=${BUILDER_HOME}=~" "-ffile-prefix-map=$ENV{USERPROFILE}=~")
    add_link_options("LINKER:/pdbaltpath:%_PDB%")
endif()

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Unattended in-game test driver (src/SmokeTest.cpp). Never ship a build with this on.
option(CM_SMOKE_TEST "Build the in-game smoke test driver" OFF)
if (CM_SMOKE_TEST)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CM_SMOKE_TEST)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})
