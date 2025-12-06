set(EEPUB_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../)

set(EEPUB_SRCS
    ${EEPUB_ROOT}/src/EePub.cpp
    ${EEPUB_ROOT}/src/ePubLoader.cpp
    ${EEPUB_ROOT}/src/LexborWrapper.cpp
    ${EEPUB_ROOT}/src/Renderer.cpp
    ${EEPUB_ROOT}/src/LayoutEngine.cpp
    ${EEPUB_ROOT}/src/MemoryMonitor.cpp
    ${EEPUB_ROOT}/src/hal/Hal.cpp
    ${EEPUB_ROOT}/external/miniz/miniz.c
    ${EEPUB_ROOT}/external/elk/elk.c
)

idf_component_register(
    SRCS ${EEPUB_SRCS}
    INCLUDE_DIRS
        ${EEPUB_ROOT}/src
        ${EEPUB_ROOT}/external/miniz
        ${EEPUB_ROOT}/external/elk/
        ${EEPUB_ROOT}/external/lexbor/source
    REQUIRES lexbor
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
