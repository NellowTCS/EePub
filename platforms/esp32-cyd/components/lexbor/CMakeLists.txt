set(LEXBOR_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../external/lexbor)
set(LEXBOR_SRC_ROOT ${LEXBOR_ROOT}/source)

# Include the auto-generated list of wrapper sources (avoids object file name collisions)
include(${CMAKE_CURRENT_LIST_DIR}/generated/sources.cmake)

idf_component_register(
    SRCS ${LEXBOR_GENERATED_SRCS}
    INCLUDE_DIRS ${LEXBOR_SRC_ROOT}
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LEXBOR_BUILDING)

