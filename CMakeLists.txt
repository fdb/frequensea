cmake_minimum_required(VERSION 2.8.4)
project(frequensea)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -g -Wall -Werror -pedantic")

include_directories(src)
include_directories(externals/ovr/include)

find_library(LIBRARY_CPP stdc++)
find_library(LIBRARY_FFTW fftw3)
find_library(LIBRARY_GLFW glfw)
find_library(LIBRARY_HACKRF hackrf)
find_library(LIBRARY_LUA lua)
find_library(LIBRARY_OVR ovr PATHS externals/ovr/lib/darwin)
find_library(LIBRARY_PNG png)
find_library(LIBRARY_RTLSDR rtlsdr)
set(CORE_LIBS ${LIBRARY_CPP} ${LIBRARY_FFTW} ${LIBRARY_GLFW} ${LIBRARY_HACKRF} ${LIBRARY_LUA} ${LIBRARY_OVR} ${LIBRARY_PNG} ${LIBRARY_RTLSDR})

if (APPLE)
    include_directories(/System/Library/Frameworks /usr/local/include)
    find_library(FRAMEWORK_COCOA Cocoa)
    find_library(FRAMEWORK_OPENGL OpenGL)
    find_library(FRAMEWORK_OPENAL OpenAL)
    find_library(FRAMEWORK_CORE_VIDEO CoreVideo)
    find_library(FRAMEWORK_IO_KIT IOKit)
    find_library(LIBRARY_GLFW glfw3)
    set(PLATFORM_LIBS ${FRAMEWORK_COCOA} ${FRAMEWORK_OPENGL} ${FRAMEWORK_OPENAL} ${FRAMEWORK_CORE_VIDEO} ${FRAMEWORK_IO_KIT} ${LIBRARY_GLFW})
endif (APPLE)

if (LINUX)
    find_library(LIBRARY_GLFW glfw)
    set(PLATFORM_LIBS ${LIBRARY_GLFW})
endif (LINUX)

set(SOURCE_FILES
    src/main.c
    src/nfile.c
    src/ngl.c
    src/nim.c
    src/noise.c
    src/nrf.c
    src/nvr.c
    src/nwm.c
    src/obj.c
    src/vec.c)

add_executable(frequensea ${SOURCE_FILES})
target_link_libraries(frequensea ${CORE_LIBS} ${PLATFORM_LIBS})
