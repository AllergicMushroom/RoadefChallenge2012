cmake_minimum_required(VERSION 3.8) # For C++17 Support

project(RoadefGoogleChallenge LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set (UNIX TRUE)
    add_definitions(-DPLATFORM_UNIX)

    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	set(WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
    
    set(CMAKE_CXX_FLAGS "/W4 /EHsc")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
endif()

option(ENABLE_DEBUG "Debug option" OFF)
if (ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE DEBUG)
    add_definitions(-DMODE_DEBUG)
else()
    set(CMAKE_BUILD_TYPE RELEASE)
    add_definitions(-DMODE_RELEASE)
endif()

set(APP_NAME RoadefGoogleChallenge)

# Source files
include_directories(External)
include_directories(Source/Application)

file(GLOB_RECURSE COMMON_SOURCES 
    Source/Application/*.hpp
    Source/Application/*.cpp
)

# Executables
add_executable(${APP_NAME} ${COMMON_SOURCES} Source/Application/EntryPoint.cpp)
if (UNIX)
    target_link_libraries(${APP_NAME} pthread)
endif()

# Copy exemples and assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy Coverage generation for windows
# if (WIN32)
# file(COPY ${CMAKE_CURRENT_Source_DIR}/GenerateCoverage.bat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# endif()

# Coverage tests
# include(CTest)
# set(GLOBAl_PROPERTIES PROPERTIES TIMEOUT 30)