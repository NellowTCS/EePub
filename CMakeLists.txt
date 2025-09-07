cmake_minimum_required(VERSION 3.16)
project(EePub LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/external/elk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/lexbor/source
)

# Core sources
set(CORE_SRCS
    src/EePub.cpp
    src/ePubLoader.cpp
    src/LexborWrapper.cpp
    src/Renderer.cpp
)

# Build external libraries as static
add_library(miniz STATIC external/miniz/miniz.c)
target_include_directories(miniz PUBLIC external/miniz)

add_library(elk STATIC external/elk/elk.c)
target_include_directories(elk PUBLIC external/elk/include)

# Desktop wrapper and example
add_executable(EePubDesktop
    frameworks/Desktop/main.cpp
    ${CORE_SRCS}
)

# Link static libraries
target_link_libraries(EePubDesktop PRIVATE
    miniz
    elk
    ${CMAKE_CURRENT_SOURCE_DIR}/external/lexbor/build/liblexbor_static.a
)

# Define macro for Desktop build
target_compile_definitions(EePubDesktop PRIVATE DESKTOP_BUILD)
