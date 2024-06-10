cmake_minimum_required(VERSION 3.16.3) 

project(Syntics)
set(EXE ${PROJECT_NAME})

#-march=native -mtune=native
#set(CMAKE_CXX_STANDARD 17)
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

option(INCLUDE_TEST "Include testning" OFF)

#set(SOURCES "${PROJECT_NAME}/src/syntics.c")
file(GLOB STB "${PROJECT_NAME}/vendor/stb/*.c")
file(GLOB SOURCES "${PROJECT_NAME}/src/*.c")
file(GLOB MATH "${PROJECT_NAME}/src/math/*.c")

IF (WIN32)
    file(GLOB PLATFORM "${PROJECT_NAME}/src/win32/*.c")
ELSE()
    file(GLOB PLATFORM "${PROJECT_NAME}/src/linux/*.c")
ENDIF()

if(INCLUDE_TEST)
    add_library(${EXE} STATIC ${SOURCES} ${STB} ${OBJ})
    add_subdirectory("${CMAKE_SOURCE_DIR}/Test")
    include(CTest)
    add_test("Test" "Test/bin/Test" COMMAND "Test/bin/Test")
else()
    add_executable(${EXE} ${SOURCES} ${STB} ${MATH} ${PLATFORM})
endif()

target_include_directories(${EXE}
    PUBLIC "${CMAKE_SOURCE_DIR}"
    PUBLIC "${CMAKE_SOURCE_DIR}\\${PROJECT_NAME}\\headers"
    PUBLIC "${CMAKE_SOURCE_DIR}\\${PROJECT_NAME}\\src"
    PUBLIC "${CMAKE_SOURCE_DIR}\\${PROJECT_NAME}\\vendor"
)

IF (WIN32)
    target_include_directories(${EXE} PUBLIC "C:\\VulkanSDK\\1.3.275.0\\Include")
    target_link_directories(${EXE} PRIVATE "C:\\VulkanSDK\\1.3.275.0\\Lib")
    target_link_options(${EXE} PRIVATE "/SUBSYSTEM:WINDOWS") 
    # For gcc and g++, clang
    #target_link_options(${EXE} PRIVATE "-mwindows") 
    target_link_libraries(${EXE} vulkan-1 user32 Winmm pdh)

    option(WINDOW_32 "Windows" ON)
    option(LINUX "Linux" OFF)

ELSE()
    target_link_libraries(${EXE} vulkan xcb xcb-xfixes X11 X11-xcb xcb-cursor m)

    option(WINDOW_32 "Windows" OFF)
    option(LINUX "Linux" ON)
ENDIF()
