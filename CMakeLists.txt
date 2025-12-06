cmake_minimum_required(VERSION 3.16)
project(EePub LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EEPUB_ENABLE_WASM "Build EePub WebAssembly bindings" OFF)
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(EEPUB_ENABLE_WASM ON)
endif()

set(LEXBOR_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/lexbor/build/liblexbor_static.a"
    CACHE FILEPATH "Path to the prebuilt Lexbor static library")

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
    src/LayoutEngine.cpp
    src/MemoryMonitor.cpp
    src/hal/Hal.cpp
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
    ${LEXBOR_LIB_PATH}
)

# Define macro for Desktop build
target_compile_definitions(EePubDesktop PRIVATE DESKTOP_BUILD)

if (EEPUB_ENABLE_WASM)
    add_executable(EePubWasm
        frameworks/Wasm/EePubWasm.cpp
        ${CORE_SRCS}
    )

    target_link_libraries(EePubWasm PRIVATE
        miniz
        elk
        ${LEXBOR_LIB_PATH}
    )

    target_compile_definitions(EePubWasm PRIVATE EEPUB_WASM)

    if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        target_link_options(EePubWasm PRIVATE
            "-sMODULARIZE=1"
            "-sEXPORT_NAME=EePubModule"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sFORCE_FILESYSTEM=1"
            "-sEXPORTED_RUNTIME_METHODS=['FS','FS_createPath','FS_unlink']"
            "--bind"
        )
    endif()
endif()
