## Source files
file( GLOB_RECURSE    LIB_ADEC_C_SOURCES       src/*.c )

## set LIB_ADEC_INCLUDES & LIB_ADEC_SOURCES
set( LIB_ADEC_INCLUDES     "${CMAKE_CURRENT_LIST_DIR}/api"           )

unset(LIB_ADEC_SOURCES)
list( APPEND  LIB_ADEC_SOURCES   ${LIB_ADEC_C_SOURCES}    )
