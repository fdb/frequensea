cmake_minimum_required(VERSION 3.14)
project(frequensea)

# Set compiler flags globally
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -g -Wall -pedantic")

# Add Lua subdirectory
add_subdirectory(externals/lua)

# Include local directories
include_directories(src)
include_directories(externals/ovr/Include)
include_directories(externals/ovr/Src)
include_directories(externals/lua/src)
include_directories(externals/stb)

# Find package configs
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

# Use pkg-config to find all external dependencies
pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(GLEW REQUIRED glew)
pkg_search_module(PNG REQUIRED libpng)
pkg_search_module(FFTW REQUIRED fftw3)
pkg_search_module(HACKRF REQUIRED libhackrf)
pkg_search_module(RTLSDR REQUIRED librtlsdr)

# Platform-specific configurations
if(APPLE)
    # Set PKG_CONFIG_PATH to find openal-soft
    set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/openal-soft/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    pkg_search_module(OPENAL REQUIRED openal)

    # Apple-specific frameworks
    find_library(FRAMEWORK_COCOA Cocoa REQUIRED)
    find_library(FRAMEWORK_OPENGL OpenGL REQUIRED)
    find_library(FRAMEWORK_CORE_VIDEO CoreVideo REQUIRED)
    find_library(FRAMEWORK_IO_KIT IOKit REQUIRED)

    set(PLATFORM_LIBS
        ${FRAMEWORK_COCOA}
        ${FRAMEWORK_OPENGL}
        ${FRAMEWORK_CORE_VIDEO}
        ${FRAMEWORK_IO_KIT}
    )

    include_directories(/System/Library/Frameworks /usr/local/include)
elseif(UNIX AND NOT APPLE)
    # Linux configuration
    pkg_search_module(OPENAL REQUIRED openal)
    find_package(X11 REQUIRED)
    set(PLATFORM_LIBS ${X11_X11_LIB} ${X11_Xrandr_LIB})
else()
    # For other platforms
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIBRARIES ${OPENAL_LIBRARY})
endif()

# Libraries that don't typically have pkg-config modules
find_library(LIBRARY_MATH m REQUIRED)
find_library(LIBRARY_PTHREAD pthread REQUIRED)

# Include all necessary directories
include_directories(
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${FFTW_INCLUDE_DIRS}
    ${HACKRF_INCLUDE_DIRS}
    ${RTLSDR_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

link_directories(
    ${FFTW_LIBRARY_DIRS}
    ${HACKRF_LIBRARY_DIRS}
    ${PNG_LIBRARY_DIRS}
    ${RTLSDR_LIBRARY_DIRS}
    ${GLFW_LIBRARY_DIRS}
    ${GLEW_LIBRARY_DIRS}
    ${OPENAL_LIBRARY_DIRS}
)

# Source files
set(SOURCE_FILES
    src/main.cpp
    src/nfile.c
    src/ngl.c
    src/nim.c
    src/noise.c
    src/nosc.c
    src/nrf.c
    src/nut.c
    src/nwm.c
    src/obj.c
    src/vec.c
)

# Conditional source files
option(WITH_NVR "Build with NVR support" OFF)
if(WITH_NVR)
    list(APPEND SOURCE_FILES src/nvr.cpp)
endif()

# Create executable
add_executable(frequensea ${SOURCE_FILES})

# Link libraries
target_link_libraries(frequensea
    ${LIBRARY_MATH}
    ${LIBRARY_PTHREAD}
    ${FFTW_LIBRARIES}
    ${HACKRF_LIBRARIES}
    ${PNG_LIBRARIES}
    ${RTLSDR_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${PLATFORM_LIBS}
    lua
    stdc++
)
