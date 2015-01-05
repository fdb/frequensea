cmake_minimum_required(VERSION 2.8.4)
project(frequensea)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -g -Wall -Werror -pedantic")

if (APPLE)
    include_directories(/System/Library/Frameworks /usr/local/include src)
    find_library(FRAMEWORK_COCOA Cocoa)
    find_library(FRAMEWORK_OPENGL OpenGL)
    find_library(FRAMEWORK_CORE_VIDEO CoreVideo)
    find_library(FRAMEWORK_IO_KIT IOKit)
    find_library(LIBRARY_GLFW glfw3)
    find_library(LIBRARY_LUA lua)
    find_library(LIBRARY_HACKRF hackrf)
    set(EXTRA_LIBS ${FRAMEWORK_COCOA} ${FRAMEWORK_OPENGL} ${FRAMEWORK_CORE_VIDEO} ${FRAMEWORK_IO_KIT} ${LIBRARY_GLFW} ${LIBRARY_LUA} ${LIBRARY_HACKRF})
endif (APPLE)

set(SOURCE_FILES
    src/nfile.c
    src/ngl.c
    src/noise.c
    src/nrf.c
    src/nwm.c
    src/obj.c
    src/vec.c)

function(add_demo demo_name)
  add_executable(${demo_name} ${SOURCE_FILES} demo/${demo_name}.c)
  target_link_libraries(${demo_name} ${EXTRA_LIBS})
endfunction(add_demo)

add_executable(frequensea ${SOURCE_FILES} src/main.c)
target_link_libraries(frequensea ${EXTRA_LIBS})

add_demo(static)
