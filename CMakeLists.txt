cmake_minimum_required(VERSION 2.8.4)
project(frequensea)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -g -Wall -Werror -pedantic")

add_subdirectory(externals/lua)

include_directories(src)
include_directories(externals/ovr/Include)
include_directories(externals/ovr/Src)
include_directories(externals/lua/src)
include_directories(externals/stb)

find_library(LIBRARY_MATH m)
find_library(LIBRARY_PTHREAD pthread)
find_library(LIBRARY_FFTW fftw3)
find_library(LIBRARY_HACKRF hackrf)
find_library(LIBRARY_PNG png)
find_library(LIBRARY_RTLSDR rtlsdr)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include(FindOpenAL)
set(CORE_LIBS ${LIBRARY_MATH} ${LIBRARY_PTHREAD} ${LIBRARY_FFTW} ${LIBRARY_HACKRF} ${LIBRARY_PNG} ${LIBRARY_RTLSDR} ${OPENGL_LIBRARY})
include_directories(${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})

if (APPLE)
    include_directories(/System/Library/Frameworks /usr/local/include)
    find_library(LIBRARY_OVR ovr PATHS externals/ovr/Lib/Mac/Debug)
    find_library(FRAMEWORK_COCOA Cocoa)
    find_library(FRAMEWORK_OPENGL OpenGL)
    find_library(FRAMEWORK_OPENAL OpenAL)
    find_library(FRAMEWORK_CORE_VIDEO CoreVideo)
    find_library(FRAMEWORK_IO_KIT IOKit)
    set(PLATFORM_LIBS ${LIBRARY_OVR} ${FRAMEWORK_COCOA} ${FRAMEWORK_OPENGL} ${FRAMEWORK_OPENAL} ${FRAMEWORK_CORE_VIDEO} ${FRAMEWORK_IO_KIT})
    add_definitions(-DWITH_NVR)
    set(WITH_NVR 1)
endif (APPLE)

if (LINUX)
    find_package(X11 REQUIRED)
    set(PLATFORM_LIBS ${X11_X11_LIB} ${RT_LIBRARY} ${X11_Xrandr_LIB})
endif (LINUX)

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
    src/vec.c)

if (WITH_NVR)
    set(SOURCE_FILES ${SOURCE_FILES} src/nvr.cpp)
endif (WITH_NVR)

add_executable(frequensea ${SOURCE_FILES})
target_link_libraries(frequensea ${CORE_LIBS} ${PLATFORM_LIBS} stdc++ lua ${GLEW_LIBRARIES} ${OPENAL_LIBRARY} ${GLFW_LDFLAGS})
