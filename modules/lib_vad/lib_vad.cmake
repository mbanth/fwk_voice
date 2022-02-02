## Source files
file( GLOB_RECURSE    LIB_VAD_C_SOURCES       src/*.c )

set(DEPS_ROOT ../../..)

## LIB_DSP temp
file( GLOB_RECURSE LIB_DSP_SOURCES_C  ${DEPS_ROOT}/lib_dsp/lib_dsp/src/*.c )
file( GLOB_RECURSE LIB_DSP_SOURCES_XC  ${DEPS_ROOT}/lib_dsp/lib_dsp/src/*.xc )
file( GLOB_RECURSE LIB_DSP_SOURCES_ASM  ${DEPS_ROOT}/lib_dsp/lib_dsp/src/*.S )
set( LIB_DSP_SOURCES  ${LIB_DSP_SOURCES_C} ${LIB_DSP_SOURCES_XC} ${LIB_DSP_SOURCES_ASM})
set( LIB_DSP_INCLUDES  ${DEPS_ROOT}/lib_dsp/lib_dsp/api/)

message( STATUS "DEPS_ROOT ${DEPS_ROOT}" )


set( LIB_VAD_INCLUDES  ${CMAKE_CURRENT_LIST_DIR}/api ${CMAKE_CURRENT_LIST_DIR}/src  ${LIB_DSP_INCLUDES})


unset(LIB_VAD_SOURCES_XCORE)
list( APPEND  LIB_VAD_SOURCES_XCORE  ${LIB_VAD_ASM_SOURCES} ${LIB_DSP_SOURCES})

unset(LIB_VAD_SOURCES)
list( APPEND  LIB_VAD_SOURCES   ${LIB_VAD_C_SOURCES}    )
list( APPEND  LIB_VAD_SOURCES   ${LIB_VAD_C_SOURCES_${CMAKE_SYSTEM_NAME}}    )

## cmake doesn't recognize .S files as assembly by default
set_source_files_properties( ${LIB_XS3_MATH_ASM_SOURCES} PROPERTIES LANGUAGE ASM )

message( STATUS "LIB_VAD_INCLUDES ${LIB_VAD_INCLUDES}" )
message( STATUS "LIB_VAD_C_SOURCES ${LIB_VAD_C_SOURCES}" )
