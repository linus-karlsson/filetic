cmake_minimum_required(VERSION 3.16.3) 

project(FileTicTest)
set(EXE ${PROJECT_NAME})

set(GLFW_USE_HYBRID_HPG ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
IF(WIN32)
    set(STANDARD_FLAGS "-WL -nologo -Gm- -WX -W4 -wd4505 -wd4100 -wd4201 -wd4189 -wd4101 -wd4127 -wd4311")
    set(CMAKE_CXX_FLAGS_DEBUG "${STANDARD_FLAGS} -Od -Oi -Z7 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${STANDARD_FLAGS} -O2 -Oi -Z7 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${STANDARD_FLAGS} -Od -Oi -Z7 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${STANDARD_FLAGS} -O2 -Oi -Z7 -DNDEBUG")
ELSE()
    set(CMAKE_C_COMPILER "clang")
    set(STANDARD_FLAGS "-Wno-null-dereference -DCRASH_DEREF -DLINUX")
    set(CMAKE_CXX_FLAGS_DEBUG "${STANDARD_FLAGS} -g -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${STANDARD_FLAGS} -O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${STANDARD_FLAGS}  -g -fPIC -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${STANDARD_FLAGS} -O2 -pedantic -DNDEBUG")
ENDIF()

file(GLOB STB "../lib/stb/*.c")
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

add_subdirectory("../lib/glfw" "${CMAKE_BINARY_DIR}/glfw-build")

IF (WIN32)
    file(GLOB PLATFORM "../src/platform/windows/*.c")
ELSE()
ENDIF()

add_executable(${EXE} ${PLATFORM} ${SOURCES} ${STB} "../lib/glad/src/glad.c" "../lib/math/ftic_math.c" "../src/buffers.c" "../src/camera.c" "../src/directory.c" "../src/font.c" "../src/ftic_guid.c" "../src/ftic_window.c" "../src/hash.c" "../src/hash_table.c" "../src/logging.c" "../src/object_load.c" "../src/opengl_util.c" "../src/rendering.c" "../src/set.c" "../src/shader.c" "../src/texture.c" "../src/thread_queue.c" "../src/util.c" "../src/util.c" )

target_include_directories(${EXE}
    PUBLIC ".."
    PUBLIC "../include"
    PUBLIC "../src"
    PUBLIC "../lib"
    PUBLIC "../lib/math"
    PUBLIC "../lib/glad/include"
    PUBLIC "../lib/glfw/include"
)

target_link_libraries(${EXE} glfw)

IF (WIN32)
    target_link_options(${EXE} PRIVATE "/SUBSYSTEM:CONSOLE" PRIVATE "/ENTRY:mainCRTStartup") 
    # For gcc and g++, clang
    #target_link_options(${EXE} PRIVATE "-mwindows") 
    target_link_libraries(${EXE} user32 Winmm opengl32)

    option(WINDOW_32 "Windows" ON)
    option(LINUX "Linux" OFF)

ELSE()
    target_link_libraries(${EXE})

    option(WINDOW_32 "Windows" OFF)
    option(LINUX "Linux" ON)
ENDIF()
