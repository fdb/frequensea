cmake_minimum_required(VERSION 2.8.4)
project(frequensea)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99  -O3 -w")

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENAL REQUIRED openal)

add_subdirectory(externals/lua)

include_directories(src)
include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads/)
include_directories(/opt/vc/include/interface/vmcs_host/linux/)
include_directories(externals/ovr/include)
include_directories(externals/lua/src)
include_directories(externals/stb)
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${OPENAL_INCLUDE_DIRS})

find_library(LIBRARY_MATH m)
find_library(LIBRARY_PTHREAD pthread)
find_library(LIBRARY_FFTW fftw3)
find_library(LIBRARY_HACKRF hackrf)
find_library(LIBRARY_PNG png)
find_library(LIBRARY_RTLSDR rtlsdr)
set(CORE_LIBS ${LIBRARY_MATH} ${LIBRARY_PTHREAD} ${LIBRARY_FFTW} ${LIBRARY_HACKRF} ${LIBRARY_PNG} ${LIBRARY_RTLSDR} -L/opt/vc/lib -lGLESv2 -lEGL -lm  -lrt  -lbcm_host -lvcos -lvchiq_arm -lpthread)

if (APPLE)
    include_directories(/System/Library/Frameworks /usr/local/include)
    find_library(FRAMEWORK_COCOA Cocoa)
    find_library(FRAMEWORK_OPENGL OpenGL)
    find_library(FRAMEWORK_OPENAL OpenAL)
    find_library(FRAMEWORK_CORE_VIDEO CoreVideo)
    find_library(FRAMEWORK_IO_KIT IOKit)
    find_library(LIBRARY_GLFW glfw3)
    find_library(LIBRARY_OVR ovr PATHS externals/ovr/lib/darwin)
    add_definitions(-DWITH_OVR)
    set(PLATFORM_LIBS ${FRAMEWORK_COCOA} ${FRAMEWORK_OPENGL} ${FRAMEWORK_OPENAL} ${FRAMEWORK_CORE_VIDEO} ${FRAMEWORK_IO_KIT} ${LIBRARY_GLFW} ${LIBRARY_OVR})
endif (APPLE)

if (UNIX)
    find_library(LIBRARY_OVR ovr PATHS externals/ovr/lib/linux)
    set(PLATFORM_LIBS ${LIBRARY_OVR})
endif (UNIX)

set(SOURCE_FILES
    src/main.c
    src/nfile.c
    src/ngl.c
    src/nim.c
    src/noise.c
    src/nrf.c
    src/nul.c
    src/nwm.c
    src/obj.c
    src/vec.c)

if (APPLE)
    list(append SOURCE_FILES src/nvr.c)
endif (APPLE)


add_executable(frequensea ${SOURCE_FILES})
target_link_libraries(frequensea ${CORE_LIBS} ${PLATFORM_LIBS} stdc++ lua ${GLFW_STATIC_LIBRARIES}  ${OPENAL_LIBRARIES})
