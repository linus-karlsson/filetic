cmake_minimum_required(VERSION 3.16.3) 

project(FileTic)
set(EXE ${PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
IF(WIN32)
    set(STANDARD_FLAGS "-WL -nologo -Gm- -WX -W4 -wd4505 -wd4100 -wd4201 -wd4189 -wd4101 -wd4127 -DCRASH_DEREF")
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

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

IF (WIN32)
    file(GLOB PLATFORM "${CMAKE_SOURCE_DIR}/src/windows/*.c")
ELSE()
ENDIF()

add_executable(${EXE} ${SOURCES} ${PLATFORM})

target_include_directories(${EXE}
    PUBLIC "${CMAKE_SOURCE_DIR}"
    PUBLIC "${CMAKE_SOURCE_DIR}/include"
    PUBLIC "${CMAKE_SOURCE_DIR}/src"
    PUBLIC "${CMAKE_SOURCE_DIR}/vendor"
)

IF (WIN32)
    target_link_options(${EXE} PRIVATE "/SUBSYSTEM:WINDOWS" PRIVATE "/ENTRY:mainCRTStartup") 
    # For gcc and g++, clang
    #target_link_options(${EXE} PRIVATE "-mwindows") 
    target_link_libraries(${EXE} user32 Winmm)

    option(WINDOW_32 "Windows" ON)
    option(LINUX "Linux" OFF)

ELSE()
    target_link_libraries(${EXE})

    option(WINDOW_32 "Windows" OFF)
    option(LINUX "Linux" ON)
ENDIF()
