cmake_minimum_required(VERSION 3.10)
project(EePubDesktopTest)

set(CMAKE_CXX_STANDARD 17)

include_directories(src external/lexbor/source external/elk/include)

add_executable(EePubTest
    src/EePub.cpp
    src/EpubLoader.cpp
    src/LexborWrapper.cpp
    src/Renderer.cpp
    main.cpp
)
